# Videos de arranque y fondo del HUB

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6.1, Windows.

## Secuencia configurada

| Etapa | Archivo suministrado | Copia incluida en el proyecto | Asset |
| --- | --- | --- | --- |
| Estudio | dutix-intro-v2-v1.mp4 | Content/Movies/Startup/StudioIntro.mp4 | /Game/OneManStudio/UI/StartupVideos/MS_StudioIntro |
| Introduccion del juego | gemini_generated_video_7459b8ea.mp4 | Content/Movies/Startup/GameIntro.mp4 | /Game/OneManStudio/UI/StartupVideos/MS_GameIntro |
| Fondo del HUB | Video Menu.mp4 | Content/Movies/Startup/HubLoop.mp4 | /Game/OneManStudio/UI/StartupVideos/MS_HubLoop |

Los originales de Downloads se conservan. Las copias son identicas, sin recodificacion. Se usa Media Framework/WmfMedia de Unreal, sin dependencias externas, controles de video ni conexion a internet.

Al iniciar: estudio > introduccion > HUB. Un clic izquierdo en cualquier parte de cualquiera de las dos intros, incluidas las bandas negras, salta directamente al HUB. El clic y su liberacion se consumen; los botones se habilitan despues de soltar y terminar el fundido. Mantener pulsado no activa Start Game.

El fondo usa el mismo MediaPlayer al abrir/cerrar How to Play, Credits y Audio. Music y Master tambien regulan el audio del video. La musica de menu anterior no se superpone con la pista del video. El fondo se ajusta completo a la pantalla, conservando su proporcion con bandas negras cuando corresponde.

Start Game funde menu, video y audio a negro, cierra el reproductor y carga el nivel jugable configurado. Una capa negra independiente del nivel bloquea entrada durante la carga y se desvanece cuando el nuevo flujo esta listo. En el banco actual aparece el vestuario de Take 1; Ready/timeout devuelve el control de movimiento como antes. Las intros solo aparecen una vez por ejecucion; Return to Main Menu vuelve directamente al fondo del HUB. Los sistemas MovieManager, grabacion, premiere, vestuario y objetivos siguen siendo sus propietarios originales.

## Configurar en Unreal Editor

No es necesario montar un nuevo widget ni cambiar el mapa para probar los defaults: abre el proyecto y usa Play/Standalone Game. Para guardar otras asignaciones:

1. Abre tu Blueprint hijo de **StudioFlowManager**, o crea uno, por ejemplo BP_StudioFlow. En Components selecciona **StartupVideos** y en Details abre **Startup Videos > Settings**.
2. Asigna **Studio Intro**, **Game Introduction** y **Hub Background** a File Media Source assets. Los tres defaults anteriores ya estan creados. Para sustituirlos, copia tus MP4 a `Content/Movies` y crea File Media Source desde Content Browser > Media; elige el archivo local. Mantenerlos dentro de Movies permite distribuirlos sin rutas a Downloads.
3. **Starting Gameplay Level** permite elegir el mapa de juego. El default es `/Game/OneManStudio/Maps/Lvl_Bank`. El mapa debe estar incluido en el cook. Para conservar el flujo de ONE MAN STUDIO usa StudioGameMode o su hijo con el MovieManager configurado.
4. Deja **Video Material** en `M_StartupVideoUI`. Es un material User Interface opaco con el parametro de textura externa **VideoTexture**, asignado por el componente en runtime. `MT_StartupVideo` es la referencia de textura del material; cada sesion usa su propia textura runtime.
5. Ajusta **Fade Seconds** (default 0.3), **Open Timeout Seconds** (8), **Prime Seconds** (0.1), **Hub Dimmer Opacity** (0.25) y **Audio Volume** (1). **Enabled** desactiva toda la presentacion de video y recupera el flujo de menu e introduccion de texto anterior.
6. Coloca una sola instancia de BP_StudioFlow en el mapa de entrada: StudioGameMode la reutiliza. Alternativamente asigna ese hijo a **StudioGameMode > Studio Flow Class** en tu GameMode hijo. Conserva el MovieManager y las referencias de tus personajes/camara.
7. Guarda tus cambios y vuelve a empaquetar. Los ajustes hechos solo a una instancia durante PIE se pierden al terminar la sesion.

Un intro ausente, invalido, rechazado por el decoder o sin progreso tras el timeout avanza a la siguiente etapa. Un fondo ausente o fallido deja el HUB utilizable sobre negro. Un mapa de juego no asignado/no incluido deja el menu abierto y emite **On Media Issue**; no inicia un viaje invalido.

## UI y Blueprints propios

Se conserva StudioFlowWidget, su layout y sus funciones de navegacion. Las capas de video son nativas: fondo en Z=140, menu existente en Z=150 y fundido/bloqueo en Z=200. La capa de carga entre niveles esta en Z=10000. No se llama RemoveAllWidgets ni se guardan cambios sobre tus Widget Blueprints.

Para un StudioFlowWidget con Designer propio, deja transparentes sus fondos donde quieras ver el video. Usa **On Studio View Changed / ViewData.Screen**:

- **StartupVideos**: oculta el contenido del menu.
- **MainMenu, HowToPlay, Credits, AudioSettings**: muestra tu pagina existente. El video sigue independiente.
- **StartingGame**: conserva la pagina del menu visualmente y deshabilita sus botones; la capa superior realiza el fundido completo.
- Los otros estados mantienen el comportamiento anterior.

Usa **Play Movie** para Start Game, y las funciones existentes How To Play, Credits, Audio Settings, Back y Quit. **StudioFlowWidget > Text > Start Game Text** permite cambiar la etiqueta del boton nativo. No crees otro reproductor ni vuelvas a llamar Begin Sequence para cambiar de panel. Para habilitar botones consulta **StartupVideos.IsHubInteractive**. El widget nativo lo hace automaticamente.

El componente expone Settings, Stage, Phase, BlackOpacity, bHasVideoFrame, bSkipped, MediaPlayer/Texture/Sound, y eventos **On Stage Changed**, **On Hub Prepared**, **On Hub Interactive**, **On Gameplay Black** y **On Media Issue**. On Hub Prepared permite preparar el menu bajo negro; On Hub Interactive indica cuando ya se puede interactuar. **Skip Introductions** tambien permite conectar un boton propio. No lo conectes a un clic que se propague al menu.

## Empaquetado y pruebas

Con el Editor cerrado:

```powershell
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Editor
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-Movie.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StartupVideos.ps1' -Scenario Auto
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StartupVideos.ps1' -Scenario SkipStudio
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StartupVideos.ps1' -Scenario SkipGame
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StartupVideos.ps1' -Scenario Failures
& 'C:\Games\OneManStudio\MyProject\Scripts\Package-Windows.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-StartupVideos.ps1' -Scenario Auto -PackageDirectory 'C:\Games\OneManStudio\MyProject\Saved\Packages\StartupVideos'
```

El paquete actual usa `Saved/Packages/StartupVideos`; los paquetes historicos se conservan. Si cambias el mapa de inicio, pasa todos los mapas necesarios a Package-Windows.ps1 **-Maps** o incluyelos en Packaging > List of maps to include. El directorio de assets de startup se cocina explicitamente y los MP4 se distribuyen desde Content/Movies.

El runner de video usa render real, audio habilitado, eventos de puntero Slate y OpenLevel real. Auto comprueba ambas intros; SkipStudio y SkipGame mantienen pulsado el clic sobre la futura zona Start para comprobar que no atraviese; todos navegan opciones y esperan dos bucles antes de entrar al juego. Failures elimina referencias solo en runtime y prueba tambien un MP4 existente con contenido invalido. Los informes y capturas quedan en Saved/Automation/StartupPlay* o StartupPackaged*. El driver esta excluido de Shipping.

Test-StudioFlow.ps1 usa **-StudioSkipStartupVideos** exclusivamente para aislar la regresion del flujo anterior. El juego normal no necesita ese argumento. No ejecutes Create-StartupVideoAssets.py sobre assets que ya existen: el script se niega a sobrescribirlos.

Prueba manual: lanza el juego sin argumentos; observa estudio e intro y el menu; abre Audio/How/Credits y comprueba que el fondo no empieza de nuevo; pulsa Start Game, Ready y muevete. Reinicia el proceso y prueba un clic en cada intro, manteniendolo un segundo antes de soltar. Para probar archivos ausentes cambia las referencias en tu Blueprint de prueba, no borres los assets originales.

Resultados reales y limites: [StartupVideosVerification.md](StartupVideosVerification.md).
