# Personaje importado - base (3).fbx

Fuente pedida por el director: **C:\Users\diego\Downloads\base (3).fbx**, 131,772 bytes. Copia preservada en **C:\Games\OneManStudio\MyProject\SourceArt\Base3\base (3).fbx**. SHA-256: `739B3CF1EA24F2459DD44EE1685E3EA078D92AFE0D7CB6911DE7DD7FE86D946E`.

En el Content Browser abre **OneManStudio > Characters > Base3**:

- **SK_Base3**: Skeletal Mesh importado con pesos y 19 huesos.
- **SK_Base3_Skeleton**: esqueleto independiente, raiz Hips.
- **SK_Base3_PhysicsAsset**: Physics Asset generado al importar.
- **model**: material importado del FBX.

Haz doble clic en SK_Base3 para abrirlo en Skeletal Mesh Editor. Puedes arrastrarlo al nivel como actor de preview. El archivo no contiene animaciones ni referencias a texturas externas; no se inventaron clips o texturas finales.

**Actualización del camerino unificado:** Base3 es ahora el modelo visual por defecto durante el juego, el retrato y los replays. `StudioGameMode` / `MovieManager` aplican `Customization/DA_Base3Character` en ejecución, conservando el mismo pawn, controller y cápsula. El Animation Blueprint original de Quinn sigue disponible como respaldo y no debe asignarse directamente al Skeleton de Base3. Las copias retargeteadas, `ABP_Base3`, los sockets adaptados y las instrucciones actuales están en [UnifiedCustomization.md](UnifiedCustomization.md).

Los huesos importados son Hips, Spine, Spine1, Neck, Head; Left/RightShoulder, Arm, ForeArm, Hand; y Left/RightUpLeg, Leg, Foot. No hay huesos de dedos ni los IK bones adicionales del mannequin.

La importacion uso FbxFactory de UE 5.6, conversion de escena/unidades y escala 1; importo mesh/materiales y creo un nuevo esqueleto/Physics Asset, sin importar animaciones. PythonScriptPlugin/EditorScriptingUtilities se habilitaron solo para el proceso de importacion, sin modificar .uproject ni agregar dependencias runtime.

Reporte real: **Saved/CharacterImport/Base3Import.json**. Script: **Scripts/Import-Base3Character.py**; se niega a sobrescribir una carpeta Base3 con assets, para proteger tus cambios posteriores. El FBX original y los assets/mapas previos se verifican por hash al cerrar fase 7.

La verificacion con render **paso cuatro vistas** en un proceso separado; no guardo ningun mapa. Capturas en **Saved/CharacterImport/Preview/CharacterView1.png ... CharacterView4.png**. Bounds de render aproximados: **87.9 x 90.4 x 204.6 cm**, incluidos los margenes del skeletal mesh. Se revisaron visualmente el cuerpo, cabeza y extremidades; el FBX emitio el aviso de smoothing groups ausentes, pero importo y renderizo. No se probo aun una animacion retargeteada.

El perfil visual de MovieManager y el script de empaquetado incluyen Base3 y sus nuevos assets en el cook. Los datos originales de importación y el FBX descritos arriba permanecen intactos; no se modificó el Blueprint del jugador para cambiar su presentación en runtime.
