# Verificación — correcciones finales y audio

Las pruebas se ejecutaron sobre `C:\Games\OneManStudio\MyProject\MyProject.uproject`, con UE 5.6.1 y MSVC 14.44. No se usan servicios de IA ni Blender durante el juego. El paquete anterior de `Saved/Packages/ProductionArt` se conserva.

## Resultados comprobados en Editor

| Prueba | Resultado y evidencia en `Saved/Automation` |
|---|---|
| Development Editor / UBT | Compilación correcta del candidato; registro de construcción del proyecto |
| Lógica, datos y configuración | **52 correctas, 0 fallos** en `Movie/index.json`; incluye referencias de los 34 audios, máscaras individuales, transforms de referencia y pasos sin premios de soundboard |
| Antifaz | Frente, ambos perfiles y dos tres cuartos en `CorrectedMaskViews`; combinación gorro+antifaz animada en `CorrectionsMotion/BeanieMask` |
| Sombreros, accesorios, props | Seis combinaciones con idle, caminar, correr, salto, cinco gestos y replay simultáneo: `CorrectionsMotion/{PoliceCap,BeanieMask,Crown,Cowboy,Chicken,Mask}` |
| Ajuste final de patillas/lentes | Cinco vistas en `CorrectionsViewsFinal/Glasses`; movimiento, gestos, ambas manos y replay en `CorrectionsMotionFinal` |
| Prendas y selección | 23 opciones/modelos de arte, seis prendas con piel morada, tres tomas y reinicio en `FinalCorrectionsArt2`; 26 capturas |
| Cine y encuadre final | `FinalCinemaCheck/result.json` y `CinemaMovie.png`; misma cámara/pantalla, tres actores, eventos sincronizados y reinicio |
| Juego completo con audio proporcionado | `StudioPlayFinalCandidate/result.json`: menú, audio, guion, camerinos, movimiento real, 15 objetivos, 100/100, cinco estrellas, replay inmutable, película con cero objetivos, nueva película, menú, volver a jugar y Quit |
| Señal real de audio | WAV de salida en `StudioPlayFinalAudio2`, `StudioPlayFinalCandidate` y `CorrectionsMotionFinal`; análisis de muestras en `Saved/FinalCorrections/AudioMixVerification.json` |

Las primeras capturas fuera de pantalla salieron mudas por `UnfocusedVolumeMultiplier=0` de Unreal. Las pruebas optativas ahora habilitan audio sin foco en su propio proceso. La repetición produjo muestras reales; la captura de 61,72 s de `StudioPlayFinalAudio2` tuvo pico **0,16276**, sin muestras saturadas. Los controles de esa prueba estaban en Master 50%, Música 25% y Efectos 75%. No es una escucha humana ni una prueba de todos los dispositivos; el dispositivo usado expone ocho canales. Las preferencias normales del juego se conservan.

## Windows

Paquete **Windows x64 Development** en `Saved/Packages/FinalCorrections/Windows`. UAT/UBT, cocinado, staging y archivado finalizaron con **BUILD SUCCESSFUL, ExitCode=0**. La última construcción incremental tardó 38 s; la construcción completa inicial, 195,62 s. No se sustituyó el paquete anterior. El juego no requiere Unreal Editor, Blender ni una API de IA para ejecutarse.

| Prueba del ejecutable cocinado | Resultado y evidencia en `Saved/Automation` |
|---|---|
| Ciclo final con duraciones reales | `StudioPackagedFullTimingBufferedFinal/result.json`: **correcto**, 10 s de camerino / 30 s de toma, tres actuaciones; 601 muestras por toma, 20 Hz, duración final 30 s. Repite el ciclo con la configuración de audio final |
| Candidato final con cola de audio actualizada | `StudioPackagedFinalBuffers/result.json`: **correcto**, controles de audio, ocho camerinos ejercitados entre películas/reinicios, Ready y timeout, tres estrenos, replay, película sin objetivos, nueva película, menú y Quit |
| Objetivos y resultados | 15 objetivos, 100/100 y cinco estrellas; otra película con cero objetivos se puede ver y obtiene su resultado; replay conserva grabaciones y puntuaciones |
| Arte y sincronización | `FinalPackagedArt/result.json`: **correcto**, 23 opciones/modelos, 26 capturas; tres actores con sus apariencias, eventos separados, cine y reinicio |
| Locomoción, accesorios y audio de actuación | `FinalPackagedMotion/result.json`: **correcto**, caminar/correr/saltar, cinco gestos, seis sonidos, grabación de segunda toma mientras se reproduce la primera; WAV real de salida |
| Camerino con puntero real de Slate | `CustomizationPackagedSurvey/result.json`: **correcto**, 39 pasos, tabs, ambas manos, rotación del retrato, aleatorio, restablecer y aislamiento durante preparación |
| Videos automáticos y HUB | `StartupPackagedAuto/result.json`: **correcto**, dos introducciones, loop de HUB, opciones sin reiniciar video, fade, carga real de nivel y entrada de juego |
| Salto de introducciones | `StartupPackagedSkipStudio` y `StartupPackagedSkipGame`: **correctos**, click consumido, sin activar accidentalmente el menú |
| Medios ausentes o fallidos | `StartupPackagedFailures/result.json`: **correcto**, continúa al HUB, menú funcional, transición a juego y cierre del audio/video |

Las pruebas de flujo rápido usan 2/5 s solo en su proceso; el recorrido de arte extiende el camerino para capturar cada opción. Los valores del mapa permanecen en **10/30 s**. La validación usa el ejecutable cocinado con la GPU y el dispositivo virtual de audio de este equipo, fuera de pantalla. No equivale a una prueba en otro PC limpio ni a certificar todos los dispositivos, resoluciones o combinaciones cosméticas.

La captura Windows previa de 312,30 s contiene señal real: pico 0,17844 y **0 muestras saturadas**. El candidato final rápido tiene pico 0,16571; la prueba de actuación, 0,32217, ambos sin saturación. El análisis completo está en `Saved/FinalCorrections/AudioMixVerification.json`. En la prueba larga anterior hubo cuatro avisos de falta de datos del dispositivo virtual; se aumentó `AudioNumBuffersToEnqueue` de 1 a 3. La repetición rápida no emitió ese aviso; la repetición larga final todavía registró **tres**. No se considera resuelta la posible discontinuidad en ese dispositivo: requiere escucha/prueba en la salida de audio objetivo. No se observó fallo del flujo ni duplicación de eventos.

Durante la primera prueba larga se detectó una comprobación adelantada respecto al tick de interpolación: el test comparaba eventos futuros con la reproducción ya evaluada. Se corrigió para observar el tiempo efectivamente procesado por el replay y se comprueba también que al terminar no falte ni se duplique ningún evento. No se creó otro reloj ni se alteró el interpolador. Los pasos automáticos requieren además velocidad real para evitar falsos pasos por pequeños ajustes físicos.

## Protección y archivos

`Saved/FinalCorrections/FilesChanged.json` compara Source, Config, Scripts, Docs y Content contra el inicio de esta tarea. Solo se autorizan como Content preexistente modificado los mapas `Lvl_Bank`, `Lvl_Cinema` y `DA_ProductionCosmetics`. No se modifican los Blueprints ni los originales de personaje, ropa, gorros o accesorios. Las correcciones de malla/material son assets nuevos.

`Saved/FinalCorrections/AudioAssignments.csv` registra los 34 WAV y su fuente. Los originales de audio y los tres archivos fuente de la bóveda conservan sus SHA-256. Las instrucciones de edición y las limitaciones de arte están en `FinalCorrections.md`.

Los archivos principales añadidos son `StudioAssetCorrectionTools.{h,cpp}`, `StudioAudioProfile.{h,cpp}` y `Tests/FinalAssetTests.cpp`. Se extendieron `ActingComponent`, `ActingTypes`, `WardrobeComponent`, `WardrobeTypes`, `MovieManager`, `MovieManagerPresentation`, `StudioFlowManager`, `StudioFlowTypes`, `StudioFlowWidget`, `StudioAudioSubsystem` y `StartupVideoComponent`. `UnifiedWardrobeWidget` solo conecta el click existente; `ReplayActor.h` añade una consulta del tiempo ya evaluado. También cambian herramientas de importación, pruebas y empaquetado. El manifiesto enumera cada archivo y distingue nuevos de modificados.

No se cambiaron los Blueprints, el esqueleto, las animaciones ni las mallas/texturas importadas originales. Las variantes corregidas de antifaz, patillas, materiales y máscaras son assets nuevos. Los únicos assets Content anteriores editados son los dos mapas y `DA_ProductionCosmetics`. No quedan referencias de audio obligatorias sin asignar; la UI final del director sigue siendo trabajo de arte pendiente, con la interfaz funcional actual conservada.
