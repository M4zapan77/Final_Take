# Fase 7 - menu, introduccion y juego completo

Proyecto activo: **C:\Games\OneManStudio\MyProject\MyProject.uproject**, UE 5.6.1. El mapa principal sigue siendo **/Game/OneManStudio/Maps/Lvl_Bank**. No se modificaron su archivo, tus Blueprints ni los widgets artisticos existentes para agregar el menu.

## Jugar sin comandos

1. Abre el proyecto y pulsa Play, o ejecuta **Saved/Packages/UnifiedCustomization/Windows/Play-ONE-MAN-STUDIO.bat** en el paquete Windows actual. Los paquetes StartupVideos, Phase7 y BankIntegration se conservan como versiones históricas. El nuevo personaje y el camerino de ocho categorías se configuran en [UnifiedCustomization.md](UnifiedCustomization.md).
2. Se reproducen el logo DUTIX y la introduccion del juego, luego aparece el menu sobre un video en loop. Un clic izquierdo en cualquiera de las intros salta ambas y consume el clic antes de habilitar botones. El fondo continua al abrir **How to Play**, **Credits** y **Audio**.
3. **Start Game** funde a negro y carga el nivel configurado, actualmente el banco, abriendo Take 1. How to Play y el vestuario muestran los controles. Si desactivas StartupVideos > Settings > Enabled, se conserva el flujo anterior Play > introduccion de texto > Choose My Costume. Configuracion de fuentes, fades y nivel: **[StartupVideos.md](StartupVideos.md)**.
4. En cada vestuario tienes **10 segundos**, o puedes pulsar Ready. Luego grabas **30 segundos**. Las actuaciones anteriores se reproducen mientras controlas el siguiente personaje.
5. Tras cada toma se oyen/muestran los avisos del director, con una tarjeta breve para el siguiente actor. Tras la tercera, That's a wrap, tarjeta del titulo, premiere desde la camara existente y resultados.
6. En resultados: **Replay Movie** conserva todo; **Record New Movie** borra la pelicula y abre el vestuario de Take 1; **Return to Main Menu** limpia la sesion y vuelve al fondo del HUB sin repetir las intros. Start Game permite comenzar otra pelicula cargando el nivel inicial configurado.
7. **Quit** cierra el juego. En Play in Editor termina la sesion de juego, no guarda ni cierra tu proyecto.

Controles: WASD/mouse, Space para saltar, E para recoger el prop cercano o usar el equipado, 1-5 para Dance/Point/Scared/Faint/Arrest, Z/X/C/V/B/N para Evil Laugh/Scream/Cry/Siren/Kiss/Failure y R para reiniciar la pelicula. El HUD indica exactamente que hara E: si hay un pickup cercano, E lo recoge; alejate de pickups para usar un prop ya equipado. R borra todas las tomas, no solo la actual.

## Configuracion permanente en el Editor

La clase **StudioGameMode**, ya guardada en el banco, crea el front end. Reutiliza el MovieManager, Character y PlayerController existentes. No hay un nuevo mapa de menu ni un nuevo GameInstance obligatorio. El subsistema de arranque conserva el fundido y los ajustes de audio durante OpenLevel.

Para guardar tus ajustes visuales/audio:

1. Crea un Blueprint hijo de **StudioFlowManager**, por ejemplo BP_StudioFlow, dentro de tu propia carpeta de Content.
2. Coloca **una sola instancia** en Lvl_Bank. StudioGameMode la encuentra y la utiliza en lugar de crear el actor nativo por defecto.
3. En **Settings** edita Game Title, Movie Synopsis, How To Play y Credits. Los roles y sus instrucciones vienen de **MovieManager > Roles**; el titulo de la pelicula viene de **MovieManager > Results Settings > Movie Title**.
4. En **Widget Class** asigna tu hijo UMG de StudioFlowWidget si quieres sustituir el layout del menu. Los widgets existentes de vestuario, objetivos y resultados siguen configurandose en MovieManager.
5. Guarda tu nivel/Blueprint desde el Editor y vuelve a empaquetar. Las referencias a sonidos/texturas/widgets de ese actor se incluiran en el cook.

Alternativa: un Blueprint hijo de StudioGameMode puede asignar **Studio Flow Class**. Configuralo como GameMode Override de tu nivel. **Use Front End = false** permite reutilizar el modo de arranque directo anterior. El GameMode de ThirdPerson y los Blueprints originales no fueron modificados.

## Tiempos y transiciones

| Propiedad | Default / efecto |
| --- | --- |
| MovieManager > Wardrobe Duration Seconds | 10 s, Ready y timeout usan la misma funcion protegida |
| MovieManager > Recording Duration Seconds | 30 s, igual para las tres performances |
| StudioFlowManager > Settings > Page Fade Seconds | 0.2 s para el texto de las paginas/tarjetas |
| Settings > Between Actors Seconds | 2.5 s entre tomas |
| Settings > Wrap Seconds | 2.25 s antes del titulo final |
| Settings > Next Actor Cue Delay | 0.6 s entre Cut y Next actor / That's a wrap |
| MovieManager > Premiere Settings | Transicion de titulo previa, bandas y fade de resultados de fase 6 |

StudioFlowManager aplica los tiempos entre actores al MovieManager antes de iniciar la sesion. MovieManager los copia, al igual que sus tiempos 10/30, al empezar una pelicula. Para probar rapido cambia los tiempos **antes** de Play/Start, no durante una grabacion activa. Las duraciones del flow deben ser finitas y no negativas.

El nuevo **EMovieState::TakeTransition** se agrego al final del enum: los valores anteriores de los Blueprints se conservan. Durante estas tarjetas no hay recording, el reloj compartido vale cero y los replays detienen su media. El mundo sigue funcionando; no se usa Pause Game para menus/vestuario ni para la cuenta atras.

## Director y musica

**Settings > Announcements** contiene cinco entradas con Cue estable, Text, Sound opcional, Duration Seconds y Volume:

- ChooseCostume: Choose your costume!
- Action: Action!
- Cut: Cut!
- NextActor: Next actor!
- Wrap: That's a wrap!

Puedes asignar tus voces/SoundCues sin C++. Duration Seconds limita voz y subtitulo, incluso si el asset esta en loop; cero desactiva su presentacion. Los avisos anteriores se detienen al empezar el siguiente o al abandonar el estado. No dupliques Cue en la lista.

Settings tiene **Menu Music**, **Introduction Music**, **Wardrobe Music**, **Recording Music** opcional y **Button Sound**. Si Introduction Music esta vacio, continua Menu Music. Usa SoundCues/Waves con loop para la musica y clips breves sin loop para los clicks. La musica de wardrobe se detiene al comenzar Action; la de premiere/results sigue en **MovieManager > Premiere Settings**, sin dos bandas sonoras superpuestas.

El menu Audio ofrece Master, Music y Effects / Director. **StudioAudioSubsystem**, creado automaticamente por el GameInstance existente, aplica esos multiplicadores a musica, director, botones y sonidos de las actuaciones vivas/replays. No cambia los volumenes guardados en las performances. Los ajustes sobreviven a Replay, New Movie y Return to Menu durante la misma ejecucion; **no se guardan a disco** al salir.

## UMG editable

1. Crea un Widget Blueprint hijo de **StudioFlowWidget** y asignalo a Widget Class del flow.
2. Sin raiz Designer, usa el layout nativo: Heading Font, Body Font, colores, Background Image, textos de botones y tutorial son editables en Class Defaults.
3. Con una raiz Designer propia, se conserva tu arbol. Implementa **On Studio View Changed** y usa **ViewData**: Screen, Revision, titulos, synopsis, roles/scripts, take actual, remaining seconds, Director Text/Opacity, Page Opacity, niveles de audio y error.
4. Muestra tu menu/paginas en MainMenu, HowToPlay, Credits, AudioSettings y MovieIntroduction; tu tarjeta en NextCharacter. En Wardrobe/Recording/Premiere/Results oculta las capas de menu y deja solo el aviso del director cuando corresponda. Los widgets anteriores siguen mostrando sus respectivas interfaces.
5. En botones usa las funciones heredadas **Play Movie**, **Begin Movie**, **How To Play**, **Credits**, **Audio Settings**, **Back**, **Quit** y **Set Volumes**. No inicies recorder/replays ni manipules el reloj desde UMG.

El flow establece foco, cursor e InputModeUIOnly en sus paginas. Ready devuelve GameOnly; recording restablece movimiento/mirada. La pelicula oculta al Pawn y al HUD y utiliza la camara final existente. Resultados conserva la UI de fase 6. No se llama RemoveAllWidgets ni se destruyen interfaces ajenas.

El HUD de objetivos agrega **Controls**, **Active Gesture** y **Recording** a ViewData; tiene un panel de referencia de todas las teclas y countdown visible, con aviso rojo en los ultimos cinco segundos. Puedes editar **Control Reference**, **Show Control Reference** y el estilo del ObjectiveHUDWidget hijo existente. Con un Designer propio usa esos mismos datos en On Objective View Changed.

## API Blueprint y limpieza

StudioFlowManager expone PlayMovie, BeginIntroducedMovie, ShowHowToPlay, ShowCredits, ShowAudioSettings, BackToMenu, ReturnToMainMenu, QuitGame, SetAudioLevels, GetView e IsFrontEndOpen. Sus eventos son **OnScreenChanged** y **OnDirectorAnnouncement**. StudioAudioSubsystem expone GetLevels, SetLevels y OnAudioChanged.

MovieManager sigue siendo el propietario de recording, replays, objetivos, scores, outfits y premiere. Replay/New Movie no se reimplementaron. Su funcion ReturnToMainMenu usa el front end activo; Main Menu Map puede permanecer vacio. El viaje a un mapa separado sigue disponible cuando no hay front end activo y asignaste ese mapa, como antes.

Para tu UI propia de salida usa **StudioFlowManager.ReturnToMainMenu**, no solo StopMovie: ademas de limpiar la pelicula, restablece el menu y sus locks/foco/camara. El menu solamente libera los locks de movimiento/mirada que adquirio; conserva los de otros sistemas. Las interfaces anteriores quedan deshabilitadas al cerrarlas y no pueden comenzar otra toma o repetir resultados viejos.

## Pruebas y paquete

Con Unreal Editor cerrado:

```powershell
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Editor
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-Movie.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StudioFlow.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StudioFlow.ps1' -EnableAudio -ReportSuffix Audio
& 'C:\Games\OneManStudio\MyProject\Scripts\Package-Windows.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StudioFlow.ps1' -PackageDirectory 'C:\Games\OneManStudio\MyProject\Saved\Packages\StartupVideos' -UseDefaultTimings -EnableAudio -ResolutionX 1920 -ResolutionY 1080
```

El smoke abre procesos separados y usa los botones UMG/delegates y las teclas normales. No pasa Map URL, GameMode override, ExecCmds, Movie.Test ni StudioSkipMenu para empezar. Ahora usa StudioSkipStartupVideos para aislar la regresion del flujo previo; Test-StartupVideos comprueba el arranque nuevo con puntero y viaje reales. Por defecto acelera los tiempos; UseDefaultTimings conserva 10/30 y tarda varios minutos. EnableAudio inyecta **tonos sinteticos muy bajos, solo en la prueba**, para verificar voces reales y limpieza sin exigir tus assets finales; no se guardan en Content ni son defaults del juego.

Los runners antiguos Heist/Premiere/Wardrobe usan StudioSkipMenu explicitamente para aislar sus pruebas anteriores. Ese bypass no se usa para jugar ni funciona en Shipping. El driver de prueba se excluye de Shipping; menu, audio settings y flujo son runtime.

Resultados reales, builds, archivos y limites: **[Phase7Verification.md](Phase7Verification.md)**.
