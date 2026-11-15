# ONE MAN STUDIO - MovieManager

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, UE 5.6.

Actualizacion de fase 6: la premiere agrega titulo/transicion, oculta el HUD de gameplay y termina en resultados con estrellas, premios, Replay y New Movie. Consulta [PremiereAndResults.md](PremiereAndResults.md) y [Phase6Verification.md](Phase6Verification.md). [ObjectivesAndScoring.md](ObjectivesAndScoring.md), [Wardrobe.md](Wardrobe.md) y [ActingEvents.md](ActingEvents.md) conservan sus instrucciones. Los nueve resultados de abajo describen la fase 2; el script actual exige 36 pruebas.

## Estado de verificacion

- El codigo de fase 2 compilo y enlazo como **MyProjectEditor / Win64 / Development en el proyecto activo**, en 33.22 segundos. La DLL del Editor ya esta actualizada. Log: `Saved/Logs/Build-MyProjectEditor.log`.
- Las **nueve pruebas pasaron en el proyecto activo**: cuatro del recorder/replay y cinco del MovieManager, sin fallos ni advertencias de automatizacion. Incluyen los tiempos completos 10/30 y la reactivacion de CharacterMovement. Reporte: `Saved/Automation/Movie/index.json`.
- El destino **MyProject / Win64 / Development** compilo y enlazo en el proyecto activo en 49.81 segundos.
- Los 250 archivos de Content/Config conservan sus hashes.
- Ya puedes abrir de nuevo `MyProject.uproject` y probar el bucle. La inspeccion visual del encuadre y de los controles con tus Blueprints se hace en Play; las pruebas automaticas se ejecutaron sin renderizado.

`AMovieManager` coordina tres interpretaciones del mismo Pawn controlado. Las tomas anteriores aparecen mediante `AReplayActor`; el recorder, sus muestras de transform y la interpolacion existentes se conservan. `ClearRecording` cancela y descarta datos sin disparar el evento de toma terminada. La fase 3 agrega eventos de actuacion a esos mismos datos.

## Bucle

`Idle -> Preparation 1 -> Recording 1 -> Preparation 2 -> Recording 2 -> Preparation 3 -> Recording 3 -> MoviePremiere -> Finished`

- Cada preparacion dura **10 segundos** y cada grabacion **30 segundos**, configurables por instancia.
- En preparacion el reloj de escena queda detenido en cero. Se colocan el Pawn y los replays en sus inicios y se detienen movimiento y saltos del jugador.
- En Recording 1 solo actua el jugador. En Recording 2 actua junto al replay 1. En Recording 3 actua junto a los replays 1 y 2.
- El recorder y el reloj compartido arrancan en el mismo frame. Todos los replays usan `SceneStartTimeSeconds = 0`.
- La premiere reinicia el reloj en cero y reproduce las tres tomas desde una camara fija. Se oculta el Pawn vivo y se bloquean sus controles de movimiento y mirada.
- Finished conserva las tres poses finales y la camara. No inicia otra pelicula automaticamente.

La configuracion de duraciones y roles se copia al comenzar la pelicula. Los cambios posteriores se aplican al reiniciar.

## Compilar y ejecutar las pruebas

Con Unreal Editor cerrado, desde la carpeta del proyecto:

```powershell
.\Scripts\Build.ps1
.\Scripts\Test-Movie.ps1
```

El primer comando compila Development Editor / Win64. El segundo ejecuta las pruebas de grabacion anteriores y las del gestor, valida un reporte nuevo y devuelve error si alguna prueba falta o falla. No modifica mapas ni Blueprints.

Reporte: `Saved/Automation/Movie/index.json`. Logs: `Saved/Logs/MovieTests.log` y `Saved/Logs/Build-MyProjectEditor.log`.

Pruebas nuevas:

| Prueba | Verifica |
| --- | --- |
| `ConcurrentRecordingAndReplay` | Grabar una trayectoria distinta mientras otra toma sigue reproduciendose, sin mezclar sus datos |
| `ThreeTakeLoop` | Tres roles, poses iniciales, reloj en cero durante preparacion, sincronizacion por frame, camara de premiere y poses finales |
| `DefaultTimingAndFinishedRestart` | Bucle completo con 10/30 segundos, 601 muestras por toma, premiere de 30 segundos y reinicio desde Finished |
| `RestartAndCleanup` | Reinicio durante grabacion y premiere, borrado del buffer y tomas, destruccion de replays/camara propia, restauracion de vista y locks de input sin duplicar la tecla R |
| `CharacterMovementLifecycle` | Character real con suelo: no camina ni salta en preparacion, vuelve a moverse al grabar, su desplazamiento se registra y se restaura su movimiento al detener |

Las duraciones largas se simulan mediante ticks del mundo; no requieren esperar 150 segundos reales en automatizacion. La inspeccion visual del encuadre y de los controles con tus Blueprints se hace con Play.

## Prueba rapida sin guardar cambios en Content

1. Abre `MyProject.uproject` y tu mapa Third Person existente. Conserva su GameMode habitual. Inicia una sesion nueva de **Play**, un jugador, red Standalone. No uses Simulate.
2. No ejecutes `Acting.Test` ni uses el GameMode de prueba de fase 1 en esta sesion: ambos coordinadores intentarian controlar el mismo reloj.
3. Escribe **`Movie.Test 1 3`** en la consola del juego. Otra opcion es **Mayus+F1**, escribirlo en el campo **Cmd** inferior del Editor, Enter, y volver a hacer clic en el viewport.
4. El comando crea un MovieManager temporal y empieza: un segundo de preparacion y tres de grabacion por rol. El texto de depuracion muestra fase, rol, toma y tiempo restante.
5. En cada fase Recording, muevete y gira. Comprueba que en la segunda aparece un replay y en la tercera aparecen dos. Sus rutas vuelven a empezar desde cero en cada toma.
6. Tras la tercera grabacion, la vista cambia a la camara fija y ves los tres replays simultaneamente. Este ejemplo tarda unos **15 segundos** en total: tres fases de 1+3 segundos y una premiere de 3 segundos.
7. Presiona **R** en cualquier fase, incluida la premiere o Finished. Debe borrar las tomas y los replays y regresar a Preparation de la toma 1, con la camara del jugador y reloj cero.

**`Movie.Test`** sin argumentos usa los tiempos normales de 10/30: 120 segundos para las tres interpretaciones y 30 para la premiere, aproximadamente **150 segundos** en total. `Movie.Test 0 2` permite preparacion cero. El comando solo existe fuera de Shipping.

Los cambios de esta prueba ocurren en el mundo de Play; no se guardan actores en el mapa. Un comando repetido reinicia el gestor existente. Detener Play elimina los objetos temporales.

## Colocar MovieManager en tu propio mapa

1. En C++ Classes / MyProject busca **MovieManager** y arrastralo a tu mapa de prueba. Solo debe haber un gestor activo por mundo. Esta colocacion y el guardado del mapa quedan en tus manos.
2. Activa **Auto Start**, o llama **Start Movie** desde tu Blueprint/HUD cuando el jugador este listo. Auto Start espera hasta encontrar un PlayerController con Pawn.
3. Configura **Preparation Duration Seconds** y **Recording Duration Seconds**. Deja los valores 10 y 30 para el bucle normal; usa 1 y 3 para revisar rapidamente.
4. En **Roles** configura exactamente tres identificadores unicos. Por defecto son `Role_1`, `Role_2` y `Role_3`.
5. **Roles -> Initial Transform** es relativo al origen de la pelicula. Con **Use Player Start As Origin** activo, el origen se captura de la pose del Pawn al iniciar. Si lo desactivas, se usa el transform del MovieManager.
6. Los offsets iniciales predeterminados son `(0,0,0)`, `(0,180,0)` y `(0,-180,0)` centimetros. Ajustalos para colocar cada personaje en el escenario. Los transforms se resuelven a coordenadas del mundo y se guardan con cada interpretacion.
7. Opcionalmente coloca una **CameraActor** o CineCameraActor y asignala a **Premiere Camera**. Se usa su encuadre tal cual: el gestor no la mueve ni la destruye. Si no asignas una, crea una camara temporal desde **Automatic Camera Offset**, mirando al centro de los tres inicios.
8. **Replay Actor Class** acepta tu futuro Blueprint visual derivado de ReplayActor. Los placeholders actuales son las mallas basicas del motor. No se crean assets nuevos.
9. **Show Debug Overlay** muestra la informacion de prueba. **Bind Restart Key** asigna R al reinicio; desactivalo cuando tu HUD o sistema de input maneje esa accion.

El mismo Pawn permanece poseido durante las tres tomas; no se destruye ni se reemplaza el personaje del jugador. El rol actual y los eventos permiten que mas adelante conectes vestuario o presentacion de personajes.

## Interfaz para Blueprints

| Funcion / propiedad | Uso |
| --- | --- |
| `StartMovie()` | Inicia un ciclo nuevo; devuelve false si la configuracion no es valida o ya hay una sesion activa |
| `RestartMovie()` | Cancela lo actual, borra todas las tomas y el buffer del recorder, restaura al jugador y comienza otra vez desde Preparation 1 |
| `StopMovie()` | Borra la sesion y regresa a Idle; devuelve al jugador su pose, vista, visibilidad, colision y movimiento originales |
| `State` | Idle, Preparation, Recording, MoviePremiere, Finished o Error |
| `CurrentTakeIndex` / `GetCurrentTakeNumber()` | Indice 0-2 / numero 1-3; fuera de una toma el numero es 0 |
| `GetCurrentRoleId()` | Identificador del rol que se esta preparando o grabando |
| `GetPhaseDurationSeconds()` | Duracion de la fase actual |
| `GetPhaseElapsedSeconds()` / `GetPhaseRemainingSeconds()` | Temporizadores para el HUD, basados en tiempo de juego |
| `GetSceneTimeSeconds()` | Tiempo del reloj compartido; cero durante preparacion y reinicio |
| `Performances` | Array de `FMoviePerformance`: RoleId, InitialTransform mundial y FActingRecording independiente |
| `ActiveReplays` | Replays propiedad del gestor; su indice corresponde a Performances |
| `LivePerformer` | Pawn controlado durante la sesion |
| `GetPremiereCamera()` | Camara que usa la premiere, sea asignada o temporal |
| `LastError` / `HasActiveSession()` | Diagnostico y estado de propiedad de la sesion |

Eventos disponibles:

- **OnStateChanged(PreviousState, NewState)**: cambiar paneles del HUD.
- **OnPreparationStarted(TakeNumber, RoleId, DurationSeconds)**: mostrar preparacion y el rol correspondiente.
- **OnRecordingStarted(TakeNumber, RoleId, DurationSeconds)**: mostrar la senal de accion y el contador.
- **OnTakeRecorded(Performance)**: consultar la interpretacion guardada. Se emite antes de avanzar a la siguiente toma o a la premiere.
- **OnPremiereStarted** y **OnMovieFinished**: presentar la pelicula y su pantalla final.
- **OnMovieReset**: limpiar la presentacion entre Idle y el nuevo inicio.
- **OnMovieError(Message)**: mostrar un fallo de configuracion o perdida del Pawn/recorder. El gestor restaura al jugador y queda en Error.

No llames StartClock, PauseClock, StopRecording ni ClearRecording desde otro coordinador mientras MovieManager esta activo. El gestor es propietario de la linea de tiempo; una cancelacion externa del recorder aborta el ciclo. Las funciones publicas para terminarlo son RestartMovie y StopMovie.

## Alcance y limites

- Datos solo en memoria. No se escriben assets ni archivos de pelicula.
- Se registran transforms y eventos explicitos de gesto/sonido de ActingComponent. No se graban inputs ni el estado completo del Anim Blueprint. No se implementan vestuario, objetivos, puntuacion ni efectos cinematicos avanzados.
- Se reinician el Pawn vivo y los replays administrados; no se rebobina el decorado ni la simulacion de otros actores del nivel.
- La camara permanece fija. Debe encuadrar las rutas completas; si un personaje sale del encuadre, ajusta la camara o los inicios, no hay seguimiento automatico.
- Los replays conservan las protecciones existentes contra colision, overlaps, fisica y navegacion. Si tu Blueprint agrega componentes durante playback, utiliza DisablePhysicalInteraction como en fase 1.
- La pausa global y la dilatacion temporal afectan tanto a grabacion como a reproduccion. No se agrega una pausa independiente del movie loop.
- El gestor esta disenado para un Pawn local, especialmente Character/Pawn con MovementComponent; no agrega soporte de vehiculos ni de cuerpos ragdoll.
