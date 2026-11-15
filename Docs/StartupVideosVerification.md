# Verificacion del arranque con tres videos

Fecha: **13 noviembre 2026**. Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. Unreal **5.6.1**, Windows x64, DirectX 12. MSVC **14.44.35230**, Windows SDK **10.0.22621.0**.

## Resultado implementado

**DUTIX Studio > introduccion del juego > HUB con fondo en loop > Start Game > negro > carga real del banco > vestuario de Take 1.** Un clic izquierdo durante cualquiera de las intros salta todo lo restante. Se consume el clic, se espera su liberacion y se mantiene bloqueada la entrada durante el fundido. El mismo reproductor del HUB sigue activo al navegar entre menu, instrucciones, creditos y audio. Su pista respeta Music/Master. Al comenzar el juego se cierran reproductor y audio antes del viaje.

Las tres fuentes y el nivel de juego son editables en **StudioFlowManager > StartupVideos > Settings**. El layout del menu y la arquitectura MovieManager/recording/replay se conservan. Instrucciones exactas: [StartupVideos.md](StartupVideos.md).

## Builds y pruebas reales

| Verificacion | Resultado |
| --- | --- |
| Build MyProjectEditor Development | **PASS**, compilacion final 30.42 s |
| Automation OneManStudio | **44/44 PASS**, 0 failures, 0 warnings; informe final UTC 17:29:23 |
| Build MyProject Win64 Development | **PASS**, 75.28 s |
| BuildCookRun, cook, stage, pak/IoStore y archive | **PASS**, ExitCode 0; total 119.95 s |
| StartupPlayAuto, render real, 1280 x 800 | **PASS** |
| StartupPlaySkipStudio y StartupPlaySkipGame | **PASS**, clic real mantenido y liberado sin activar Start |
| StartupPlayFailures | **PASS**, referencias vacias y decoder rechazando un archivo MP4 corrupto |
| StartupPackagedAuto, Windows cocinado, 1920 x 1080 | **PASS** |
| StartupPackagedSkipStudio y StartupPackagedSkipGame, 1280 x 800 | **PASS** |
| StartupPackagedFailures | **PASS**, menu negro funcional y viaje al juego |
| StudioPlayVideoRegression con audio | **PASS**, regresion del flujo completo anterior |
| StudioPackagedStartupRegression con audio, Windows cocinado | **PASS**, tres tomas, premiere, resultados, replay, nueva pelicula, menu y Quit |

Los cuatro escenarios de video se ejecutaron con audio habilitado y sin NullRHI. Usan entrada de puntero de Slate, botones reales y **OpenLevel**, sin reemplazar el GameMode ni pasar un mapa para arrancar. Auto verifica las dos intros en orden. Los dos escenarios de skip pulsan sobre la futura zona de Start Game y mantienen el boton pulsado a traves de la transicion: no comienza el juego hasta un clic posterior deliberado. Cada caso normal navega How to Play, Credits y Audio sin cambiar el MediaPlayer y espera al menos dos bucles del fondo. Despues verifica negro, nueva instancia del nivel, cierre de media, Ready y movimiento real con W.

Failures prueba fuentes ausentes, error real de Windows Media Foundation tanto en la primera como en la segunda intro y fallo del video del HUB. Estos errores de decoder son intencionales, se registran en esos logs y no impiden usar el menu ni empezar el juego. Las configuraciones de prueba son transitorias; no cambian los assets.

Se inspeccionaron las capturas de DUTIX, introduccion, HUB, opciones y vestuario. Las capturas 1280 x 800 muestran ajuste 16:9 con bandas negras; las del paquete 1920 x 1080 muestran el menu por encima del video y el banco despues de viajar. Se comprobo una captura negra durante la transicion. El primer intento del driver detecto que WMF informa brevemente un estado no-playing durante su seek de loop; se verifica que el mismo reproductor reanude en menos de 0.75 s. Tambien se corrigio una referencia fuerte del propio driver que retenia el nivel antiguo durante el diagnostico de viaje. No queda esa referencia en la prueba final.

Una prueba de regresion especifica comprueba que una notificacion de fin de video durante el fade de skip no pueda redirigir a la segunda intro. Las otras 40 pruebas cubren los sistemas anteriores de movimiento, eventos, wardrobe, props, objetivos, scores, premiere y flow.

La regresion Studio se ejecuto con sus tiempos acelerados (grabaciones de 5 s), verificando al inicio que los defaults siguen siendo 10/30. Paso en standalone del Editor y en el paquete: 8 aperturas de vestuario, 6 inicios Ready, 1 timeout, 3 title cards/playbacks, las cinco voces del director, primera pelicula 100/100 y 5 estrellas, una pelicula de cero objetivos, reinicio, replay sin cambios de scores, limpieza de nueva pelicula, regreso al menu, volver a empezar y Quit. Su bypass StudioSkipStartupVideos aisla ese recorrido previo; las cuatro pruebas StartupPackaged* comprueban por separado el arranque real con video, sin bypass. En esta entrega no se repitio todo el recorrido con cada duracion de 30 s.

## Distribucion y arte preservado

Paquete nuevo: `Saved/Packages/StartupVideos/Windows`. Ejecuta **MyProject.exe** o **Play-ONE-MAN-STUDIO.bat**. Se conservan los paquetes Phase6, BankIntegration y Phase7. Los tres MP4 aparecen en **Manifest_NonUFSFiles_Win64.txt** y se distribuyen como archivos locales en `MyProject/Content/Movies/Startup`. Las fuentes/material/textura estan incluidos en el cook/IoStore. Los logs del paquete abren las rutas de Movies del propio paquete, no las de Downloads ni las de Content del proyecto fuente.

Se verificaron **264 archivos originales de Content, Config y .uproject sin cambios de hash**. No se guardo ningun cambio en tus mapas, personajes, materiales previos ni Blueprints. Las copias de video conservan el SHA-256 de sus originales de Downloads.

## Archivos de esta entrega

Nuevos C++ en Source/MyProject:

- Public/StartupVideoComponent.h y Private/StartupVideoComponent.cpp: secuencia, loop, volumen, fallbacks, guardas de input y eventos Blueprint.
- Public/StartupVideoWidget.h y Private/StartupVideoWidget.cpp: presentacion con proporcion original, capa negra y captura del clic.
- Public/StartupTravelSubsystem.h y Private/StartupTravelSubsystem.cpp: capa negra persistente entre niveles y memoria de intros por ejecucion.
- Private/Tests/StartupVideoTests.cpp: cuatro pruebas de defaults, fallbacks, skip y estado de viaje.
- Private/Testing/StartupVideoPlayTest.cpp: prueba integrada de video y viaje, excluida de Shipping.

C++ ampliado: **MyProject.Build.cs** (MediaAssets/AudioMixer del motor), **StudioFlowManager.h/.cpp**, **StudioFlowTypes.h** (estados nuevos agregados al final), **StudioFlowWidget.h/.cpp** (fondo, habilitacion y etiqueta Start Game editable). Los archivos de MovieManager, recorder, ReplayActor y objetivos no se reescribieron.

Scripts nuevos: **Create-StartupVideoAssets.py**, **Test-StartupVideos.ps1**. Scripts ampliados: **Test-Movie.ps1** (44 casos), **Test-StudioFlow.ps1** (bypass solo para aislar la regresion previa), **Package-Windows.ps1** (directorio nuevo y cook de startup).

Content nuevo, limitado a esta funcionalidad:

- Movies/Startup/**StudioIntro.mp4**, **GameIntro.mp4**, **HubLoop.mp4**.
- OneManStudio/UI/StartupVideos/**MS_StudioIntro**, **MS_GameIntro**, **MS_HubLoop**, **MT_StartupVideo**, **M_StartupVideoUI**.

Docs: **StartupVideos.md**, **StartupVideosVerification.md** y actualizacion de **MainMenuAndGameFlow.md**.

## Evidencia y limites

Informes JSON y capturas: `Saved/Automation/StartupPlay*`, `Saved/Automation/StartupPackaged*`, `Saved/Automation/Movie/index.json`, `Saved/Automation/StudioPlayVideoRegression/result.json` y `Saved/Automation/StudioPackagedStartupRegression/result.json`. Logs: `Saved/Logs/Startup*.engine.log`, `Package-Windows.log` y los logs de build. La copia de auditoria de esta integracion esta en el workspace `Saved/StartupVideos`.

Se verifico este equipo Windows y el paquete **Development**; no se afirma una prueba Shipping ni una prueba en un equipo limpio. La reproduccion usa los codecs de Windows Media Foundation del sistema. No se hizo una evaluacion auditiva humana ni captura continua de todos los fotogramas: las comprobaciones de audio son de estado, volumen/limpieza y reproduccion del motor, ademas de capturas visuales puntuales. La advertencia de UBT sobre MSVC 14.44 frente al preferido 14.38 no impidio compilar ni empaquetar.

Para cambiar de nivel debes incluir el nuevo mapa en el cook y conservar/configurar el GameMode de juego. Un Widget Blueprint con Designer propio debe hacer transparentes sus fondos y consumir los estados nuevos segun StartupVideos.md. El fallback nativo ya esta operativo sin montaje UMG manual.
