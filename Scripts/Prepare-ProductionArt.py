"""Preserve and inventory the director's archives; safely expand stored RAR5 and nested ZIPs.

RAR5 header layout: https://www.rarlab.com/technote.htm . This archive stores ZIP payloads;
only uncompressed, CRC-checked RAR entries are supported. No archive code is executed.
"""
from pathlib import Path, PurePosixPath
import hashlib
import io
import json
import shutil
import struct
import zipfile
import zlib

PROJECT = Path(r"C:\Games\OneManStudio\MyProject")
ROOT = PROJECT / "SourceArt" / "ProductionArt20261113"
AUDIT = PROJECT / "Saved" / "ProductionArt"
AUDIT.mkdir(parents=True, exist_ok=True)

def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1048576), b""):
            h.update(chunk)
    return h.hexdigest()

if not (AUDIT / "ProtectedBefore.json").exists():
    protected = []
    for folder in ("Content", "Config", "Source", "Scripts", "Docs"):
        for file in sorted((PROJECT/folder).rglob("*")):
            if file.is_file():
                protected.append({"path":str(file.relative_to(PROJECT)),"sha256":sha(file)})
    protected.append({"path":"MyProject.uproject","sha256":sha(PROJECT/"MyProject.uproject")})
    (AUDIT/"ProtectedBefore.json").write_text(json.dumps(protected,indent=2),encoding="utf8")
    backup=AUDIT/"Backups"
    for folder in ("Source","Scripts","Docs","Config"):
        shutil.copytree(PROJECT/folder,backup/folder,dirs_exist_ok=True)
    for file in (PROJECT/"Content/OneManStudio/Maps").glob("*"):
        if file.is_file():
            (backup/"Maps").mkdir(parents=True,exist_ok=True)
            shutil.copy2(file,backup/"Maps"/file.name)

def safe(root, name):
    p=PurePosixPath(name.replace("\\","/"))
    if p.is_absolute() or ".." in p.parts or any(":" in part for part in p.parts):
        raise RuntimeError("Unsafe archive path: "+name)
    target=root.joinpath(*p.parts)
    if not target.resolve().is_relative_to(root.resolve()):
        raise RuntimeError("Path escaped source directory")
    return target

def write(target, data):
    target.parent.mkdir(parents=True,exist_ok=True)
    if target.exists():
        if hashlib.sha256(target.read_bytes()).digest()!=hashlib.sha256(data).digest():
            raise RuntimeError("Existing extracted source differs: "+str(target))
    else:
        target.write_bytes(data)

def unzip(path, root):
    with zipfile.ZipFile(path) as archive:
        for entry in archive.infolist():
            if entry.is_dir():continue
            if (entry.external_attr>>16)&0o170000==0o120000:
                raise RuntimeError("Symlink in archive")
            target=safe(root,entry.filename)
            write(target,archive.read(entry))
    for nested in sorted(root.rglob("*.zip")):
        destination=nested.with_suffix("")
        if not (destination/".extracted").exists():
            unzip(nested,destination)
            (destination/".extracted").write_text(sha(nested),encoding="ascii")

def unrar_stored(path,root):
    data=path.read_bytes()
    if data[:8]!=b"Rar!\x1a\x07\x01\x00":raise RuntimeError("Not RAR5")
    position=8
    def vint():
        nonlocal position
        value=shift=0
        while True:
            byte=data[position];position+=1;value|=(byte&127)<<shift;shift+=7
            if shift>70:raise RuntimeError("Invalid RAR vint")
            if not byte&128:return value
    while position+7<len(data):
        crc=struct.unpack_from("<I",data,position)[0];position+=4;start=position
        header_size=vint();end=position+header_size
        if zlib.crc32(data[start:end])!=crc:raise RuntimeError("RAR header CRC mismatch")
        kind=vint();flags=vint();extra=vint() if flags&1 else 0;length=vint() if flags&2 else 0
        if flags&0x18:raise RuntimeError("Split RAR unsupported")
        if kind==2:
            file_flags=vint();unpacked=vint();attributes=vint()
            if file_flags&2:position+=4
            file_crc=struct.unpack_from("<I",data,position)[0] if file_flags&4 else None
            if file_flags&4:position+=4
            compression=vint();host=vint();name_length=vint();name=data[position:position+name_length].decode("utf8")
            method=(compression>>7)&7
            if method or unpacked!=length:raise RuntimeError("RAR compression requires a full extractor: "+name)
            if not file_flags&1:
                payload=data[end:end+length]
                if file_crc is not None and zlib.crc32(payload)!=file_crc:raise RuntimeError("RAR file CRC mismatch")
                write(safe(root,name),payload)
        position=end+length
        if kind==5:break
    for nested in sorted(root.rglob("*.zip")):
        dest=nested.with_suffix("")
        if not (dest/".extracted").exists():
            unzip(nested,dest)
            (dest/".extracted").write_text(sha(nested),encoding="ascii")

archives=[]
for name in ("banco_obj.rar","ropas.zip","objetos.zip","accesorios_cara.zip","gorros.zip"):
    path=Path(r"C:\Users\diego\Downloads")/name
    destination=ROOT/path.stem
    if name.endswith(".rar"):unrar_stored(path,destination)
    else:unzip(path,destination)
    archives.append({"archive":str(path),"sha256":sha(path),"extracted":str(destination)})

models=[]
for file in sorted(ROOT.rglob("*.fbx")):
    textures=[str(p) for p in file.parent.parent.rglob("*.png")]
    models.append({"file":str(file),"bytes":file.stat().st_size,"sha256":sha(file),"nearby_textures":textures})
(AUDIT/"SourceInventory.json").write_text(json.dumps({"archives":archives,"fbx":models},indent=2),encoding="utf8")
print(json.dumps({"archives":len(archives),"fbx_files":len(models),"png_files":len(list(ROOT.rglob('*.png'))),"source":str(ROOT)}))
