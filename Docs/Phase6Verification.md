# Fase 6 - Verificacion y archivos cambiados

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, UE **5.6.1 CL44394996**. Trabajo iniciado el 12 de noviembre de 2026, America/Mexico_City; logs en UTC del 13 de noviembre.

Estos resultados describen el hito de premiere/resultados sobre el mapa existente. El director pidio posteriormente sustituir el escenario por un banco Meshy; esa integracion se documenta por separado, despues de verificar este hito.

## Implementacion

- `MoviePremiere` ahora contiene TitleCard y Playback. El reloj compartido se mantiene en cero hasta terminar titulo/fades; los eventos de tiempo cero solo salen al comenzar playback.
- Se conserva la camara final asignada, sin cambiar su transform ni destruirla. El fallback automatico se reutiliza al pulsar Replay.
- `Finished` abre resultados automaticamente. No se cambiaron los valores del enum de estados anterior.
- Se retiran el HUD de objetivos/debug y DrawHUD durante premiere/resultados. El Pawn vivo permanece oculto y bloqueado; los replays conservan sus propios movimientos, gestos, sonidos, cosmetics y props.
- El resultado/estadisticas/premios se calculan una vez desde las performances y ScoreSummary guardados. Los limites de estrellas usan el double sin redondear.
- Replay Movie conserva todos los datos y vuelve a titulo/playback. Record New Movie usa la limpieza existente y retorna a Take 1 wardrobe; los widgets anteriores quedan invalidados.
- UI nativa funcional y subclass UMG editable, textos/reviews, fuentes, colores, fondos, iconos de premio, musica/stings/click y transiciones configurables.
- No se implemento Scene Capture/cine en pantalla, exportacion MP4 ni un menu inexistente.

Configuracion exacta y APIs: [PremiereAndResults.md](PremiereAndResults.md).

## Compilacion real

VS2022 Community / MSVC **14.44.35230**, Windows SDK **10.0.22621.0**, .NET del motor. UBT avisa que prefiere MSVC 14.38; los destinos compilaron y enlazaron con la version instalada.

| Destino | Resultado | Ultimo tiempo |
| --- | --- | --- |
| MyProjectEditor / Win64 / Development | Succeeded, DLL activa actualizada | 47.56 s |
| MyProject / Win64 / Development | Succeeded, exe actualizado | 191.97 s |
| Cook/stage/pak/IoStore/archive Windows Development | BUILD SUCCESSFUL; AutomationTool ExitCode 0 | BuildCookRun 495.62 s |

Logs: `Saved/Logs/Build-MyProjectEditor.log`, `Build-MyProject.log`, `Package-Windows.log`. Las primeras compilaciones de esta fase y los intentos intermedios quedan en los logs de auditoria del workspace; los resultados de la tabla son los finales de cada destino. Dos errores del driver de pruebas (deduccion de TObjectPtr y llamada a API de input antigua/inexistente) y la expresion multilinea de su script se corrigieron antes de ejecutar el smoke. El recorder/replay no se reescribio.

## Automatizacion: 36 casos

`Scripts/Test-Movie.ps1` valido el JSON nuevo **2026.11.13-02.07.51 UTC**: **36 Success, 0 succeededWithWarnings, 0 failed, 0 notRun, 0 inProcess**. Duracion de los casos: **1.491 s**, aparte del arranque del motor. Se avanzan relojes de juego por ticks; no son tomas humanas en tiempo real.

Las 28 regresiones previas siguen incluidas y aprobadas. Antes de sumar los ocho casos se corrio tambien la suite previa de 28 contra la primera implementacion, con todos los casos aprobados.

| Caso nuevo | Lo comprobado |
| --- | --- |
| Ratings.ScoreThresholds | Ambos lados de 25/50/75/90, cero/100, NaN seguro, promedio fraccional sin redondear, score inconsistente rechazado |
| Ratings.AwardsAndTies | Cinco premios, todos los empatados, sonidos repetidos no aumentan variedad, Director por IDs completados y costume por tags capturados |
| Ratings.ConditionalAwards | Cero objetivos/eventos no inventa premios; Best Actor representa el empate en cero; StopGesture/UseProp visual no suma Dance; forbidden tags y datos invalidos rechazados |
| Premiere.TitleAndReplay | Titulo silencioso con clock cero, camara asignada intacta, bloqueo del Pawn, tres eventos independientes, resultados automaticos, replay duplicado rechazado, comparacion reflejada completa de performances y resultados antes/despues |
| Premiere.ResultsButtons | Jerarquia UMG nativa, datos correctos, delegates reales de Replay/New Movie, menu ausente oculto y widgets anteriores invalidados |
| Premiere.RestartAndCleanup | Interrumpir titulo/playback, borrar datos/actores, restaurar cursor/click/camara/visibilidad, conservar locks externos |
| Premiere.ZeroScore | Pelicula vacia visible, cero puntos/una estrella, sin objetivos inventados, Replay y nuevo movie; cambios posteriores de authoring no alteran el resultado anterior |
| Premiere.Configuration | Transicion invalida rechazada, titulo de duracion cero, menu inexistente no destruye grabaciones |

Reporte `Saved/Automation/Movie/index.json`; log `Saved/Logs/MovieTests.log`.

## Play con render y UI reales

**`Scripts/Test-PremierePlay.ps1 -RenderScreenshots`: PASS**, reporte **2026.11.13-02.23.44 UTC**. Proceso separado `-game`, D3D12 offscreen **1280x720**, LocalPlayer/viewport y personajes/controlador Blueprint reales del proyecto.

- StudioGameMode inicio el bucle sin comandos de debug para iniciar la pelicula. Se verifico movimiento mediante una pulsacion W enviada al input del PlayerController existente antes de mover el fixture por sus trayectorias.
- **6 vestuarios**, **3 tarjetas**, **3 playbacks**: primera pelicula, Replay de esa misma pelicula y una pelicula nueva con cero objetivos.
- Los botones se accionaron mediante los delegates de las instancias UMG nativas. Ready rechazaba duplicados, y los widgets viejos de resultados fueron rechazados.
- Primera pelicula: roles **100/75/50**, promedio **75**, **4 estrellas / Box Office Hit**, seis objetivos configurados completados y los cinco premios; los empates de Dance y Sound incluyeron ambos ganadores.
- El fixture grabo movimientos, gestos, sonidos logicos, outfits distintos, recogida de bolsa y uso de donut. En premiere se comprobaron sus eventos una vez por actor, props independientes, colision desactivada y camara asignada inmovil.
- Replay conservo las propiedades reflejadas de todas las performances y el resultado; New Movie borro scores/objetivos/resultados y regreso al primer wardrobe.
- La segunda pelicula obtuvo **0**, **1 estrella / Cinematic Disaster**, pudo verse y mostro los tres roles con cero. No aparecieron Director/Dance/Sound/Costume; Best Actor mostro el empate de los tres.

La prueba usa **2 s de wardrobe, 2 s de recording y 1 s de titulo** para revisar rapido; los defaults siguen siendo 10/30 y titulo de 2.65 s. Los objetivos reducidos del fixture son solo para probar fronteras/premios. Los quince objetivos del banco siguen verificados por la suite previa.

Reporte `Saved/Automation/PremierePlay/result.json`; log `Saved/Logs/PremierePlay.log`. Se inspeccionaron [titulo](../Saved/Automation/PremierePlay/TitleCard.png), [tres actores/cine](../Saved/Automation/PremierePlay/Playback.png), [resultados](../Saved/Automation/PremierePlay/Results.png), [cero puntos](../Saved/Automation/PremierePlay/ZeroResults.png). Estrellas, textos, objetivos, premios y botones son legibles; la zona central tiene scroll para listas largas.

## Paquete Windows

**Package-Windows.ps1: BUILD SUCCESSFUL**, UAT ExitCode=0, BuildCookRun **495.62 s** (primer cook con shaders SM5/SM6 y dos workers). Archive real: **Saved/Packages/Phase6/Windows**, **46 archivos / 673.0 MiB**, incluidos exe runtime, pak/IoStore, dependencias y prerequisitos. No es solo un exe Development sin cocinar.

**Test-PremierePlay.ps1 -PackageDirectory .../Saved/Packages/Phase6 -RenderScreenshots: PASS**, ejecutando `Windows/MyProject/Binaries/Win64/MyProject.exe`, sin Editor ni assets uncooked. Reporte **2026.11.13-02.45.44 UTC** en `Saved/Automation/PremierePackaged/result.json`:

- 6 preparaciones, 3 tarjetas, 3 playbacks; score 75, cuatro estrellas.
- Entrada nativa de juego y movimiento por input W verificados usando Character/PlayerController existentes.
- UI, tres actores, eventos/cosmetics/props, premios/empates, Replay inmutable, New Movie limpio y pelicula de cero puntos aprobados.
- Se inspeccionaron las capturas cocinadas de [playback](../Saved/Automation/PremierePackaged/Playback.png), [resultados](../Saved/Automation/PremierePackaged/Results.png), titulo y cero. Fuentes/estrellas/materiales e interfaces se ven correctamente en el paquete.

Log `Saved/Logs/PremierePackaged.log`. Hubo un aviso de warm-up de 2.5 s al aplicar los comandos de calidad de render del runner; el flujo completo paso. El juego manual se abre con **Saved/Packages/Phase6/Windows/Play-ONE-MAN-STUDIO.bat**. Sin ese launcher/URL, el exe respeta el GameMode guardado originalmente en el mapa.

Ademas se repitieron **Test-HeistPlay.ps1 -RenderScreenshots** y **Test-WardrobePlay.ps1 -RenderScreenshots** tras esta fase: ambos **PASS**. El primero verifico los **quince objetivos y 100/100/100**; el segundo, **3 wardrobes, 2 Ready, 1 timeout**, outfits/eventos/premiere/restart. Reportes actualizados en sus carpetas Saved/Automation. El check del HUD del heist ahora exige que este oculto durante playback.

## Archivos cambiados

Rutas Public/Private relativas a **Source/MyProject/** en el proyecto activo:

| Archivo(s) | Cambio |
| --- | --- |
| Public/MovieResultsTypes.h + Private/MovieResultsTypes.cpp | Nuevos: fases de presentacion, settings, ratings/reviews, estilos/reglas de premios, estadisticas y resultados Blueprint |
| Public/MovieResultsLibrary.h + Private/MovieResultsLibrary.cpp | Nuevos: evaluador puro de la pelicula, thresholds sin redondear, premios/empates sin recompensas nuevas |
| Public/MoviePresentationWidget.h + Private/MoviePresentationWidget.cpp | Nuevos: titulo, bandas, resultados, objetivos/premios, botones y subclass UMG editable |
| Private/MovieManagerPresentation.cpp | Nuevo: presentacion, reloj retenido, replay/resultado/menu, input/HUD y audio propio |
| Public/MovieManager.h + Private/MovieManager.cpp | Integracion localizada de fases/resultados, snapshot de settings, camara reutilizada y limpieza del bucle existente |
| Public/WardrobeTypes.h + Private/WardrobeTypes.cpp | Tags opcionales en cosmetics; tags de ejemplo Chicken/Uniform guardados por valor en el snapshot existente |
| Public/StudioGameMode.h + Private/StudioGameMode.cpp | Nuevos: entrada opcional de juego empaquetado usando los Blueprint Character/PlayerController existentes, sin modificar mapa/Config |
| Private/Tests/MovieResultsTests.cpp | Nuevo: tres casos de ratings/awards |
| Private/Tests/MoviePremiereTests.cpp | Nuevo: cinco casos de flujo, UI, replay, limpieza y configuracion |
| Private/Testing/PremierePlayTest.cpp | Nuevo: smoke opt-in separado, protegido por -game/-PremiereSmoke y excluido de Shipping |
| Private/Testing/ObjectivePlayTest.cpp | Expectativa actualizada: HUD de gameplay oculto durante premiere, manteniendo checks de eventos/props/scoring |
| Scripts/Test-Movie.ps1 | Exige los 36 casos y reporte nuevo |
| Scripts/Test-PremierePlay.ps1 | Nuevo: prueba renderizada en Editor o exe empaquetado, con Map y reporte JSON fresco |
| Scripts/Package-Windows.ps1 | Nuevo: UAT BuildCookRun, control de workers de shaders y launcher del paquete |
| Docs/PremiereAndResults.md, Docs/Phase6Verification.md | Nuevos: setup completo y resultados reales |

Tambien se actualizaron las referencias de Docs/MovieManager.md, ActingEvents.md, Wardrobe.md, ObjectivesAndScoring.md y el README del workspace. Total del hito: **26 archivos aplicados al proyecto (17 Source, 3 Scripts, 6 Docs)** y el README del workspace. Binaries/Intermediate/Saved contienen los outputs normales de compilacion/pruebas; no son cambios a assets artisticos.

## Limites

- Las pruebas usan **NoSound**. Se validan eventos/IDs/cursors, no la escucha de musica ni SFX. Asigna tus sonidos y revisa mezcla/volumen manualmente.
- Los clips finales siguen configurandose en ActingComponent. Las actuaciones logicas cuentan aunque no tengan animacion/sonido asignado; se conservaron las pruebas de presentacion de animaciones de fases anteriores.
- El layout es funcional y configurable; no sustituye el arte final de tu HUD. Los iconos de premios estan en los datos para tu Widget Blueprint; el layout nativo usa texto.
- No hay mapa Main Menu existente; su viaje no se ejecuto. Un mapa faltante/no cocinado no habilita el boton ni borra la pelicula.
- No se verifico Shipping, otra PC, otra GPU ni resoluciones distintas. El runner no se incluye en Shipping; la logica y StudioGameMode si son runtime.
- No hay MP4, persistencia entre ejecuciones ni Scene Capture a pantalla de cine. Pantalla completa es el fallback implementado.
- Persisten avisos del motor por iconos de VisionOS ausentes y el toolchain no preferido. Los mensajes internos de AutomationTest anteriores al arranque de los casos no forman parte de fallos de la suite; el JSON final de los 36 casos no tiene warnings/fallos.

Al cerrar este hito, **antes de importar el banco pedido despues**, la auditoria SHA-256 conserva sin cambios los **250 archivos Content/Config y MyProject.uproject**. La auditoria previa al banco queda en el workspace `Saved/Phase6/FinalBeforeBankAudit.json`. La integracion posterior del nuevo escenario tiene su propio alcance autorizado y registro.
