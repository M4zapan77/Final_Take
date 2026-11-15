# ONE MAN STUDIO - Premiere, estrellas y resultados

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, UE 5.6. Se conserva la camara final y el mismo recorder/replay de las fases anteriores. No es necesario crear un Widget Blueprint para probar.

Desde fase 7, el arranque normal incluye **Main Menu > Movie Introduction > Wardrobe**, avisos del director y transiciones entre personajes. Setup actual del menu/audio/UMG y launcher Phase7: [MainMenuAndGameFlow.md](MainMenuAndGameFlow.md).

## Probar en Unreal Editor

1. Abre el proyecto activo: el mapa principal es **`/Game/OneManStudio/Maps/Lvl_Bank`**. Pulsa **Play**, elige **Play** en el menu, lee la introduccion y pulsa **Choose My Costume**. No ejecutes `Heist.Demo` en este mapa: ya tiene zonas y props permanentes. Para probar rapido cambia Wardrobe Duration Seconds a 2 y Recording Duration Seconds a 5 en su MovieManager antes de Play. En el mapa Third Person conservado siguen disponibles los comandos temporales de prueba anteriores.
2. Completa las tres tomas con el vestuario y controles existentes. Puedes fallar todos los objetivos: la pelicula igualmente termina y recibe una estrella.
3. Despues de Take 3 aparece el titulo. El mundo sigue funcionando, pero el reloj compartido permanece detenido en cero y los actores esperan silenciosamente en su primer pose durante toda la tarjeta/transicion.
4. Al terminar la transicion comienza playback desde tiempo cero para los tres actores, en la camara final existente. Se ocultan HUD de objetivos, debug y DrawHUD, y se bloquean movimiento/mirada/actuaciones del Pawn vivo. Los outfits y eventos de props/gestos/sonidos vienen de cada grabacion.
5. Cuando termina playback se abre automaticamente Results: titulo, promedio, estrellas, roles, objetivos completados, premios y critica del publico.
6. Pulsa **Replay Movie**. Se repite titulo y pelicula con las mismas grabaciones, resultados y camara. El boton no recalcula scores ni premios.
7. Pulsa **Record New Movie** para volver al vestuario de Take 1 con datos limpios. **R** tambien sirve en la UI nativa de resultados. Los botones anteriores quedan invalidados: no pueden controlar una pelicula nueva.

**Return to Main Menu** usa el front end de fase 7 dentro del banco: esta habilitado sin asignar otro mapa y limpia las grabaciones/audio antes de mostrarlo. Sin front end activo conserva el viaje anterior a Main Menu Map, si asignaste un mapa existente y cocinado. No hay exportacion MP4 ni codificacion de video. La reproduccion de pantalla completa es el modo implementado.

## Configurar MovieManager

Selecciona tu instancia/Blueprint de MovieManager. No reemplaces tu personaje, recorder ni ReplayActor. Los campos previos de vestuario, objetivos y tiempos 10/30 siguen funcionando.

| Propiedad | Funcion |
| --- | --- |
| Premiere Camera | Tu camara final del nivel. Se usa sin moverla, y no se destruye al reiniciar |
| Automatic Camera Offset | Fallback existente si no asignaste una camara; se crea una vez por pelicula y se reutiliza al pulsar Replay |
| Results Settings > Movie Title / Tagline | Titulo y subtitulo; se copian al iniciar la pelicula |
| Results Settings > Rating Texts | Nombres y critica por numero de estrellas. Las reglas numericas permanecen en C++ |
| Results Settings > Award Styles | Nombre, descripcion e icono opcional para cada tipo de premio |
| Results Settings > Costume Disaster Rules | Combinaciones opcionales de tags guardados en el outfit |
| Premiere Settings > Title Fade In Seconds | Fade del texto inicial, 0.25 s por defecto |
| Premiere Settings > Title Hold Seconds | Titulo plenamente visible durante 2 s por defecto |
| Premiere Settings > Title Fade Out Seconds | Desvanecimiento de la tarjeta para revelar la camara, 0.4 s por defecto |
| Premiere Settings > Results Fade In Seconds | Fade de resultados, 0.25 s por defecto |
| Premiere Settings > Letterbox / Height Fraction | Bandas negras durante playback; 0.08 del alto por banda por defecto |
| Premiere Settings > Premiere Music / Premiere Sting | Audio opcional al entrar en el titulo; la musica puede continuar durante playback |
| Premiere Settings > Results Music / Results Sting | Audio opcional al abrir resultados |
| Premiere Settings > Button Sound | Sonido opcional de los botones Replay/New Movie |
| Premiere Settings > Music Volume / Effects Volume | Volumen configurable; usa SoundCue/Wave looped si deseas musica en bucle |
| Movie Presentation Widget Class | Clase nativa MoviePresentationWidget o tu Widget Blueprint hijo |
| Show Movie Presentation UI | Activa la UI nativa/clase asignada; desactivala solo si tu UI propia manejara resultados/input |
| Main Menu Map | Referencia al mapa real del menu. Dejalo en None si aun no existe |

La tarjeta dura la suma de Fade In + Hold + Fade Out: **2.65 s** por defecto. Puedes poner los tres en cero para omitirla durante pruebas. El playback siempre empieza despues, desde cero; un frame largo de la transicion no adelanta la pelicula. Musica/stings/botones pertenecen al MovieManager y se detienen al cambiar de presentacion, reiniciar, parar o destruirlo. No se cambian los sonidos grabados ni assets de audio.

Se usa `MoviePremiere` durante titulo y reproduccion, y el estado existente `Finished` para resultados. **Presentation Phase** distingue None, TitleCard, Playback y Results. No se cambiaron valores del enum EMovieState que usan tus Blueprints.

## Puntuacion y estrellas

El promedio proviene del ScoreSummary existente: `(Robber + PoliceOfficer + Customer) / 3.0`. `MovieResults.FinalScore` conserva el double original. La UI nativa muestra dos decimales; la decision de estrellas ocurre **antes de redondear**.

| Promedio sin redondear | Estrellas | Nombre predeterminado |
| --- | --- | --- |
| 90 a 100 | 5 | Masterpiece |
| 75 a menos de 90 | 4 | Box Office Hit |
| 50 a menos de 75 | 3 | Cult Classic |
| 25 a menos de 50 | 2 | So Bad It's Good |
| Menos de 25 | 1 | Cinematic Disaster |

Por ejemplo, 89/90/90 produce 89.666..., cuatro estrellas. Los textos y reviews se editan en Rating Texts. El evaluador puro `MovieResultsLibrary.GetStarsForScore` tambien esta disponible en Blueprint.

Cada resultado de rol expone RoleId, RoleName, Score, ObjectiveCount, CompletedObjectiveIds/Names, DanceEvents, DistinctSoundIds y CostumeTags. Los nombres de objetivos se resuelven desde las definiciones copiadas al inicio, no desde widgets actuales. Los premios usan estas estadisticas de los eventos almacenados; ver la pelicula otra vez no incrementa contadores.

## Condiciones de los premios

| Premio | Condicion y empates |
| --- | --- |
| Best Actor | Mayor score individual; todos los roles empatados aparecen. Si los tres tienen cero, los tres comparten el empate |
| Director's Cut | Todos los objetivos configurados en la pelicula aparecen completados, y existe al menos un objetivo. Es un premio a la pelicula completa; no se deduce solo de obtener 100 |
| Dancing Legend | Mayor numero de eventos Gesture con ID `Dance`, con minimo uno; todos los lideres empatados ganan |
| Sound Chaos | Mayor numero de IDs distintos de eventos Sound, con minimo uno. Repetir la misma risa no aumenta variedad; todos los lideres empatados ganan |
| Costume Disaster | Outfit inicial grabado que cumple alguna regla de tags; se muestran todos los roles que cumplen |

No se muestran premios Dance/Sound/Director/Costume sin cumplir su condicion. StopGesture y el gesto visual opcional de UseProp no se cuentan como un segundo evento Dance. Los premios no conceden mas puntos a los objetivos.

Para Costume Disaster, edita **Cosmetic Catalog > item > Cosmetic Tags** y las reglas en Results Settings. Una regla requiere Id y al menos un Required Tag; exige todos los Required Tags y ninguno de los Forbidden Tags. La regla de ejemplo `ChickenOnDuty` usa **`Costume.AnimalHead` + `Costume.OfficialUniform`**: Head.Chicken y Body.Uniform del catalogo nativo ya tienen esos tags. Los catalogos serializados en Blueprints antiguos pueden necesitar que los agregues tu mismo. Los tags se copian en cada outfit grabado; cambiar el catalogo de otro rol no modifica premios previos. El sistema usa FName, sin agregar un modulo GameplayTags.

## Disenar tu UMG

1. Crea un Widget Blueprint con **MoviePresentationWidget** como Parent Class (o cambia Parent Class en Class Settings). Asignalo a Movie Presentation Widget Class.
2. Con el Designer sin raiz, funciona el layout nativo. Puedes editar Title Font, Score Font, Body Font, colores, imagen de fondo de titulo/resultados, etiquetas de botones y encabezados en Class Defaults.
3. Si agregas una raiz Canvas/Overlay propia, el codigo conserva tu jerarquia. Implementa **On Presentation Changed** y usa su Data: Phase, Results, PresentationId, TitleTextOpacity, TitleBackgroundOpacity, ResultsOpacity y flags CanReplay/CanRecordNew/CanReturnToMenu.
4. Muestra tu tarjeta en TitleCard, tus bandas/cine en Playback y tu panel en Results. Los valores de opacidad permiten reutilizar las transiciones nativas o crear tu propia presentacion. `Results.Awards` incluye Name, Description, Icon y WinnerRoleIds; resuelve nombres desde Results.Roles.
5. En tus botones llama los metodos heredados **Replay Movie**, **Record New Movie**, **Return to Main Menu** del widget. Ya verifican estado y el token de la instancia; no llames StartRecording ni manipules el reloj desde UI.
6. Si creas una interfaz independiente con Show Movie Presentation UI desactivado, escucha OnResultsReady, lee MovieResults y llama las funciones del MovieManager. Tu Blueprint debe establecer foco/cursor/InputModeUIOnly para sus propios botones.

Los iconos de premio estan expuestos para tu UMG; el layout nativo usa texto y estrellas, sin exigir texturas. La zona central de resultados tiene scroll para admitir mas objetivos/textos sin cortar los botones inferiores. Tus fuentes y texturas deben quedar referenciadas en la clase de widget/manager usada por el mapa para incluirlas en el cook.

El gestor retira su HUD de objetivos y oculta DrawHUD del PlayerController durante premiere/resultados; restaura su estado al detener/reiniciar. Si posteriormente agregas otro HUD UMG propio, ocultalo al recibir **OnStateChanged = MoviePremiere/Finished**, y muestralo en Preparation/Recording. No se usa RemoveAllWidgets ni se eliminan interfaces ajenas.

## API y eventos Blueprint

- **MovieResults**, **ScoreSummary**, **PresentationPhase**, **PresentationId**, **ActiveMoviePresentation**: lectura del resultado/presentacion actual.
- **IsResultsOpen**, **GetPresentationView**, **ReplayMovie**, **RecordNewMovie**, **CanReturnToMainMenu**, **ReturnToMainMenu**: API para UI.
- **OnPremiereTitleStarted**: tarjeta preparada y reloj detenido.
- **OnPremiereStarted**: empieza el playback real despues de la tarjeta, con reloj en cero; conserva el significado de inicio de reproduccion.
- **OnPresentationPhaseChanged**: cambios entre TitleCard, Playback y Results; OnMovieReset/OnStateChanged indican limpieza o nueva sesion.
- **OnResultsReady**: copia del resultado cuando se abre Results; despues se emite el OnMovieFinished existente si ningun listener cambio de sesion.

Replay solo esta permitido desde Results, reinicia indices de eventos/poses/props y reutiliza la misma camara. RecordNewMovie usa la limpieza segura de RestartMovie; este ultimo sigue disponible para interrumpir una toma, titulo o playback. Un nuevo movie borra performances, actores, datos del recorder, objetivos/scores, resultados, audio de presentacion e indices/temporizadores de la sesion y vuelve al default de vestuario de Take 1.

## Entrada para un juego empaquetado

La fase 6 original conservo mapa/Config. Despues, por peticion del director, se integro **Lvl_Bank** como mapa principal: contiene **StudioGameMode**, MovieManager Auto Start y una camara final fija. Reutiliza **BP_ThirdPersonCharacter y BP_ThirdPersonPlayerController** existentes y crea/inicia un solo MovieManager si no hay uno colocado. Tambien sirve para Shipping, aunque esta entrega verifica el paquete Development. El mapa anterior sigue disponible; detalles en [BankIntegration.md](BankIntegration.md).

Para integracion permanente, configura tu mapa en **World Settings > GameMode Override = StudioGameMode**, o crea un Blueprint hijo para elegir tu MovieManager Class/Default Pawn Class/Player Controller Class. Si ya tienes un GameMode personalizado con logica propia, conservalo: coloca/configura MovieManager y usa Auto Start o StartMovie desde su flujo actual. No necesitas cambiar el GameMode para la premiere.

Para probar sin guardar ese cambio, el launcher del paquete usa la URL:

```text
/Game/OneManStudio/Maps/Lvl_Bank?game=/Script/MyProject.StudioGameMode
```

El banco Meshy ya contiene zonas/props de los quince objetivos y esta incluido en el paquete actual. El paquete historico Phase6 sigue conservado aparte. Si hay un menu, incluyelo tambien: asignarlo en Main Menu Map sin cocinarlo deja el boton deshabilitado.

## Compilar, empaquetar y repetir pruebas

Con Unreal Editor cerrado:

```powershell
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Editor
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-Movie.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-PremierePlay.ps1' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -ReportSuffix Bank -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Game
& 'C:\Games\OneManStudio\MyProject\Scripts\Package-Windows.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-PremierePlay.ps1' -PackageDirectory 'C:\Games\OneManStudio\MyProject\Saved\Packages\Phase7' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -ReportSuffix Bank -RenderScreenshots
```

Package-Windows usa UAT BuildCookRun, Win64 Development, cook del mapa indicado, stage, pak/IoStore y archive en **Saved/Packages/Phase7**. El mapa por defecto del script es Lvl_Bank. Limita shaders a dos workers por defecto para reducir memoria; puedes ajustar ShaderWorkers. Usa Maps para incluir otros mapas/menu, por ejemplo `-Maps '/Game/OneManStudio/Maps/Lvl_Bank','/Game/UI/MainMenu'`. El script de empaquetado no escribe esos overrides en Config.

Para jugar el paquete manualmente abre **Saved/Packages/Phase7/Windows/Play-ONE-MAN-STUDIO.bat**. El banco tiene StudioGameMode y MovieManager Auto Start guardados, por lo que tambien funciona como mapa predeterminado del exe. Si sustituyes el mapa, guarda su GameMode/Auto Start o adapta el launcher despues de cocinarlo.

El smoke runner esta protegido por flags de lanzamiento y no se ejecuta en Play normal ni Shipping. Abre un proceso separado, pulsa los delegates de botones UMG, prueba input W, mueve los actores de prueba, verifica Replay/New Movie y una segunda pelicula con cero objetivos, escribe JSON/capturas y cierra solo ese proceso. Las pruebas se hacen con NoSound: falta escuchar tu musica y sonidos finales. Resultados reales, archivos cambiados y limites: [Phase6Verification.md](Phase6Verification.md).
