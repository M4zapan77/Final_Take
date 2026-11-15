# Banco - builds, pruebas y archivos cambiados

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, **UE 5.6.1 CL44394996**, Win64. Trabajo del 12 de noviembre de 2026, America/Mexico_City; los logs siguientes estan fechados el 13 en UTC. La premiere/resultados de fase 6 se termino y verifico antes de integrar el banco solicitado despues: [Phase6Verification.md](Phase6Verification.md).

## Resultado

**Lvl_Bank es el mapa principal del Editor y del juego.** Contiene el banco importado, suelo exterior, luces, un mostrador provisional, las tres zonas de objetivos, bolsa/donut/banana, PlayerStart, MovieManager Auto Start y camara final fija. El archivo Lvl_ThirdPerson original sigue intacto. No se reemplazaron el Character, su skeleton, las animaciones ni los Blueprints originales.

Setup y uso exactos: [BankIntegration.md](BankIntegration.md). Premiere, scores/premios y UMG: [PremiereAndResults.md](PremiereAndResults.md).

## Compilacion y cook reales

| Destino/paso | Resultado |
| --- | --- |
| MyProjectEditor / Win64 / Development | **Succeeded**, ultima compilacion **55.92 s** |
| MyProject / Win64 / Development | **Succeeded**, cambios del runner del banco compilados/enlazados en **75.41 s** |
| Primer BuildCookRun del banco | **BUILD SUCCESSFUL**, **143.14 s**, ExitCode 0 |
| Recook final tras guardar uso Nanite del material | **BUILD SUCCESSFUL**, **58.97 s**, ExitCode 0; Game up to date en **5.28 s** |

Paquete real, con cook/stage/pak/IoStore/archive y prerequisitos: **Saved/Packages/BankIntegration/Windows**, aproximadamente **727 MiB**. No es solo un exe uncooked. El paquete historico Phase6 permanece aparte. El launcher **Play-ONE-MAN-STUDIO.bat** inicia el banco y el loop de forma automatica.

Logs activos: Saved/Logs/Build-MyProjectEditor.log y Package-Windows.log. El workspace Saved/BankIntegration conserva PackageFirstConsole.log, PackageConsole.log y los logs de importacion/pruebas. La segunda ejecucion recocino los assets; no se cambio C++ despues del build del Game.

Se uso VS2022 Community / MSVC **14.44.35230**, SDK **10.0.22621.0**, .NET 8 incluido con UE. UBT mantiene el aviso de toolchain no preferido (prefiere 14.38), pero ambos destinos compilaron. Cook emitio estimaciones de memoria para el mesh grandes respecto de la disponible; termino correctamente con dos workers de shaders.

## Regresiones automaticas

**Test-Movie.ps1: 36 Success, 0 warnings de casos, 0 failed, 0 notRun, 0 inProcess**. Reporte fresco **2026.11.13-04.09.25 UTC**, despues de seleccionar Lvl_Bank como mapa de inicio.

Los 36 casos cubren recording/replay/clock/colision, eventos, tres tomas, vestuario, props/objetivos y la fase 6: thresholds exactos sin redondear, empates/premios condicionales, titulo/replay, botones reales, limpieza, configuracion y pelicula de cero puntos. No se agregaron sistemas de gameplay al integrar el mapa.

Reporte: **Saved/Automation/Movie/index.json**; log: Saved/Logs/MovieTests.log.

## Banco real en Editor

**Test-HeistPlay -Map /Game/OneManStudio/Maps/Lvl_Bank -UseLevelSetup -ReportSuffix Bank -RenderScreenshots: PASS**.

- Uso el MovieManager, la camara, zonas y props colocados en el banco, sin sustituirlos por el demo sintetico.
- El fixture inicio al robber fuera de la puerta azul y envio W al input existente: atraveso la puerta y completo Robber.EnterBank antes de cualquier teletransporte. La colision del FBX permite entrar.
- Completo **15 objetivos, tres roles a 100/100/100 y promedio 100**; comprobo HUD, interacciones grabadas, premiere y reset del score.
- Las acciones posteriores a la entrada usan movimientos/llamadas del fixture para cubrir todos los objetivos; no es una partida humana completa.

**Test-PremierePlay -Map .../Lvl_Bank -ReportSuffix Bank -RenderScreenshots: PASS**: 6 vestuarios, 3 tarjetas, 3 playbacks, scores 100/75/50, promedio 75, cuatro estrellas, Replay conservado, New Movie limpio y nueva pelicula de cero puntos.

Reportes: **Saved/Automation/HeistPlayBank/result.json** y **PremierePlayBank/result.json**. Se revisaron las capturas para corregir exposicion de inicio y encuadre: la camara final esta por encima del techo abierto y no oculta los actores detras de la viga frontal.

## Paquete Windows final

**Test-PremierePlay -PackageDirectory .../BankIntegration -Map .../Lvl_Bank -ReportSuffix Bank -RenderScreenshots: PASS**, reporte final **2026-11-13 04:18:51 UTC**:

- Ejecuto Windows/MyProject/Binaries/Win64/MyProject.exe con D3D12, viewport y UMG reales; no cargo assets uncooked del Editor.
- **6 preparations, 3 title cards, 3 playbacks**: pelicula original, Replay y pelicula nueva.
- **75/100, cuatro estrellas**, scores individuales 100/75/50, cinco premios correctos con empates.
- Grabaciones/outfits/gestos/sonidos logicos/props independientes y camera fija sincronizados. Replay conserva todas las propiedades reflejadas de performances y resultados.
- New Movie borra los datos y vuelve al primer vestuario. La pelicula nueva con cero objetivos se ve y recibe una estrella.
- Entrada nativa de StudioGameMode e input W verificados.

**Test-HeistPlay -PackageDirectory .../BankIntegration -UseLevelSetup -ReportSuffix Bank -RenderScreenshots: PASS**, reporte final **2026-11-13 04:19:27 UTC**:

- Se omitio deliberadamente **Map** para comprobar el arranque desde la configuracion guardada. El log confirma **Game class StudioGameMode** y **World /Game/OneManStudio/Maps/Lvl_Bank.Lvl_Bank**.
- Entrada por la puerta con W verificada en la colision cocinada, quince objetivos y **100/100/100**, premiere y reinicio aprobados.

Reportes: **Saved/Automation/PremierePackagedBank/result.json** y **HeistPlayPackagedBank/result.json**. Capturas inspeccionadas del paquete: [tres actores](../Saved/Automation/PremierePackagedBank/Playback.png), [resultados](../Saved/Automation/PremierePackagedBank/Results.png), [vestuario](../Saved/Automation/PremierePackagedBank/Wardrobe.png). El reporte de premiere tambien conserva TitleCard.png y ZeroResults.png.

El primer paquete paso las pruebas funcionales, pero su log mostro que faltaba persistir **Used With Nanite** en M_MeshyBank. Se habilito usando MaterialEditingLibrary, se guardo, se recocino y **se repitieron ambas pruebas en el paquete corregido**. Los logs finales no tienen el aviso de material Nanite faltante. Los avisos restantes de TickDeferredCommands corresponden a aplicar los comandos de calidad del runner al arrancar (unos cientos de ms).

## Archivos cambiados respecto del cierre de fase 6

| Ruta(s), relativas al proyecto activo | Cambio |
| --- | --- |
| Content/OneManStudio/Maps/Lvl_Bank.umap | Nuevo mapa principal, con actores/configuracion permanente |
| Content/OneManStudio/Environment/Bank/SM_MeshyBank.uasset | Nuevo mesh importado, Nanite y colision compleja con puertas transitables |
| .../M_MeshyBank.uasset | Nuevo PBR de dos caras, uso Nanite persistido |
| .../T_Bank_BaseColor, T_Bank_Normal, T_Bank_Metallic, T_Bank_Roughness.uasset | Cuatro texturas configuradas, limite runtime 4096 |
| .../MI_BankGround.uasset, MI_BankCounter.uasset | Dos materiales provisionales nuevos |
| Config/DefaultEngine.ini | Solo EditorStartupMap y GameDefaultMap apuntan a Lvl_Bank |
| SourceArt/MeshyBank | Los cinco archivos fuente originales; FBX tambien extrajo cuatro imagenes en su carpeta .fbm |
| Source/MyProject/Private/Testing/BankInspectTest.cpp | Nuevo survey/capturas opt-in, excluido de Shipping |
| Source/MyProject/Private/Testing/ObjectivePlayTest.cpp | Modo opcional UseLevelHeistSetup: usa actores reales, verifica entrada por W y reporte independiente; conserva fixture anterior |
| Scripts/Import-MeshyBank.py | Importacion limitada a la nueva carpeta, material/texturas/Nanite/colision y reporte |
| Scripts/Build-BankLevel.py | Generacion de nuestro nuevo nivel; protege cambios posteriores mediante hash |
| Scripts/Test-HeistPlay.ps1 | Map, PackageDirectory, UseLevelSetup y sufijo de reporte; fixture sintetico sigue usando ThirdPerson por defecto |
| Scripts/Test-PremierePlay.ps1 | Sufijo de reporte para conservar pruebas historicas |
| Scripts/Package-Windows.ps1 | Defaults del nuevo mapa y archive separado; launcher usa el primer mapa cocinado |
| Docs/BankIntegration.md, BankVerification.md, PremiereAndResults.md | Setup del banco, resultados y guia actualizada |

Tambien se actualizo el README del workspace. La lista completa de los **26 archivos de fase 6 (17 Source, 3 Scripts, 6 Docs)** esta en Phase6Verification.md. La integracion del banco no reescribe recorder, replay, MovieManager ni ObjectiveManager.

## Auditoria y limites

La auditoria SHA-256 del workspace **Saved/BankIntegration/FinalAudit.json** compara los **251 archivos protegidos originales**: la unica modificacion existente autorizada es Config/DefaultEngine.ini y sus dos entradas de mapa. Se conservan el mapa anterior, .uproject y todos los assets/Blueprints originales. Hay nueve archivos nuevos en Content: ocho assets del banco y su mapa. El ZIP y los cinco fuentes originales se comparan por hash; los sources/scripts/docs instalados se verifican contra sus manifests.

No se agregaron plugins permanentes ni dependencias runtime para importar. Python/EditorScripting solo se habilitaron en el proceso de generacion. Los scripts se niegan a sobrescribir un nivel editado fuera del proceso.

Limites: NoSound valida eventos/IDs/tiempos, **no la mezcla audible**. Debes asignar/revisar tus clips finales, ropa y props artisticos; mostrador/cosmetics/props de juego siguen siendo placeholders. El FBX trae avisos de smoothing/normales; se verificaron render y puerta, no un acabado artistico final ni FPS objetivo. Solo se probo Windows **Development** en esta PC a 1280x720; Shipping, otros equipos/GPU y resoluciones no estan verificados. No hay Main Menu, Scene Capture ni exportacion MP4.
