# Fase 5 - Resultados reales y archivos cambiados

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. Verificacion: **12 de noviembre de 2026**, America/Mexico_City. Los logs guardan UTC, ya **13 de noviembre**.

## Resultado

Se agregaron objetivos configurables por rol, recogida/uso de props con E, eventos de interaccion en la linea de tiempo existente, HUD nativo durante vestuario/grabacion y puntajes guardados con cada performance. El resultado de la pelicula promedia los tres roles y se presenta durante la premiere. Fallar todos los objetivos no impide completar el bucle.

Se conservaron recorder, samples a 20 Hz, shared scene clock, replay por interpolacion, tres tomas, gestos/sonidos, vestuario, rig y camaras existentes. Las recompensas solo se evaluan sobre el Pawn controlado durante Recording; los replays son targets y presentacion independientes, sin colision ni inventario compartido.

No se crearon/modificaron mapas, Widget Blueprints, assets artisticos, Config ni el descriptor del proyecto. No hay nuevas dependencias de modulos/plugins. La configuracion manual y los comandos temporales estan en [ObjectivesAndScoring.md](ObjectivesAndScoring.md).

## Compilacion

Motor instalado: **UE 5.6.1 CL44394996**, compatible 5.6.0, en `C:\Program Files\Epic Games\UE_5.6`. VS2022 Community, MSVC **14.44.35230**, Windows SDK **10.0.22621.0**, .NET incluido con el motor.

| Destino | Resultado | Tiempo de la ultima compilacion |
| --- | --- | --- |
| MyProjectEditor / Win64 / Development | Succeeded; DLL actualizada en el proyecto activo | 27.06 s |
| MyProject / Win64 / Development | Succeeded; ejecutable actualizado | 76.26 s |

Logs: `Saved/Logs/Build-MyProjectEditor.log` y `Saved/Logs/Build-MyProject.log`. No se hizo cook/empaquetado ni se compilo Shipping. UBT mantiene su aviso de toolchain no preferido: recomienda MSVC 14.38; la instalada es 14.44.

## Suite de automatizacion

`Scripts/Test-Movie.ps1` verifico un reporte nuevo del **2026.11.13-01.08.56 UTC** (19:08:56 del 12 de noviembre en Mexico): **28 Success, 0 succeededWithWarnings, 0 failed, 0 notRun, 0 inProcess**. Duracion de los casos segun el reporte: **0.986 s**, aparte del arranque del Editor. Los fixtures avanzan el tiempo de juego con ticks; no esperan las tomas completas en tiempo real.

Las **20 regresiones de fases anteriores** siguen pasando, incluidas grabacion concurrente/replays, loop 10/30, limpieza, CharacterMovement, orden/unicidad de eventos, animaciones, outfits independientes, UMG nativa y el skeleton existente.

| Caso nuevo | Verificacion |
| --- | --- |
| Objectives.DefinitionsAndValidation | Cinco objetivos y 100 puntos disponibles por rol; IDs duplicados, prerequisitos ausentes/ciclicos y payloads incompletos rechazados |
| Objectives.RobberRules | Preparacion sin scoring/interaccion, distancia de recogida, Dance sin bolsa/prerequisito no concede, E recoge, cada logro una vez, score/IDs guardados, siguiente rol en cero |
| Objectives.AllRolesAndScores | Los quince retos predeterminados, 100/100/100 y promedio 100; uso con gesto configurable sin duplicar eventos; uso presentado por el replay; premiere completa |
| Objectives.ProximityAtActionTime | Target con score cero, arresto lejano rechazado, pose grabada evaluada al tiempo de accion aunque el transform visual este atrasado; arresto teatral no cambia al target |
| Objectives.PropReplayAndReset | Timestamp compartido y un evento por pickup, mesh capturado independiente de cambios del catalogo, inventarios separados, replay no consume pickup vivo ni premia, volver al primer pose restaura prop inicial, nuevos takes/restart restauran props y limpian datos |
| Objectives.ZeroAverageAndCap | Tres roles con cero completan la pelicula; promedio fraccional 175/3; presupuesto custom excesivo limitado a 100 sin recompensas repetidas |
| Objectives.NativeHUD | Jerarquia UMG, preview en vestuario, role/timer/score, indicador completado/puntos, feedback y prompt E |
| Objectives.ZonesAndBadConfiguration | Zona rotada y referencia directa, definicion invalida expuesta sin bloquear las tres tomas |

Reporte: `Saved/Automation/Movie/index.json`. Log: `Saved/Logs/MovieTests.log`. Dos fallos iniciales de fixtures se corrigieron: el Robber de prueba escapaba antes de la accion del Customer, y el Police de otro caso empezaba ya dentro del radio de proximidad. No se relajaron las reglas para hacerlos pasar.

## Play con LocalPlayer, UMG y render

**`Scripts/Test-HeistPlay.ps1 -RenderScreenshots`: PASS.** Proceso separado `-game`, mapa existente, LocalPlayer/viewport reales, render D3D12 offscreen **1280x720**, sin guardar Content. Fixture temporal con **3 s de preparacion y 3 s de recording**; los defaults siguen siendo 10/30.

- HUD visible con los cinco objetivos correctos y score cero antes de cada toma.
- Los quince objetivos completados mediante movimiento y APIs de actuacion/interaccion sobre el Pawn vivo.
- Resultados guardados **[100, 100, 100]**; promedio **100.0**.
- Durante Take 2/3, props equipados independientes, scores previos conservados y pickups del jugador actual disponibles; replays sin colision.
- En premiere, cada uno de los tres replays despacho exactamente sus **tres eventos** grabados una vez: Robber (recogida, Dance, EvilLaugh), Police (Arrest, Use.Donut, PoliceSiren), Customer (Scared, Dance, Scream).
- El pickup de bolsa se oculto como presentacion de la recogida en premiere, y el HUD mostro el resultado final.
- Restart limpio performances, scores y objetivos; no se restauro el score anterior despues de probar el reinicio.

Reporte final: `Saved/Automation/HeistPlay/result.json`, **2026.11.13-01.14.23 UTC**. Log: `Saved/Logs/HeistPlay.log`.

Capturas inspeccionadas: [Preparacion](../Saved/Automation/HeistPlay/Preparation.png), [Robber](../Saved/Automation/HeistPlay/RobberHUD.png), [Police](../Saved/Automation/HeistPlay/PoliceHUD.png), [Customer](../Saved/Automation/HeistPlay/CustomerHUD.png), [Premiere y scores](../Saved/Automation/HeistPlay/PremiereScore.png). La prueba coloca los personajes muy cerca para validar proximidad; en esa captura pueden superponerse. Sus tres instancias, inventarios, outfits y eventos se verifican individualmente. En tu banco, separa las posiciones/actuaciones para componer el plano.

**`Scripts/Test-WardrobePlay.ps1 -RenderScreenshots`: PASS.** Regresion existente con tres UMG visibles, **2 Ready + 1 timeout**, input/camara restaurados, outfits y eventos independientes, premiere y restart. Reporte `Saved/Automation/WardrobePlay/result.json`, **2026.11.13-01.16.07 UTC**; log `Saved/Logs/WardrobePlay.log`.

Una ejecucion de esa regresion **sin render** fallo en el countdown visual: con NullRHI no se actualiza el NativeTick del widget. Se repitio con render y paso; las pruebas de logica/temporizadores sin UI tambien pasan en la suite de 28. Usa RenderScreenshots para la regresion visual de vestuario. No se cambio el sistema de vestuario para ocultar esa limitacion del runner.

## Archivos cambiados

Se aplicaron **38 archivos** al proyecto: 31 en Source, 2 en Scripts y 5 en Docs. Rutas relativas a `C:\Games\OneManStudio\MyProject`. En los pares Public/Private, `.h` esta en Public y `.cpp` en Private.

| Archivo(s) | Cambio |
| --- | --- |
| Public/ObjectiveTypes.h + Private/ObjectiveTypes.cpp | Nuevos: definiciones, estados, resultados y guion THE WORST BANK HEIST |
| Public/ObjectiveManagerComponent.h + Private/ObjectiveManagerComponent.cpp | Nuevos: scoring del Pawn vivo, validacion, prerequisitos, proximidad al tiempo del evento y delegates |
| Public/ObjectiveZone.h + Private/ObjectiveZone.cpp | Nuevos: volumen por tag/referencia con marcador opcional, sin overlaps fisicos |
| Public/InteractableProp.h + Private/InteractableProp.cpp | Nuevos: pickups configurables y clases MoneyBagProp, DonutProp, BananaProp |
| Public/PropInteractionComponent.h + Private/PropInteractionComponent.cpp | Nuevos: E, equipar/usar, eventos e inventario separado por actor |
| Public/ObjectiveHUDWidget.h + Private/ObjectiveHUDWidget.cpp | Nuevos: UMG nativa, datos/evento para subclass editable, feedback y resumen final |
| Private/Testing/HeistDemo.cpp | Nuevo: comando Heist.Demo; fixtures temporales opt-in, sin guardar mapa |
| Private/Testing/ObjectivePlayTest.cpp | Nuevo: runner opt-in separado con -game -ObjectiveSmoke; checks de HUD, quince objetivos, replays, premiere y reset; excluido de Shipping |
| Private/Tests/ObjectiveSystemTests.cpp | Nuevo: ocho pruebas de objetivos/props/scoring |
| Public/MovieManager.h + Private/MovieManager.cpp | Definiciones por rol, ObjectiveManager, HUD, componente de interaccion, binding E, resultados y reset de props integrados al bucle existente |
| Public/ActingTypes.h + Private/ActingTypes.cpp | CollectProp y UseProp agregados al final del enum; payload validado con IDs, posicion y snapshot visual |
| Public/ActingRecorderComponent.h + Private/ActingRecorderComponent.cpp | Entrada RecordEvent sobre el timestamp/reloj existente, con validacion y copia |
| Private/ActingRecording.cpp | Valida los tipos/payloads nuevos junto a eventos previos |
| Public/ActingComponent.h + Private/ActingComponent.cpp | Interacciones usan el dispatch existente; presentacion de uso opcional sin evento de gesto duplicado |
| Public/ReplayActor.h + Private/ReplayActor.cpp | Componente de interaccion, consulta de pose al scene time y restauracion de props al rebobinar/parar en primer pose |
| Public/WardrobeComponent.h + Private/WardrobeComponent.cpp | Aplicar solo visual del prop, resolver y congelar su definicion/materiales preservando los otros cosmetics |
| Public/WardrobeTypes.h | UseGestureId opcional en definicion de cosmetic; no cambia IDs existentes |
| Private/Tests/MovieManagerTests.cpp, Private/Tests/ActingEventTests.cpp | Conteos de bindings incluyen la nueva tecla E, manteniendo las comprobaciones de no duplicar |
| Scripts/Test-HeistPlay.ps1 | Nuevo: runner de Play con reporte fresco y render/capturas opcionales |
| Scripts/Test-Movie.ps1 | Exige los 28 casos y comprueba el reporte JSON fresco, no solo exit code |
| Docs/ObjectivesAndScoring.md, Docs/Phase5Verification.md | Nuevos: configuracion exacta, manual de pruebas y resultados |
| Docs/MovieManager.md, Docs/ActingEvents.md, Docs/Wardrobe.md | Referencias actualizadas a fase 5; los resultados historicos de otras fases se conservan |

Todas las rutas Public/Private anteriores estan dentro de **Source/MyProject/**. El README del workspace tambien se actualizo para apuntar a esta fase. Los outputs de compilacion, logs y capturas estan bajo Binaries/Intermediate/Saved, como en las fases previas.

## Limites y comprobacion manual

- Assets visuales de props y cosmetics siguen siendo placeholders configurables. No se crearon animaciones, sonidos ni arte final.
- Todas las pruebas se ejecutaron con **NoSound**. Verifican IDs, timestamps y dispatch, pero falta escuchar tus sonidos finales y revisar tus montages/slots in-place en tu banco. Se puede puntuar un gesto/sonido logico aun sin asset asignado.
- Las pruebas de Play mueven el Pawn y llaman APIs; no automatizan una partida humana con E/WASD ni revisan la distribucion artistica del mapa. La guia explica esa prueba manual y como evitar recoger otro objeto cuando se quiere usar el donut.
- La escena se guarda en memoria de la sesion; no se implemento persistencia entre ejecuciones, consumo/fisica de props ni generacion procedural.
- No hay bloqueos conocidos en los checks finales. Targets/zonas faltantes no bloquean la pelicula: esos objetivos quedan sin completar. Verifica que los volumenes incluyan el centro de la capsula y que los Role Id/Target Role Id coincidan.
- Arrancar el motor produce avisos preexistentes de dos iconos de VisionOS ausentes. La suite tambien registra tres condiciones internas de AutomationTest antes de iniciar los casos; el reporte de los 28 casos no contiene errores ni warnings. Estos mensajes de arranque no se ocultaron ni se modifico el motor.

La auditoria SHA-256 contra el estado previo a fase 5 mantiene los **250 archivos Content/Config** y `MyProject.uproject` sin cambios. La copia de trabajo se conserva en `Integration/Phase5/MyProject` dentro del workspace; abre el proyecto activo de C:\Games para usar la DLL compilada.
