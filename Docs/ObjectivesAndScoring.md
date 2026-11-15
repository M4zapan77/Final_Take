# ONE MAN STUDIO - Objetivos, props y puntuacion

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6. El vestuario, recorder, reloj, actuaciones y MovieManager existentes siguen siendo los responsables de sus respectivas fases.

Actualizacion de fase 6: los resultados finales ahora aparecen despues de la premiere con estrellas/premios y botones Replay/New Movie. Durante la pelicula se oculta el HUD de gameplay. Configuracion actual: [PremiereAndResults.md](PremiereAndResults.md).

## Probar sin guardar cambios en el mapa

1. Abre el proyecto activo y tu mapa Third Person. Inicia **Play**, con un jugador y un personaje poseido; no Simulate.
2. En la consola ejecuta **`Heist.Demo`** y despues **`Movie.Test`**. El primero coloca tres zonas y tres pickups temporales cerca del inicio. El segundo usa 10 segundos de preparacion y 30 de grabacion. Para una prueba corta: **`Movie.Test 5 12`**. Ningun comando guarda el mapa.
3. Antes de cada toma aparece el vestuario existente y, a la izquierda, los cinco objetivos del rol. Ready o el timeout inician la grabacion como antes.
4. **Robber:** entra en Bank.Entry, recoge la bolsa con **E**, pulsa **1** para Dance mientras la llevas y **Z** para EvilLaugh. Visita Bank.Escape despues de recogerla. Permanece un rato en el banco para que las otras actuaciones puedan acercarse a ti durante sus primeros segundos.
5. **Police Officer:** entra en el banco, acercate a menos de 200 unidades del replay Robber y pulsa **5** para Arrest. Usa el donut con E y pulsa **V** para PoliceSiren. Arrest no detiene ni cambia al replay.
6. **Customer:** visita Bank.Counter, acercate al replay Police Officer, pulsa **3** para Scared cerca del Robber, **1** para Dance cerca de cualquiera de los dos y **X** para Scream.
7. Al terminar la premiere se muestran los tres puntajes, promedio, estrellas y premios en Results. **R** o Record New Movie reinician toda la pelicula y sus resultados.

**E prioriza recoger el pickup disponible mas cercano; si no hay ninguno a distancia, usa el prop equipado.** La distancia de recogida predeterminada es 180 unidades. Si tienes el donut y E recoge otro objeto, recoge de nuevo un donut y alejate de los demas pickups antes de usarlo, o llama `UseEquippedProp` desde tu propio boton/input. El prompt del HUD indica que hara E. Police Officer empieza con un donut en su outfit predeterminado; elegir otro prop en vestuario cambia su equipo inicial.

Teclas existentes: 1 Dance, 2 Point, 3 Scared, 4 Faint, 5 Arrest; Z EvilLaugh, X Scream, C Cry, V PoliceSiren, B Kiss, N Failure. `Bind Acting Keys`, `Bind Interaction Key` y `Bind Restart Key` se pueden desactivar si tu Enhanced Input o HUD llama las funciones correspondientes. No combines `Acting.Test` con `Movie.Test`.

`Heist.Demo` coloca Bank.Entry a +350 X del origen, Bank.Counter a +650 X/-300 Y y Bank.Escape a +1100 X. La bolsa queda cerca de la entrada. Los ejes siguen la orientacion inicial del personaje. Son marcadores geometricos, no un banco artistico. Si ya existe cualquier ObjectiveZone en el mundo, el comando conserva tu configuracion y no agrega otra. No lo uses para completar automaticamente un mapa parcialmente configurado.

## Colocar zonas en tu banco

Estos son cambios que tu realizas y guardas en Unreal Editor; esta implementacion no modifica tu mapa.

1. Deten Play. En Place Actors busca la clase C++ **ObjectiveZone** y arrastra tres instancias. Tambien aparece bajo C++ Classes / MyProject. No necesitas Blueprint derivado.
2. Selecciona la primera y configura **Zone Tag = `Bank.Entry`**. Coloca su centro dentro del vestibulo, en el lado interior de la entrada. Una zona detecta la posicion de la raiz/capsula del jugador: dale altura suficiente para contener su centro, no solo sus pies.
3. En su componente **Bounds**, ajusta **Box Extent** para cubrir el paso. Por ejemplo, X=150, Y=300, Z=150 es un punto de partida, no una medida obligatoria. Rotacion y escala del actor se admiten. No hace falta configurar overlaps ni collision channels.
4. Configura la segunda con **Zone Tag = `Bank.Counter`**, frente al mostrador, y la tercera con **Zone Tag = `Bank.Escape`**, en el exterior o salida elegida.
5. **Show Floor Marker** deja una primitiva visible para probar. Puedes desactivarla cuando tu escenario indique las zonas. Bounds sigue funcionando sin el marcador.

`Zone Tag` es la propiedad de ObjectiveZone, no el nombre del actor en el Outliner. Los objetivos tambien aceptan un Actor Tag igual al Target Tag. Si prefieres referencias directas, asigna **Target Actor** al ObjectiveZone concreto. Se admite un actor propio con un **BoxComponent**; se usa ese volumen. Se comprueba presencia en la zona en cada tick de grabacion y al actuar; usa volumenes suficientemente anchos para que el jugador no los atraviese enteros entre frames.

## Colocar y configurar props

1. Arrastra las clases nativas **MoneyBagProp**, **DonutProp** y **BananaProp** al nivel. Coloca la bolsa dentro del banco, el donut en una ubicacion accesible al policia y la banana donde quieras permitir recogerla. No llevan simulacion fisica ni bloquean al jugador.
2. Conserva sus **Prop Id** predeterminados: `Prop.MoneyBag`, `Prop.Donut`, `Prop.Banana`. Cambiar el nombre del actor en el Outliner no cambia estos IDs.
3. **Pickup Id** identifica esa instancia en los eventos. Dejarlo en None genera uno unico al comenzar Play y lo conserva en todas las tomas/restarts de esa sesion. Si lo asignas manualmente, usa un valor distinto por instancia, por ejemplo `Bank.MoneyPickup.01`.
4. Asigna el mesh visible en el mundo en **Pickup Mesh > Static Mesh**, y sus materiales/transform como cualquier componente. **Interaction Distance** vale 180 por defecto; se aplica el menor limite entre el pickup y PropInteractionComponent del personaje.
5. Para el mesh que se ve en la mano, edita la entrada con ese Prop Id en **MovieManager > Cosmetic Catalog**: Static Mesh o Skeletal Mesh, Socket Name, Relative Transform, Actor Relative Transform, materiales y tinte. El socket predeterminado es `hand_r`. Se reutilizan rig y wardrobe existentes.
6. Con **Use Performer Catalog Visual** activado, el pickup usa esa definicion del catalogo del personaje. Para un objeto que tenga un aspecto equipado distinto, desactivalo y configura **Equipped Visual** en la propia instancia. El aspecto del objeto en el suelo y el de la mano son configuraciones independientes.

MovieManager agrega PropInteractionComponent al Pawn si falta. Durante Preparation no interactua ni registra eventos. Antes de cada toma, premiere y restart restaura los pickups a su transform y visibilidad iniciales. Para objetos creados o recolocados por Blueprint despues de BeginPlay, llama **Capture Initial State** tras colocarlos para actualizar ese punto de restauracion.

La recogida equipa una copia visual adjunta al personaje y oculta su pickup durante la toma viva. Su evento `CollectProp / Collect` guarda PropId, PickupId, posicion, instante y una copia de assets/settings. El replay equipa su propia copia en ese instante; durante otras tomas no consume el pickup del jugador actual. En premiere se oculta visualmente el pickup correspondiente al reproducir su recogida. No hay inventario global ni objetos fisicos compartidos.

## Uso del donut y assets de actuacion

`UseEquippedProp` emite un unico evento `UseProp`, con ID `Use.Donut`, `Use.MoneyBag` o `Use.Banana` segun el prop; puedes pasar un Action Id personalizado. El uso no consume el prop y puede repetirse para actuar; un objetivo ya completado no vuelve a dar puntos.

El evento de uso y la puntuacion funcionan sin animacion asignada. Para darle una presentacion propia:

1. En **ActingComponent > Gestures**, agrega una definicion con **Identifier = `EatDonut`**, duracion, rate y una animacion in-place o montage compatible con el skeleton actual.
2. En la definicion `Prop.Donut` del Cosmetic Catalog, asigna **Use Gesture Id = `EatDonut`**. Si usas Equipped Visual propio, configuralo ahi.
3. Usa el prop durante Recording. Se presenta ese gesto, pero solo se graba el evento de uso: no se crea otro evento de gesto ni se concede otra recompensa. La configuracion capturada permite reproducirlo en el actor correcto.

Conserva los IDs de actuaciones existentes `Dance`, `Arrest`, `Scared`, `EvilLaugh`, `PoliceSiren`, `Scream` al asignar los assets finales. Si falta un asset, el evento logico cuenta y se expone `OnPresentationIssue`; no hay animacion/sonido final inventado. Montages necesitan el Slot apropiado en tu Anim Blueprint, como explica [ActingEvents.md](ActingEvents.md). Evita root motion; la trayectoria pertenece al recorder.

## Definiciones por rol

En **MovieManager > Roles > [rol] > Objectives** estan las definiciones editables. Se usa el enfoque de configuracion existente en el proyecto; no hace falta crear DataTables, DataAssets ni dependencias nuevas. Se copian al iniciar la pelicula, asi que reinicia la sesion despues de editarlas.

| Campo | Funcion |
| --- | --- |
| Id | FName estable y unico dentro de ese rol. Usa prefijos por rol para evitar ambiguedad |
| Display Name / Description | Textos expuestos al HUD y a tus widgets |
| Type | ReachZone, CollectProp, PerformGesture, PlaySound, ReachActor, ActionNearActor o UseProp |
| Target Actor / Target Tag | Referencia directa o tag de zona/actor/pickup, segun el tipo |
| Target Role Id | Rol estable del replay, por ejemplo `Robber` o `PoliceOfficer`; no el nombre de su actor |
| Any Recorded Role | Permite cualquiera de los replays para un objetivo de proximidad |
| Required Event Id | ID de gesto, sonido o interaccion. None acepta cualquiera del tipo correspondiente |
| Required Event Type | Para UseProp: Gesture para actuar sosteniendo el objeto, o UseProp para una interaccion explicita |
| Required Prop Id | Prop necesario para CollectProp o UseProp |
| Points | 0-100 por definicion; el total concedido al rol se limita a 100 |
| Prerequisite Id | ID de otro objetivo de ese rol; None no exige previo. No se admiten ciclos ni referencias inexistentes |
| Max Interaction Distance | Proximidad 3D en unidades UE; 0 usa 200 para ReachActor/ActionNearActor. Para CollectProp puede imponer una distancia adicional |

Para ReachActor/ActionNearActor se resuelve primero Target Actor, luego Target Role Id, despues Any Recorded Role o Target Tag. Los replays se consultan en su trayectoria grabada al instante de la accion, evitando depender de si su tick visual ya ocurrio. Coloca el **MovieManager**, su origen/Initial Transform de los roles y las zonas de modo que las trayectorias sean accesibles al personaje. No coloques actores Robber/Police duplicados como objetivos: el gestor crea los replays de las tomas previas, aunque hayan puntuado cero.

Los IDs de rol predeterminados son `Robber`, `PoliceOfficer`, `Customer`. Si cambias un Role Id, actualiza los Target Role Id de tus definiciones que lo referencien. Para Blueprints antiguos con arrays de objetivos vacios, **Use Default Heist Objectives If Empty** genera el guion de abajo usando los IDs de los dos primeros roles. Desactiva esa opcion si deseas un rol deliberadamente sin objetivos.

| Rol | Objetivos predeterminados | Total |
| --- | --- | --- |
| Robber | Entrar 25; recoger bolsa 25; escapar 25; Dance con bolsa 15; EvilLaugh 10 | 100 |
| Police Officer | Entrar 25; acercarse al Robber a 200 unidades 25; Arrest a 200 unidades 25; Use.Donut 15; PoliceSiren 10 | 100 |
| Customer | Mostrador 25; acercarse al Police Officer a 200 unidades 25; Scared cerca del Robber 25; Dance cerca de otro replay 15; Scream 10 | 100 |

Prerequisitos: recoger bolsa exige entrar; escapar y Dance con bolsa exigen recogerla. En Police, acercarse exige entrar, y Arrest exige acercarse. Se validan al ocurrir la accion: un gesto hecho antes de recoger la bolsa o fuera de distancia no queda pendiente para conceder puntos despues. Los demas objetivos no requieren que otro rol haya completado nada. Si faltan zonas o targets se obtienen cero puntos en esos objetivos y la pelicula continua. Una lista con IDs duplicados/prerequisitos invalidos muestra Configuration Issue y se omite, sin bloquear las tres tomas.

## HUD nativo y personalizacion en UMG

**Show Objective HUD** esta activado y **Objective HUD Class** usa `ObjectiveHUDWidget`. Aparece durante vestuario y grabacion, con toma/rol, objetivos, bloqueos, completados, puntos y score; durante grabacion muestra tiempo restante y prompt E. Cada logro muestra `+puntos` durante dos segundos. Desde fase 6 se retira al entrar en premiere; los tres scores y el promedio aparecen en MoviePresentationWidget al terminar la pelicula. No captura teclado/raton y no cambia el input del wardrobe.

Para disenar tu interfaz:

1. Crea un Widget Blueprint y selecciona **ObjectiveHUDWidget** como clase padre (o usa Class Settings > Parent Class). Asignalo a **MovieManager > Objective HUD Class**.
2. Si dejas el Designer sin raiz, se conserva el layout nativo y puedes cambiar **Panel Color**, **Complete Color** y **Panel Width** en Class Defaults.
3. Si agregas tu propia raiz Canvas/Overlay, el codigo conserva tu jerarquia. Implementa **On Objective View Changed** en Graph para actualizar tus TextBlocks y filas. El gestor conecta automaticamente la instancia: no necesitas construirla ni llamar BindMovie desde Blueprint.
4. El parametro Data expone RoleName, TakeNumber, RemainingSeconds, Preparation, Score, Objectives, CompletionFeedback, InteractionPrompt y MovieScore. Para cada objetivo usa Definition.DisplayName/Description/Points, Completed, PointsAwarded y CompletedAtSeconds. Muestra el resultado cuando MovieScore.RoleScores tenga tres entradas; usa TakeNumber para tus paneles de las tomas.
5. Para un HUD completamente propio, desactiva Show Objective HUD y consulta **MovieManager.ObjectiveManager**: Objectives, CurrentScore, IsObjectiveCompleted, IsPrerequisiteSatisfied y GetResult. Sus delegates **OnObjectiveCompleted** y **OnScoreChanged** permiten feedback sin polling. No cambies estados de completado desde widgets.

## Resultados y reinicio

Cada **FMoviePerformance.ObjectiveResult** conserva Score y CompletedObjectiveIds. `GetScoreForRole(RoleId)` consulta el rol actual o una performance guardada. Tras la tercera toma, **ScoreSummary.RoleScores** y **FinalMovieScore** estan disponibles en Blueprint, y se emite **OnMovieScoreCalculated** antes de comenzar la premiere.

`FinalMovieScore = (score0 + score1 + score2) / 3.0`: conserva fracciones y siempre incluye los tres roles, incluso con cero. Los replays nunca escuchan el evaluador de recompensas; solo el Pawn controlado durante Recording puede puntuar. Las recompensas son unicas por objetivo/toma. RestartMovie y StopMovie limpian resultados, eventos e inventario de la sesion y restauran props; los defaults configurados permanecen.

Para compilar y repetir las pruebas desde PowerShell con Unreal Editor cerrado:

```powershell
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Editor
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-Movie.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-HeistPlay.ps1' -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-WardrobePlay.ps1' -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Game
```

Los procesos de prueba no guardan el mapa. La prueba Heist crea fixtures temporales, mueve el Pawn de prueba y pulsa las APIs de actuacion para comprobar el flujo; no reemplaza una partida manual ni la revision artistica de animaciones y sonido. Usa RenderScreenshots para verificar la UI: con NullRHI no se actualiza el tick visual de UMG y la prueba de countdown visible del vestuario no es valida. Resultados reales y archivos cambiados: [Phase5Verification.md](Phase5Verification.md).
