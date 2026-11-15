# ONE MAN STUDIO - Vestuario y UI antes de cada toma

El flujo normal usa ahora UnifiedWardrobeWidget, con ocho categorías y el retrato de Base3. La clase WardrobeWidget anterior se conserva como base compatible. Consulta la configuración actual en [UnifiedCustomization.md](UnifiedCustomization.md). Los ejemplos de consola de esta guía anterior siguen disponibles para diagnóstico.

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6.

Actualizacion de fase 6: se conserva este vestuario y el HUD de objetivos junto a el. Despues de las tres tomas hay titulo, premiere y resultados, descritos en [PremiereAndResults.md](PremiereAndResults.md). Las interacciones/objetivos siguen configurandose en [ObjectivesAndScoring.md](ObjectivesAndScoring.md).

## Probar sin editar Content

1. Abre el proyecto activo y tu mapa habitual. Inicia **Play**, un jugador, con el GameMode Third Person existente. No uses Simulate ni `Acting.Test`.
2. Ejecuta **`Movie.Test`** en la consola del juego. Usa 10 segundos de vestuario y 30 de grabacion. Para revisar mas rapido: **`Movie.Test 5 8`**. El comando crea un MovieManager temporal o reinicia el existente; no guarda el mapa.
3. Antes de **Take 1** aparece la UMG nativa. Muestra Robber, su instruccion, contador, categorias y Ready. La camara muestra al personaje real a la izquierda: ese es el preview, no una imagen estatica.
4. Elige Head, Body y Prop. Prueba tambien un accesorio y color. Los objetos del ejemplo son primitivas de colores; son placeholders, no los assets artisticos finales.
5. Pulsa **Ready** para comenzar antes, o deja vencer el contador. La interfaz se cierra, la vista vuelve al personaje y se habilita movimiento/actuacion. No necesitas llamar StartRecording ni manipular el reloj desde UI.
6. Muevete y usa los gestos/sonidos de fase 3 (1-5 y Z/X/C/V/B/N si Bind Acting Keys esta activo). Al terminar se abre otro vestuario para Police Officer y despues otro para Customer. Cada nuevo vestuario empieza desde el outfit predeterminado de ese rol.
7. Comprueba que los replays anteriores conservan sus outfits aunque cambies el del jugador. En la premiere aparecen los tres outfits con sus movimientos y eventos. **R** reinicia toda la pelicula si Bind Restart Key esta activo.

El vestuario no pausa el mundo. Su contador usa el tiempo de juego del MovieManager; el reloj compartido de grabacion permanece en cero. Durante esta fase se bloquean movimiento, mirada, grabacion y gestos. Los replays se preparan en silencio, con sus sonidos detenidos.

## Configurar MovieManager

No hace falta agregar actores ni modificar tu Blueprint del personaje para la prueba: MovieManager agrega WardrobeComponent temporalmente si falta. Para configuracion permanente, crea/configura tu Blueprint derivado de MovieManager o edita su instancia en tu mapa y guarda esos cambios tu mismo.

| Propiedad | Uso |
| --- | --- |
| Wardrobe Duration Seconds | 10 por defecto. Conserva el nombre C++/Blueprint `PreparationDurationSeconds` para compatibilidad con la fase 2 |
| Recording Duration Seconds | 30 por defecto |
| Roles | Exactamente 3. Cada rol tiene RoleId estable, DisplayName, ScriptInstruction, DefaultOutfit e InitialTransform |
| Cosmetic Catalog | IDs, meshes, materiales, iconos, sockets, offsets y tintes de los items |
| Use Performer Cosmetic Catalog | Si esta activo, usa el Catalog del WardrobeComponent del personaje en lugar del catalogo del gestor |
| Wardrobe Widget Class | Clase nativa lista para usar, o tu Widget Blueprint derivado de WardrobeWidget |
| Show Wardrobe UI | Activado por defecto. Desactivalo si tu propio HUD maneja la presentacion; el estado/contador siguen funcionando |
| Use Wardrobe Preview Camera | Activado. Camara temporal delante del personaje mientras el menu esta abierto |
| Wardrobe Preview Camera | Camara opcional del nivel: se usa tal cual, no se mueve ni se destruye |
| Wardrobe Camera Offset / Look At Offset / Field Of View | Encuadre automatico relativo a la pose inicial del personaje; ajustalo al tamano de tus personajes y tu layout |
| Use Wardrobe Fill Light / Wardrobe Fill Light Lumens | Luz temporal del preview, configurable y desactivada antes de grabar; no cambia las luces del mapa |
| Auto Start | Inicia cuando hay un Pawn poseido, igual que en fase 2 |

Los tres roles predeterminados son **Robber**, **Police Officer** y **Customer**. Sus Script Instructions son texto de actuacion; los objetivos y puntos se configuran aparte en Roles > Objectives desde fase 5. Los IDs personalizados de Blueprints anteriores siguen siendo validos. Si esos roles no tienen los nuevos textos, se muestran nombres predeterminados utiles.

Los inicios, camara de premiere y Replay Actor Class siguen configurandose como en [MovieManager.md](MovieManager.md). La camara de vestuario es independiente de la camara de premiere. El numero de tomas permanece en tres; `GetNumberOfTakes` lo expone para el HUD.

## Asignar cosmetics finales

En el MovieManager abre **Cosmetic Catalog**. Conserva un ID estable para cada item; puedes reordenar las entradas. Los outfits guardados no dependen del orden del array ni del widget. IDs incluidos:

| Categoria | IDs |
| --- | --- |
| Head | `Head.PoliceCap`, `Head.RobberMask`, `Head.Crown`, `Head.Chicken` |
| Body | `Body.Casual`, `Body.Uniform`, `Body.Suit`, `Body.Dress` |
| Prop | `Prop.Banana`, `Prop.Donut`, `Prop.MoneyBag`, `Prop.Microphone` |
| Face | `Face.Glasses`, `Face.Mustache` |

Para cada entrada:

1. Ajusta **Display Name** e **Icon**. El icono es opcional; la UMG nativa muestra texto si falta.
2. Para sombreros/mascaras/props, asigna **Static Mesh** y deja **Skeletal Mesh** vacio. Para ropa modular, asigna Skeletal Mesh y deja Static Mesh vacio. Usa un solo tipo de mesh por entrada.
3. Un accesorio estatico se adjunta a **Socket Name**, que admite socket o hueso existente. Los placeholders usan `head`, `spine_03`, `pelvis` o `hand_r`. Ajusta **Relative Transform** en ese espacio local para el pivot y orientacion de tu asset.
4. Si no existe ese socket/hueso o el actor es un placeholder sin skeletal mesh, se usa **Actor Relative Transform** respecto a la raiz del actor. **Fit In Actor Space** coloca inicialmente el item con ese transform y luego guarda el offset relativo al hueso para seguirlo al actuar. Los placeholders lo usan para evitar los ejes de importacion del rig; para assets ya preparados para sockets puedes desactivarlo y usar Relative Transform directamente.
5. Para ropa modular compatible deja **Use Leader Pose** activo. Debe tener el mismo Skeleton asset que el personaje. Se monta sobre la raiz del mesh, usa Relative Transform y sigue su pose; el socket se ignora en este modo para evitar aplicar dos veces los huesos. No se reemplazan el skeletal mesh base ni su Anim Blueprint.
6. Para una variante de materiales del cuerpo, deja **ambos meshes vacios** y asigna **Materials** por slot. Asi cambias la ropa/material del rig existente. Los null slots dejan su material original. Una entrada Body sin overrides puede representar el cuerpo original.
7. **Materials** en un item con mesh modifica los slots de ese accesorio/prenda. **Use Tint**, **Tint** y **Tint Parameter** permiten un vector de color de tu material; activa Use Tint si quieres un tinte predeterminado. Para Body, el color elegido por el jugador tiene prioridad. El parametro debe existir en tu material. Los placeholders del motor usan `Color` y sus instancias son privadas por actor.
8. En **Roles -> Default Outfit** asigna los IDs predeterminados de cada rol. Head/Face/Prop aceptan None. Body debe resolver a una entrada valida; si un default queda obsoleto, se usa otro Body valido de forma determinista.

El slot Face admite un accesorio a la vez: glasses o mustache. Para combinarlos sin agregar slots puedes proporcionar un solo mesh combinado con un nuevo ID. Los props son cosmeticos: no tienen colision, pickup ni acciones de gameplay.

Si usas un Pawn con varias mallas, agrega un **WardrobeComponent** al Blueprint y asigna **Target Mesh**; en Character normalmente se usa Character.Mesh automaticamente. **Fallback Body Mesh** sirve para placeholders estaticos. Wardrobe.TargetMesh y ActingComponent.TargetMesh deben representar el mismo personaje/rig para que el replay herede la misma apariencia.

## Personalizar la UMG sin C++

Hay una interfaz nativa funcional; no se creo ni modifico ningun Widget Blueprint dentro de tu Content. Puedes mantener ese layout o reemplazarlo.

### Mantener el layout nativo y cambiar estilo

1. Crea un **Widget Blueprint** con clase padre **WardrobeWidget**. Si tu version del dialogo no permite elegirla al crear, usa **File -> Reparent Blueprint**.
2. Para conservar el layout generado por C++, deja el **Designer sin root**: elimina el Canvas Panel vacio que el Editor pueda crear automaticamente. Si existe un root, se interpreta como un layout tuyo y el codigo lo conserva.
3. En Class Defaults configura **Panel Color**, **Text Color**, **Accent Color**, **Panel Width** y **Font Size**. Activa **Use Custom Button Style** para asignar brushes/texturas de Normal/Hovered/Pressed mediante Button Style. Los iconos vienen del catalogo.
4. Asigna ese Widget Blueprint a **MovieManager -> Wardrobe Widget Class**.

### Crear tu propio layout editable

1. Crea un Widget Blueprint derivado de WardrobeWidget y conserva/agrega un Canvas Panel como root. El sistema no sobrescribe ese arbol del Designer.
2. Deja transparente la zona del preview. Coloca tu panel de seleccion al lado derecho, o ajusta la camara de vestuario para tu layout. El preview es la vista del personaje en el mundo: no necesita RenderTarget ni SceneCapture.
3. Agrega Text Blocks para toma, rol, instruccion y contador, un Progress Bar opcional, categorias/items y un boton Ready. Marca como variables los controles que actualizaras en el Graph.
4. Implementa **On Wardrobe View Changed(Data)**. Break Wardrobe View Data proporciona TakeNumber, TakeCount, RoleName, RoleId, ScriptInstruction, RemainingSeconds, DurationSeconds, Selection y CanChoose. Usa Format Text para la toma y el contador, y Ceil para mostrar segundos enteros. El porcentaje del contador es RemainingSeconds / DurationSeconds, protegiendo division por cero.
5. Ready.OnClicked llama **Ready()** en este widget. No llames MovieManager.StartMovie, StartRecording o StartClock desde ese boton. El widget transmite automaticamente el ID de preparacion y el gestor evita confirmaciones dobles.
6. Cada boton de item llama **Choose Item(Category, Id)** con un ID del catalogo. Para quitar Head, Face o Prop usa None. Body necesita una entrada valida. Para color llama **Choose Body Color(UseColor, Color)**.
7. **Get Options(Category)** devuelve entradas completas del catalogo para listas dinamicas, incluyendo iconos. Conserva el ID en cada boton/list item; no uses su posicion para restaurar un outfit. Puedes construir tambien botones fijos para los catorce IDs de la tabla.
8. Usa **ViewData.CanChoose** para habilitar los controles. **On Wardrobe Opened** permite reproducir una transicion de UI o preparar tus listas. El gestor crea un widget por preparacion y lo retira al confirmar/cancelar.
9. Asigna la clase a Wardrobe Widget Class. Las funciones de seleccion devuelven false cuando el menu esta cerrado, el ID no existe o el widget corresponde a una preparacion anterior.

El gestor usa Game And UI mientras el menu esta abierto y vuelve a Game Only al cerrarlo. R tambien funciona desde el widget nativo si Bind Restart Key esta activo. Un HUD que necesite otro input mode al salir de la pelicula puede establecerlo al recibir OnStateChanged(Idle); el sistema restaura los flags previos de cursor/click/mouse-over, pero no puede consultar y reconstruir un input mode arbitrario de otro HUD.

## Datos y API

- **FCosmeticSelection**: HeadId, BodyId, FaceId, PropId, UseBodyColor y BodyColor.
- **FRecordedAppearance**: seleccion y copias de las definiciones resueltas (assets, transform/material settings y materiales base). Se guarda en **Performances[i].Recording.Appearance**. No hay lookup contra el catalogo actual durante un replay.
- **UWardrobeComponent**: SelectItem, SetSelection, SetBodyColor, NormalizeSelection, ValidateCatalog, CaptureAppearance, GetSelection, CanEditSelection y OnSelectionChanged/OnPresentationIssue.
- **MovieManager**: LiveWardrobeComponent, ActiveWardrobeWidget, PreparationId, IsWardrobeOpen, GetCurrentRoleName, GetCurrentScriptInstruction, SelectWardrobeItem(PreparationId,...), SetWardrobeBodyColor y ReadyForTake(PreparationId).
- **ReplayActor.Wardrobe** restaura snapshots como solo lectura. Sus componentes y tintes pertenecen a ese actor; cambiar el jugador no cambia los replays.
- Los resets detienen medios de actuacion, borran recordings/snapshots y vuelven al default de Take 1. StopMovie restaura el aspecto anterior a la pelicula y quita los cosmetics temporales.

Los datos siguen en memoria. El vestuario no agrega SaveGame, persistencia en disco, nuevos rigs ni outfits animados con cloth. Los objetivos/scoring de fase 5 se documentan en [ObjectivesAndScoring.md](ObjectivesAndScoring.md). Los montages y sonidos finales se asignan como en [ActingEvents.md](ActingEvents.md).

## Compilar y verificar

Con Unreal Editor cerrado, desde `C:\Games\OneManStudio\MyProject`:

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Test-Movie.ps1
.\Scripts\Test-WardrobePlay.ps1 -RenderScreenshots
.\Scripts\Build.ps1 -Target Game
```

Test-Movie exige 36 pruebas y un reporte nuevo desde fase 6. Test-WardrobePlay abre un proceso de juego separado sin ventana visible: usa el mapa existente, un LocalPlayer real y los botones UMG, alterna Ready/timeout, comprueba outfits/premiere/restart y cierra solo ese proceso al terminar. Usa **RenderScreenshots** para comprobar el countdown visible: activa el render/tick de UMG y guarda capturas en `Saved\Automation\WardrobePlay`; NullRHI no actualiza ese widget visual. No ejecutes el comando interno de smoke dentro de tu sesion de trabajo: esta protegido por flags de lanzamiento y lo administra el script.

El modo offscreen utiliza un argumento del motor descrito en la [referencia oficial de argumentos](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference). No agrega Pixel Streaming ni dependencias de red.

Resultados historicos de vestuario: [Phase4Verification.md](Phase4Verification.md). Archivos cambiados, pruebas y limitaciones actuales: [Phase5Verification.md](Phase5Verification.md).
