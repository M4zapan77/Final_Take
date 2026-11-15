# Fase 7 - cambios y verificacion real

Proyecto activo: **C:\Games\OneManStudio\MyProject\MyProject.uproject**, UE **5.6.1 CL44394996**. Trabajo y logs del **13 de noviembre de 2026**; las horas de los reportes son UTC. Zona del director: America/Mexico_City.

## Implementado

- Arranque normal en Main Menu, con Play, How to Play, Credits, Audio y Quit; introduccion con titulo, sinopsis, roles/scripts y tutorial antes de Take 1.
- Flow manager ligero sobre el MovieManager existente. Se conserva recording/replay/shared clock, vestuario, objetivos, scoring, eventos/cosmetics/props y la camara fija del banco.
- Tarjetas entre actores y wrap final, avisos configurables Choose Costume / Action / Cut / Next Actor / Wrap. La duracion limita voz y subtitulo, incluso para clips en loop.
- HUD con rol, objetivos/score, countdown visible, gesto activo y todas las teclas. Se amplio el panel de controles tras detectar un recorte en la primera captura.
- Niveles Master/Music/Effects del GameInstance, musica/voz/clicks configurables, ganancia aplicada a los sonidos vivos y de replays sin modificar performances.
- Return to Main Menu limpia la sesion dentro del mismo nivel. No requiere otro mapa ni assets de menu. Record New Movie y Replay reutilizan sus implementaciones existentes.
- Interfaces nativas funcionales y clases/datos/eventos para UMG editable; se conservan arboles Designer y assets previos.
- Importacion adicional solicitada durante la fase: Base3, en carpeta nueva, sin reemplazar el jugador o rig actual.

Setup exacto del menu/UI/audio: **[MainMenuAndGameFlow.md](MainMenuAndGameFlow.md)**. Personaje: **[Base3Character.md](Base3Character.md)**.

## Builds reales

| Destino | Resultado |
| --- | --- |
| MyProjectEditor Win64 Development, primera implementacion | Succeeded, **221.58 s** |
| MyProjectEditor, codigo final | Succeeded, **59.80 s** incremental, actualizado con el driver final |
| MyProject Win64 Development, primer paquete | Succeeded, **62.10 s** |
| MyProject Win64 Development, paquete final | Succeeded, **37.25 s** incremental |
| Primer UAT BuildCookRun Phase7 | BUILD SUCCESSFUL, **102.60 s**, ExitCode 0 |
| UAT final | BUILD SUCCESSFUL, **76.85 s**, ExitCode 0 |

Archive real con cook/stage/pak/IoStore/prerequisitos: **Saved/Packages/Phase7/Windows**, aproximadamente **728 MiB** incluidos los caches de las pruebas. El launcher manual es **Play-ONE-MAN-STUDIO.bat** dentro de esa carpeta. Los paquetes historicos Phase6 y BankIntegration se conservaron.

Toolchain: VS2022 Community / MSVC **14.44.35230**, SDK **10.0.22621.0**, .NET 8 del motor. Persiste el aviso de compilador no preferido (UE recomienda 14.38), con compilacion/enlace satisfactorios. Logs: Saved/Logs/Build-MyProjectEditor.log y Package-Windows.log; auditoria de intentos en el workspace Saved/Phase7.

## Automatizacion - 40 casos

**40 Success, 0 succeededWithWarnings, 0 failed, 0 notRun, 0 inProcess**. Reporte fresco final **2026.11.13-14.42.30 UTC** en **Saved/Automation/Movie/index.json**. Las 36 regresiones anteriores permanecen incluidas.

| Caso nuevo | Cobertura |
| --- | --- |
| Studio.FrontEndAndIntroduction | Menu sin grabar/avanzar clock, camara existente, paginas, scripts de tres roles, tutorial y comienzo protegido de Take 1 |
| Studio.TransitionsReplayAndMenu | Tres cortes/tarjetas con reloj cero, media detenida, input, scores, replay identico, camara intacta, menu limpio y otra pelicula |
| Studio.RestartAndInputOwnership | Restart durante tarjeta, borrar samples/events, abandonar pelicula activa y liberar solo los locks propios |
| Studio.UIAudioAndConfiguration | Widgets nativos, conservar Designer root, controles del HUD, niveles finitos/clamp, preferencias no sobrescritas y settings invalidos rechazados |

Se corrigieron dos problemas de los nuevos fixtures: una insercion de un elemento de TArray en si mismo y dos inferencias de punteros TObjectPtr. Tambien se ajustaron el checked-format del preview y el driver para reconocer el frame de handoff entre recorder y late tick. No se reescribio recording/replay. El test de UI detecto que BindFlow necesitaba construir el fallback sin LocalPlayer; ese caso quedo cubierto.

## Play con render y audio reales

**Test-StudioFlow.ps1: PASS**, proceso separado -game con D3D12 offscreen **1280x720**, menu/pawn/controller reales. Reporte **Saved/Automation/StudioPlay/result.json**, final **13:49:32 UTC**.

**Test-StudioFlow.ps1 -EnableAudio -ReportSuffix Audio: PASS**, reporte **14:12:58 UTC**, **Saved/Automation/StudioPlayAudio/result.json**:

- 8 vestuarios, 6 Ready y 1 timeout; la octava visita comprueba Play Again y no comienza otra grabacion.
- Las paginas de menu, introduccion/tutorial y tres roles se verificaron con sus botones UMG/delegates reales. Las teclas W, R, E, gestos y soundboard pasan por el PlayerController/input existente.
- R borro una grabacion activa con gesto/sonido y score; un Ready viejo/duplicado no inicio otra toma.
- Quince objetivos del banco, **100/100/100**, promedio **100**, **5 estrellas**. El fixture entra mediante W y luego mueve al Pawn para cubrir las zonas/proximidades; no es una partida humana completa.
- 6 tarjetas entre tomas/wraps y los cinco cues: counts **8/7/6/4/2**. Reloj detenido durante menu/vestuario/tarjetas/titulo, con movimiento/mirada segun estado.
- 3 tarjetas de titulo y 3 playbacks: pelicula original, Replay identico y otra pelicula con cero objetivos, que se puede ver y recibe una estrella.
- New Movie, Return to Main Menu, Play Again y Quit completados; los botones anteriores quedan rechazados y se conservan las preferencias de audio.
- EnableAudio inyecto tonos sinteticos de muy bajo nivel solo en memoria. **MusicPlayback, DirectorPlayback, ActingSoundPlayback y AudioCleanup = true** mediante componentes reales activos, ganancias Master/Music/Effects y ausencia de media residual al cortar/volver al menu. No son voces/musica finales y no se guardaron como assets.

Se inspeccionaron las capturas de menu, tutorial, introduccion, HUD, siguiente actor, premiere y resultados. El panel de controles cabe completo tras el ajuste y el countdown queda en una linea. Los tiempos acelerados del driver son **2 s / 5 s**, tarjetas de .55 s; los defaults guardados siguen siendo 10/30 y 2.5/2.25.

## Paquete final, recorrido a tiempos reales

**PASS, proceso terminado con exit 0**, reporte fresco **2026-11-13 14:46:17 UTC** en **Saved/Automation/StudioPackagedFullTiming/result.json**. La ejecucion usa **1920x1080**, EnableAudio y **UseDefaultTimings**: tres tomas de 30 s por pelicula, un vestuario completo de 10 s, premiere y Replay completos. No se pasa URL de mapa, GameMode override, ExecCmds, Movie.Test ni StudioSkipMenu: se prueba el arranque guardado del exe, que confirma StudioGameMode y Lvl_Bank.

- **8 wardrobes, 6 Ready, 1 timeout, 6 tarjetas entre actores/wraps, 3 titulos y 3 playbacks**. Los cinco cues tuvieron counts 8/7/6/4/2.
- Las seis performances completas guardaron **601 muestras y 30.000 segundos** cada una. Primera pelicula **100/100/100, promedio 100, cinco estrellas**, quince objetivos y tres eventos independientes por rol.
- Replay conservo datos/resultados; R y New Movie borraron los datos. La segunda pelicula de cero objetivos se pudo ver y recibio una estrella.
- Return to Main Menu limpio la sesion, mantuvo el volumen, Play Again abrio Take 1 y **Quit cerro normalmente**. Musica/director/acting audio y limpieza fueron true en el reporte.
- Se inspeccionaron capturas Full HD de menu, HUD, premiere, resultados y cero puntos. El robber puede salir del encuadre al cumplir Escape, como corresponde a una camara fija; su movimiento/eventos siguen reproduciendose.

Capturas y JSON: **Saved/Automation/StudioPackagedFullTiming**. El wrapper valido el JSON nuevo y el exit 0 despues de cerrar el proceso; no se considero suficiente el mensaje previo al cierre.

La primera ejecucion a Full HD detecto datos del HUD desactualizados entre Ready y el siguiente late tick. **BeginRecording ahora refresca el HUD antes de emitir OnRecordingStarted**, con su comprobacion de generacion para listeners Blueprint. Se repitieron las 40 regresiones, build y cook antes de volver a probar este paquete.

Una ejecucion posterior completo todos los checks pero termino con exit 3 al liberar el estado del driver desde su propio timer antes de pulsar Quit. El callback mantiene ahora un SharedRef local hasta que Tick termina. **El smoke rapido del paquete corregido con audio paso y cerro con exit 0**, reporte **Saved/Automation/StudioPackagedFastAudio/result.json**, **14:39:15 UTC**. El script exige tanto JSON nuevo como codigo de salida cero: un JSON generado antes de un cierre fallido no cuenta como PASS.

## Personaje Base3

**Importacion: PASS**, cuatro assets nuevos en **Content/OneManStudio/Characters/Base3**: SK_Base3, SK_Base3_Skeleton, SK_Base3_PhysicsAsset y model. **19 huesos**, root Hips; esqueleto distinto al de SKM_Quinn_Simple. El archivo no incluye animaciones ni texturas.

**Preview renderizado: PASS, cuatro vistas**, reporte Saved/CharacterImport/Preview/result.json y CharacterView1...4.png. Se revisaron cuerpo/extremidades/cabeza. El importador aviso de smoothing groups ausentes; el mesh importo y renderizo. El FBX original y su copia en SourceArt/Base3 conservan el SHA-256. No se cambiaron el jugador actual ni sus Blueprints/animaciones: usar Base3 en gameplay requiere retargeting/ABP y revisar sockets, fuera de la peticion de importacion.

## Archivos cambiados

Rutas Public/Private relativas a **Source/MyProject**:

| Archivo(s) | Cambio |
| --- | --- |
| Public/StudioFlowTypes.h + Private/StudioFlowTypes.cpp | Nuevos: screens, cues, settings/briefs/views y niveles de audio |
| Public/StudioFlowManager.h + Private/StudioFlowManager.cpp | Nuevos: menu/intro, input/camara propios, director/media y enlace a eventos del MovieManager |
| Public/StudioFlowWidget.h + Private/StudioFlowWidget.cpp | Nuevos: menu, tutorial/creditos/audio/intro/tarjetas, UMG customizable, foco y callbacks protegidos |
| Public/StudioAudioSubsystem.h + Private/StudioAudioSubsystem.cpp | Nuevos: preferencias de sesion Blueprint, ganancias y eventos de cambio |
| Public/StudioGameMode.h + Private/StudioGameMode.cpp | Arranque front end por defecto, reutiliza actor colocado o clase configurable; bypass explicito solo para pruebas antiguas |
| Public/MovieManager.h + Private/MovieManager.cpp | TakeTransition agregado al final del enum, tiempos copiados, media detenida, gate de AutoStart y refresco inmediato de HUD |
| Private/MovieManagerPresentation.cpp | Return to Menu por flow activo y mezcla de audio; conserva fallback a mapa y arquitectura de resultados |
| Public/ActingComponent.h + Private/ActingComponent.cpp | Ganancia de efectos aplicada a audio propio, con volumen base separado de los datos grabados |
| Public/ObjectiveHUDWidget.h + Private/ObjectiveHUDWidget.cpp | Controles/gesto/estado en ViewData, panel visible, countdown legible |
| Private/Tests/StudioFlowTests.cpp | Nuevo: cuatro casos de regresion de fase 7 |
| Private/Testing/StudioFlowPlayTest.cpp | Nuevo: smoke opt-in con menu/input/UMG reales, tiempos reales/acelerados, tonos en memoria y Quit |
| Private/Testing/BankInspectTest.cpp | Preview opcional del nuevo personaje; se conserva el survey anterior; no guarda mapas |
| Scripts/Test-Movie.ps1 | Exige los 40 casos y JSON nuevo |
| Scripts/Test-StudioFlow.ps1 | Nuevo: Editor/paquete, default startup, render, resolucion, audio y timings verificables |
| Scripts/Test-HeistPlay.ps1, Test-PremierePlay.ps1, Test-WardrobePlay.ps1 | StudioSkipMenu explicito para aislar las regresiones anteriores |
| Scripts/Package-Windows.ps1 | Archive Phase7 separado; cocina el banco, launcher sin comandos de debug |
| Scripts/Import-Base3Character.py | Nuevo: importacion autorizada a carpeta nueva y reporte de rig; se niega a sobrescribir assets |
| Docs/MainMenuAndGameFlow.md, Phase7Verification.md, Base3Character.md | Nuevos: setup, resultados y personaje |
| Docs/BankIntegration.md, PremiereAndResults.md | Actualizados para el flujo/menu y launcher actuales |
| Content/OneManStudio/Characters/Base3/*.uasset | Cuatro assets nuevos, pedidos explicitamente por el director |
| SourceArt/Base3/base (3).fbx | Copia preservada del original |

Tambien se actualizo el README del workspace. Binaries/Intermediate/DerivedDataCache/Saved contienen los outputs habituales de Unreal y pruebas. No hubo nuevos modulos/dependencias en Build.cs, plugin permanente, mapa de menu ni cambio de .uproject/Config.

## Limites y arte pendiente

- Falta asignar las **voces, musica, clicks, sonidos y animaciones finales**; defaults de audio nuevos son None y los cues de texto funcionan asi. Los tonos de la prueba solo verifican el pipeline/limpieza, no la calidad o escucha de tu mezcla final.
- Completa Credits con nombres/atribuciones del equipo y personaliza fondos, iconos, fuentes, ropa/props/layout si lo deseas. La UI nativa funciona sin esos assets.
- Base3 fue importado y previsualizado; **no se hizo retargeting ni sustitucion del jugador**. No comparte skeleton con Quinn.
- Se prueba Windows **Development** en esta PC, con render a 720p y Full HD offscreen. Shipping, otra PC/GPU y un presupuesto minimo de FPS/memoria no estan verificados. Se accionan delegates UMG e input del controlador; no fue una partida humana completa ni una prueba manual con mouse.
- Los niveles de audio son de sesion, sin persistencia a disco. Para sonidos de futuros sistemas ajenos usa esos niveles desde su Blueprint o tu mezcla.
- No se agregaron pausa avanzada, guardado persistente de peliculas, MP4, Scene Capture, online o APIs de IA runtime.
- Persisten avisos del motor por toolchain no preferido e iconos de VisionOS en el Editor. Los hitches iniciales de creacion de PSO dependen del warm-up; no se midio rendimiento minimo.

La auditoria final **workspace Saved/Phase7/FinalAudit.json** compara **260 archivos originales protegidos**, incluidos banco, mapas, assets, Config y descriptor. La unica nueva carpeta de Content autorizada en esta fase es Base3. Los hashes del codigo/docs instalados y del FBX se verifican contra sus manifests/original al cerrar la entrega.
