# ONE MAN STUDIO: grabacion y reproduccion

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6.0.

Se agrega el modulo C++ `MyProject` a tu proyecto Blueprint existente. Tus mapas, Blueprints, assets, controles Enhanced Input y GameMode de Third Person se conservan. Los sistemas nuevos viven en `Source/MyProject`; no se escriben archivos en `Content` ni `Config`.

## Compilacion y estado real

El 12 de noviembre de 2026 se detectaron Visual Studio 2022 17.14, MSVC 14.44.35230, Windows SDK 10.0.22621.0 / 10.0.26100.0 y SDK de .NET Framework 4.8. UE advierte que prefiere MSVC 14.38, pero acepta el compilador instalado.

- **Windows Development (Game): compilacion y enlace correctos en el proyecto real**, con `MyProject Win64 Development` en 105.24 segundos. Incluye generacion de codigo con UnrealHeaderTool y compilacion del modulo nativo. Log: `Saved/Logs/Build-MyProject.log`.
- **Development Editor: compilacion y enlace correctos**. La compilacion inicial termino en 97.87 segundos; la recompilacion de las correcciones de pruebas tambien paso. Existe `Binaries/Win64/UnrealEditor-MyProject.dll`. Log: `Saved/Logs/Build-MyProjectEditor.log`.
- **Automatizacion: 4 de 4 pruebas correctas, sin fallos ni advertencias**. Reporte: `Saved/Automation/Recording/index.json`.
- **Play en el mapa existente: verificados** el final automatico a los 10 segundos, las 201 muestras, el cambio a un Pawn nuevo en el inicio y la aparicion del replay. El log registra varias tomas completas. La verificacion interactiva de movimiento continuo y controles del segundo personaje queda para el usuario: el control automatico de la ventana se detuvo al detectar Escape fisico.

El bloqueo de `SwarmInterface` quedo resuelto al instalar **.NET Framework 4.8 SDK**. El runtime de .NET y el SDK de .NET 8 que incluye Unreal son componentes diferentes; no sustituyen ese SDK.

Con Unreal Editor cerrado, ejecuta desde la carpeta de este proyecto:

```powershell
.\Scripts\Build.ps1 -GenerateProjectFiles
.\Scripts\Build.ps1
```

El destino predeterminado es **MyProjectEditor / Win64 / Development**. Para verificar solo la compilacion del ejecutable Windows:

```powershell
.\Scripts\Build.ps1 -Target Game
```

La compilacion Game no prepara los assets para distribucion; usa el Editor para esta prueba. Los scripts aceptan `-EngineRoot 'D:\Ruta\UE_5.6'`. No se agregan dependencias externas: el modulo usa Core, CoreUObject, Engine e InputCore.

## Probar sin editar tu mapa ni tus Blueprints

1. Despues de compilar el destino Editor, abre `MyProject.uproject`. Conserva tu mapa `Lvl_ThirdPerson` y tu `BP_ThirdPersonGameMode` actuales.
2. Configura Play con **un jugador**, modo de red **Standalone**, y entra con **Play**, no Simulate. Haz clic en el viewport para capturar el teclado.
3. Abre la consola del juego, escribe **`Acting.Test`**, pulsa Enter y cierra la consola. Otra forma verificada es liberar el cursor con **Mayus+F1**, escribir el comando en el campo **Cmd** de la barra inferior del Editor y volver a hacer clic en el viewport. Este comando crea unicamente un actor temporal en el mundo que esta jugando; no modifica el mapa guardado. Tambien puedes colocar `ActingRecorderTestActor` en un mapa de prueba si prefieres hacerlo manualmente.
4. Presiona **R** para comenzar una toma de **10 segundos**. Muevete y gira con los controles normales de Third Person. El actor de prueba agrega el recorder a la instancia del personaje si todavia no lo tiene.
5. Al finalizar, el sistema crea una **segunda instancia** de la misma clase de Pawn en el transform inicial y el PlayerController pasa a controlarla. La primera instancia se destruye en el mundo de ejecucion, despues de conservar sus datos. Se restaura la rotacion inicial de la camara y la nueva instancia empieza sin velocidad de movimiento.
6. La primera interpretacion empieza a reproducirse como un actor visual, mientras controlas al segundo personaje libremente. Camina a traves del replay: no debe bloquearte, empujarte ni producir overlaps. El replay utiliza por ahora una esfera alargada con un marcador que indica su orientacion.
7. Al terminar, el replay conserva su ultima pose. **P** repite la misma toma. **R** comienza otra prueba desde tu posicion actual. **Enter**, durante una grabacion, termina antes de los 10 segundos y realiza la misma transicion.

Resultado esperado para una toma completa a 20 Hz: **201 muestras**, desde `0.00` hasta `10.00`, separadas por `0.05` segundos. La informacion aparece en pantalla y en Output Log bajo `LogActingRecorderTest`.

El harness usa y reemplaza solo instancias de Pawn en ejecucion. Su propiedad opcional **Second Pawn Class** permite elegir otra clase; si esta vacia, usa la misma clase que la primera toma. `ActingRecorderTestGameMode` ofrece un personaje nativo de prueba opcional; no se cambia tu GameMode predeterminado. El comando de consola no se registra en Shipping.

## Agregar el componente a tu personaje

Cuando quieras integrarlo en tu Blueprint, abre el personaje y selecciona **Add Component -> Acting Recorder**. Esa modificacion del asset queda en tus manos.

- **Sample Rate Hz = 20** es el valor predeterminado. El valor se fija al iniciar cada toma.
- **Start Recording(10)** inicia una toma nueva y la termina automaticamente a los 10 segundos. `0` permite detenerla manualmente con **Stop Recording**.
- **On Recording Finished** entrega el struct `ActingRecording`. Conserva ese valor antes de destruir al personaje. **Get Recording** tambien entrega una copia; evita llamarlo cada Tick.
- **Stop Recording** captura la pose y el tiempo finales, aunque no coincidan con la cuadricula de 50 ms, y dispara el mismo evento. Iniciar una toma mientras otra sigue activa devuelve false.

## Reproducir desde Blueprint

1. Conserva los datos recibidos de **On Recording Finished** y crea un **ReplayActor**, o un Blueprint visual derivado de esa clase.
2. Obtiene **Get World Subsystem -> ActingSceneClockSubsystem** y llama **Start Clock(0)** una sola vez para toda la escena.
3. Llama **Play Recording(Recording, Scene Start Time Seconds = 0)** en cada replay. Todos deben usar el mismo tiempo de inicio del reloj compartido. Play Recording no reinicia el reloj por su cuenta.

La reproduccion interpola posicion y escala linealmente, y rotacion por el arco corto entre cuaterniones. Antes del inicio programado conserva la primera pose; al terminar conserva la ultima y dispara **On Playback Finished** una sola vez. Para repetir una toma terminada vuelve a llamar Play Recording.

**Pause Clock / Resume Clock** pausan o reanudan el grupo de replays mientras el jugador puede seguir moviendose. La pausa global y la dilatacion temporal de Unreal afectan al tiempo del mundo que usan ambos sistemas. **Stop Playback** actua sobre un replay; **Stop Clock** detiene y reinicia el reloj de la escena. El harness de prueba controla este reloj durante su demostracion.

El replay desactiva colisiones, respuestas, overlaps, simulacion fisica y efectos sobre navegacion, incluyendo componentes de Child Actors. Si agregas componentes visuales nuevos durante playback, llama **Disable Physical Interaction** despues de agregarlos. Conserva las mallas bajo la raiz movil del actor, con los offsets locales adecuados.

## Archivos y limites de este hito

| Clase / archivo | Funcion |
| --- | --- |
| `ActingRecording.h/.cpp` | Structs reutilizables y expuestos a Blueprint; validacion e interpolacion |
| `ActingRecorderComponent` | Muestreo del transform del propietario despues de movimiento/fisica, a 20 Hz |
| `ActingSceneClockSubsystem` | Reloj comun por mundo, derivado del tiempo de Unreal |
| `ReplayActor` | Reproduccion visual interpolada y sin interferencia fisica |
| `Testing/ActingRecorderTest*` | Harness temporal, personaje placeholder y GameMode opcional |
| `Private/Tests/ActingRecordingTests.cpp` | Cuatro pruebas de automatizacion para datos, muestreo, sincronizacion y colisiones |

Los datos permanecen en memoria; no se crean assets ni archivos de guardado. Se capturan posicion, rotacion y escala en coordenadas del mundo. No se graban animacion, camara, inputs, audio ni eventos. El escenario no se rebobina. Si un frame abarca varias muestras, se estiman los puntos intermedios entre las dos poses observadas; no se puede recuperar una curva o teletransporte que no se observo.

No se implementan vestuario, objetivos, puntuacion, montaje de pelicula ni otros sistemas antes de verificar este hito.

## Automatizacion despues de compilar Editor

```powershell
.\Scripts\Test-Recording.ps1
```

Tambien puedes buscar **OneManStudio.Acting** en Test Automation / Session Frontend y ejecutar `RecordingData`, `RecorderSampling`, `SharedClockAndReplay` y `ReplayCollision`. Los reportes se escriben en `Saved/Automation/Recording` y el log en `Saved/Logs/RecordingTests.log`.

Las cuatro pruebas pasaron. El entorno de pruebas avanza un numero de frame distinto en cada tick simulado y finaliza correctamente sus mundos transitorios. El script valida un reporte nuevo y devuelve error si una prueba falla o falta, aunque Unreal termine con codigo cero. La verificacion interactiva de controles y movimiento continuo indicada arriba sigue siendo necesaria.
