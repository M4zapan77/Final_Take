"""Read only binary FBX geometry/UV identifiers, omitting embedded image payloads."""
import hashlib,json,struct,zlib
from pathlib import Path

def inspect(path,include_uv=False):
    data=path.read_bytes()
    if not data.startswith(b'Kaydara FBX Binary'):return {'file':str(path),'binary':False}
    version=struct.unpack_from('<I',data,23)[0];wide=version>=7500;header=25 if wide else 13
    def prop(p):
        code=chr(data[p]);p+=1
        formats={'Y':'h','C':'?','I':'i','F':'f','D':'d','L':'q'}
        if code in formats:
            fmt='<'+formats[code];return struct.unpack_from(fmt,data,p)[0],p+struct.calcsize(fmt)
        if code in 'SR':
            size=struct.unpack_from('<I',data,p)[0];p+=4
            return data[p:p+size].decode('utf8',errors='replace') if code=='S' else None,p+size
        if code in 'fdlibc':
            count,encoding,size=struct.unpack_from('<III',data,p);p+=12;raw=data[p:p+size]
            if encoding:raw=zlib.decompress(raw)
            return {'array':code,'count':count,'bytes':raw},p+size
        raise ValueError('Unknown FBX property '+code)
    def node(p):
        end,count,length=struct.unpack_from('<QQQ' if wide else '<III',data,p);name_size=data[p+header-1]
        if not end:return None,p+header
        name=data[p+header:p+header+name_size].decode('ascii',errors='replace');p+=header+name_size
        values=[]
        for _ in range(count):value,p=prop(p);values.append(value)
        children=[]
        while p<end-header:
            child,p=node(p)
            if child:children.append(child)
            else:break
        return (name,values,children),end
    nodes=[];p=27
    while p+header<len(data):
        n,p=node(p)
        if not n:break
        nodes.append(n)
    geometries=[];models=[]
    def visit(n):
        name,values,children=n
        if name=='Geometry':
            g={'name':values[1] if len(values)>1 else '', 'type':values[-1]};uv_values=uv_indices=None
            for cname,cvalues,_ in children:
                if cname in ('Vertices','PolygonVertexIndex'):
                    a=cvalues[0];g[cname+'_count']=a['count'];g[cname+'_sha256']=hashlib.sha256(a['bytes']).hexdigest()
                    if cname=='Vertices':
                        vals=struct.unpack('<'+str(a['count'])+('d' if a['array']=='d' else 'f'),a['bytes'])
                        g['min']=[min(vals[i::3]) for i in range(3)];g['max']=[max(vals[i::3]) for i in range(3)]
                if cname=='LayerElementUV':
                    for nn,vv,_ in _:
                        if nn in ('UV','UVIndex') and vv and isinstance(vv[0],dict):
                            a=vv[0];g[nn+'_sha256']=hashlib.sha256(a['bytes']).hexdigest();g[nn+'_count']=a['count']
                            vals=struct.unpack('<'+str(a['count'])+{'d':'d','f':'f','i':'i','l':'q'}[a['array']],a['bytes'])
                            g[nn+'_sample']=list(vals[:12]);g[nn+'_quantized']=hashlib.sha256(str([round(v,5) for v in vals]).encode()).hexdigest()
                            if nn=='UV':uv_values=vals
                            if nn=='UVIndex':uv_indices=vals
            if include_uv and uv_values is not None and uv_indices is not None:g['expanded_uv']=[uv_values[2*i+j] for i in uv_indices for j in (0,1)]
            geometries.append(g)
        if name=='Model':models.append([str(v) for v in values if not isinstance(v,dict)])
        for c in children:visit(c)
    for n in nodes:visit(n)
    return {'file':str(path),'version':version,'geometries':geometries,'models':models}

if __name__=='__main__':
    root=Path(r'C:\Games\OneManStudio\MyProject\SourceArt\ProductionArt20261113')
    files=list(root.rglob('*.fbx'))+list(Path(r'C:\Games\OneManStudio\MyProject\SourceArt\Base3').glob('*.fbx'))
    out=[]
    for f in files:
        try:out.append(inspect(f))
        except Exception as e:out.append({'file':str(f),'error':str(e)})
    Path(r'C:\Games\OneManStudio\MyProject\Saved\ProductionArt\FbxGeometry.json').write_text(json.dumps(out,indent=2),encoding='utf8')
    for x in out:
        print(Path(x['file']).parts[-4:],[(g.get('Vertices_count',0)//3,g.get('PolygonVertexIndex_count'),g.get('min'),g.get('max')) for g in x.get('geometries',[])],x.get('error',''))
