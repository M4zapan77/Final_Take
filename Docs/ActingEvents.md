# ONE MAN STUDIO - Gestos y sonidos (fase 3)

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6.

Actualizacion de fase 6: se conservan estas actuaciones y el vestuario de [Wardrobe.md](Wardrobe.md). CollectProp/UseProp se configuran en [ObjectivesAndScoring.md](ObjectivesAndScoring.md). El titulo de premiere mantiene el reloj detenido; OnPremiereStarted marca el playback real, como explica [PremiereAndResults.md](PremiereAndResults.md). Test-Movie exige 36 pruebas, incluidos los cinco casos de ActingEvents. Los resultados historicos de fase 3 se conservan abajo.

## Implementacion

`UActingComponent` ejecuta acciones del jugador y presenta las acciones de cada replay. `FActingRecording` conserva las muestras de movimiento existentes y agrega `Events`: tiempo en segundos, tipo (`Gesture`, `Sound`, `StopGesture`) e identificador. La toma tambien copia las referencias de animacion/audio y el skeletal mesh con su offset respecto al actor. Cambiar la configuracion de otro rol no cambia las tomas anteriores.

El recorder conserva su muestreo de transforms a 20 Hz. Los eventos usan el instante exacto del reloj de grabacion, sin redondearlos a esa rejilla. MovieManager usa el mismo reloj de escena para movimiento, eventos y replays. Cada replay recorre su propio array una sola vez por reproduccion, conserva el orden de eventos simultaneos y procesa todos los eventos vencidos despues de un frame largo.

Preparar una toma solo coloca a los actores y limpia sus indices; no ejecuta eventos de tiempo cero. Comenzar la grabacion o la premiere inicia una nueva pasada de eventos. Los sonidos pertenecen al actor que los genero y se detienen al terminar su clip, preparar otra toma, reiniciar, detener la pelicula o destruir el actor. Los replays siguen sin colision, overlaps, fisica ni efectos de navegacion. No pueden grabar ni entrar al camino de input vivo.

## Configurar tus assets en Unreal Editor

1. Abre el proyecto activo despues de compilar, abre el Blueprint del Character/Pawn que ya controla el jugador y agrega **Acting Component** en Components. Compila y guarda ese Blueprint tu mismo. MovieManager agrega el componente temporalmente si falta; para conservar tus referencias debes agregarlo en el Blueprint.
2. Selecciona Acting Component y despliega **Gestures**. Ya incluye cinco entradas: `Dance`, `Point`, `Scared`, `Faint`, `Arrest`. Conserva esos identificadores para las funciones enum y las teclas de prueba. Puedes agregar otros IDs para las funciones `By Id`.
3. En cada entrada asigna **Montage** (preferido para el personaje vivo), o **Animation** con una secuencia compatible con su skeleton. Si asignas ambos, Montage tiene prioridad. No hay assets de gestos finales asignados por C++.
4. Para el personaje vivo con Animation Blueprint, coloca un nodo **Slot** entre la salida de locomocion y **Output Pose**. Su nombre debe coincidir con el slot del montage; para secuencias sueltas se usa **Slot Name**, por defecto `DefaultSlot`. Un montage necesita sus propios slots correctamente conectados. Esta edicion del Anim Blueprint queda en tus manos.
5. Usa animaciones **in-place**. Ajusta **Play Rate**, **Duration Seconds** (limite maximo), **Loop** y **Blend In Seconds**. Dance viene con Loop activado y un limite de 3 segundos; aumenta Duration Seconds si quieres un baile mas largo. Un gesto nuevo sustituye al anterior; no bloquea caminar. Stop Gesture cancela explicitamente y registra esa cancelacion.
6. Despliega **Sounds** y asigna un Sound Wave o Sound Cue a cada una de las seis entradas: `EvilLaugh`, `Scream`, `Cry`, `PoliceSiren`, `Kiss`, `Failure`. Ajusta volumen, pitch, atenuacion opcional y **Max Duration Seconds** (5 por defecto, tambien limita sirenas en loop). La atenuacion queda a tu eleccion; sin ella los sonidos pueden oirse como audio 2D. Permite suficientes voces/concurrencia en los assets para tres actores a la vez.
7. **Target Mesh** puede quedarse vacio en un Character: se usa Character.Mesh. En un Pawn con varias mallas asigna la malla que debe actuar. El replay recibe una copia de la referencia al skeletal mesh y del transform relativo al actor al iniciar la grabacion.
8. Compila el Blueprint. Conserva el GameMode y MovieManager de la fase 2; no necesitas modificar mapas ni input assets para las teclas de prueba.

Mientras un gesto esta activo se usa `IgnoreRootMotion`, y al terminar se restaura el modo previo del AnimInstance vivo. El replay evalua su animacion por tiempo absoluto mediante un Single Node Instance, sin copiar el Anim Blueprint del jugador (ReplayActor no es Pawn). Al salir del gesto, el jugador vuelve a su Anim Blueprint; el replay vuelve a la pose de referencia. La locomocion animada completa no se graba en esta fase.

Para una reproduccion predecible, usa montages lineales de un slot y sin secciones que salten mediante Blueprint. No uses notifies de audio/gameplay para los sonidos que quieres grabar con el soundboard: dispara esos sonidos explicitamente por ActingComponent. Los replays no ejecutan notifies al posicionar la animacion. No hay extraccion de eventos de los Anim Blueprints ni grabacion de ramas arbitrarias de montages.

Referencia del motor: [Root Motion y modos de extraccion](https://dev.epicgames.com/documentation/en-us/unreal-engine/root-motion-in-unreal-engine).

## Prueba del bucle completo

1. Inicia **Play**, un jugador, con tu personaje habitual. No uses `Acting.Test` ni el GameMode de prueba de fase 1 junto con MovieManager.
2. Si ya tienes MovieManager en el mapa, activa **Auto Start** o llama **Start Movie**. Para una prueba temporal sin guardar el mapa, usa la consola **`Movie.Test 2 10`**: 2 segundos de preparacion y 10 de grabacion por rol. Para el ritmo final usa `Movie.Test` (10/30).
3. MovieManager tiene **Bind Acting Keys** activado por defecto. Durante Recording usa las teclas de la fila superior y estas letras:

| Tecla | Accion |
| --- | --- |
| 1 / 2 / 3 / 4 / 5 | Dance / Point / Scared / Faint / Arrest |
| Z / X / C / V / B / N | Evil laugh / Scream / Cry / Police siren / Kiss / Failure |
| R | Reiniciar toda la pelicula (Bind Restart Key) |

4. Toma 1: camina, pulsa **1** cerca del segundo 2 y **Z** cerca del segundo 4. En preparacion no deben aceptarse acciones.
5. Toma 2: el primer actor repite su baile y risa en esos mismos instantes mientras tu caminas y grabas, por ejemplo, **2** y **X**. Durante preparacion el primer actor permanece en su inicio sin ejecutar sonidos.
6. Toma 3: revisa ambos replays y graba **3** y **C**. En la premiere aparecen las tres actuaciones con movimiento, gestos y sonidos independientes desde cero, vistos por la camara existente.
7. Pulsa R durante grabacion o premiere. Deben detenerse los sonidos anteriores, borrarse los eventos y regresar a preparacion de la toma 1. Completa una toma sin acciones: ningun gesto/sonido de la pelicula anterior debe reaparecer.

Sin assets asignados se aceptan y registran los eventos y se actualiza el HUD de depuracion, pero no hay baile visible ni sonido audible. `OnPresentationIssue` informa que referencia falta. Para verificar la presentacion debes asignar al menos una animacion compatible y un sonido. La camara debe encuadrar las rutas igual que en la fase 2.

## Conectar el HUD con Blueprints

Obtiene **Live Acting Component** desde MovieManager una vez iniciada la sesion, o usa Get Component By Class en el Pawn si lo agregaste al Blueprint. Los botones llaman directamente al componente; no necesitan registrar un evento adicional en el recorder.

| Interfaz de ActingComponent | Uso |
| --- | --- |
| Trigger Gesture(enum) / Trigger Gesture By Id | Disparar gesto y, cuando hay grabacion activa, agregar un evento |
| Trigger Sound(enum) / Trigger Sound By Id | Soundboard y registro automatico |
| Stop Gesture | Registrar una cancelacion del gesto actual |
| Can Trigger Actions | Habilitar botones: depende de la fase y de la grabacion activa |
| Active Gesture Id / Last Triggered Event | Presentacion actual y ultimo evento |
| On Acting Event Triggered(Event, From Replay) | Actualizar botones/indicadores; From Replay distingue a los actores grabados |
| On Gesture Ended(Id) | Quitar el indicador de gesto al terminar o sustituirse |
| On Actions Reset | Limpiar indicadores al detener medios/reiniciar |
| On Live Actions Enabled Changed | Refrescar botones al cambiar de fase; consulta tambien Can Trigger Actions |
| On Presentation Issue(Id, Message) | Mostrar o registrar una referencia que falta |

Las funciones Trigger devuelven false si estan bloqueadas o el identificador no existe. Un asset sin asignar devuelve true y graba el evento logico; emite On Presentation Issue para completar el setup. No reproduzcas otra vez el audio dentro de On Acting Event Triggered: el componente ya lo hace. No llames Record Acting Event desde los botones ademas de Trigger.

Desactiva **Bind Acting Keys** cuando Enhanced Input o tus botones manejen estas acciones. Desactiva **Bind Restart Key** si el HUD ya llama Restart Movie. Cada ReplayActor expone su propio **Acting** para indicadores sobre ese actor. No habilites input ni agregues recorders a los replays. MovieManager controla la disponibilidad y solo permite acciones vivas durante Recording.

Fuera de MovieManager, agrega recorder y ActingComponent al mismo actor. Para compartir timeline: Stop Clock, Start Recording(MaxDuration, Use Shared Scene Clock=true), Start Clock(0), y Play Recording para todos los replays en ese frame. Para ensayar sin grabar puedes desactivar Require Active Recording; MovieManager fuerza ese requisito durante su sesion y restaura el valor al detenerse.

## Compilacion y verificacion

Con Unreal Editor cerrado, desde `C:\Games\OneManStudio\MyProject`:

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Test-Movie.ps1
.\Scripts\Build.ps1 -Target Game
```

Test-Movie valida un reporte nuevo y exige las 9 pruebas anteriores y las 5 nuevas de ActingEvents: DataValidation, CaptureAndGating, PlaybackTimeline, ThreeTakeMovieAndRestart y AnimationPresentation. La prueba de animacion carga dos assets existentes del template solo para lectura; no los modifica ni guarda. Usa un montage transitorio para comprobar tambien ese camino.

Reporte: `Saved\Automation\Movie\index.json`. Log: `Saved\Logs\MovieTests.log`. Los resultados reales de esta entrega se registran en `Phase3Verification.md`.

El disparo de eventos ocurre en el primer frame que alcanza el timestamp; no es audio sample-accurate. Tras un hitch se conservan todos los eventos logicos, se posiciona la animacion segun la edad del evento y se omiten sonidos ya vencidos. El audio se reproduce en tiempo de audio normal, con pitch configurable; el cambio de dilatacion temporal durante un sonido no lo estira automaticamente. Las pruebas sin sonido/renderizado verifican datos, scheduling y evaluacion de animacion; la escucha, el slot final, las mezclas y el aspecto de tus assets se verifican manualmente en Play.
