# Pruebas de la interfaz final — 15 de noviembre de 2026

## Pruebas ejecutadas en Development Editor

| Prueba | Resultado real | Alcance |
| --- | --- | --- |
| Batería OneManStudio | 69 aprobadas; 0 fallos | Datos, relojes, replays, colisión, eventos, vestuario, objetivos, puntuación, seis turnos, localización y UI |
| SkipStudio a 1280×720 | Aprobada | Clic real de Slate, protección del salto, bucle, paneles, viaje real, Listo y movimiento |
| Auto a 1920×1080 | Aprobada | Ambos vídeos en orden, bucles, ajustes/idioma/créditos/estudio, fundido y viaje |
| SkipGame a 1920×1080 | Aprobada | Salto del segundo vídeo sin accionar botones |
| Failures a 1920×1080 | Aprobada | Medios ausentes/decodificación fallida; menú funcional con fondo negro |
| CustomizationLayout | 39 pasos y 10 capturas; aprobada | Ocho categorías, dos manos, rotación 3D, aleatorio, restablecer y aislamiento del camerino |
| ActingAudio | Aprobada | Modelo actual, andar/correr/saltar, cinco gestos, seis sonidos y grabar una segunda toma mientras otra se reproduce |
| Story Police | 6 turnos, 3 camerinos y 2 películas; aprobada | Recuperación policial, gestos/sonidos, ropa independiente, secuestro de dinero evitado, reset y menú |
| Story Civilian | 6 turnos, 3 camerinos y 2 películas; aprobada | Final del civil, lanzamiento físico real de donut, eventos, ropa, reset y menú |
| Story Failure | 6 turnos, 3 camerinos y 2 películas; aprobada | 60 segundos sin acciones, cero objetivos, película válida, puntuación y reset |
| Cobertura ES/EN | 305 traducciones; 0 ausentes | Catálogo compilado y navegación con cambios de idioma |

Los JSON están en `../Tests/FinalUI`. La prueba de escenario mal configurado
produce deliberadamente una advertencia de preparación de reparto; cuenta como
éxito con advertencia, no como fallo. Son 68 éxitos y un éxito con advertencia.

Las pruebas de vídeo usan entrada de puntero en Slate con pulsación y liberación
en fotogramas distintos. Las pruebas de historia mueven los personajes mediante
entrada W y llaman a la interacción existente al llegar a alcance; no son una
sesión de juego humana. Comprueban cursor de eventos, sonidos iniciados por cada
actor del estreno, independencia de ropa y conservación de puntos en la repetición.

La prueba aislada de personalización usa el modo simultáneo anterior y un tiempo
de preparación ampliado. Sus capturas no representan las duraciones del modo
normal de seis turnos; las capturas WardrobeRobber/Police/Civilian sí lo representan.

## Capturas

Las imágenes de `../Screenshots/FinalUI` proceden de ejecuciones reales. El archivo
Screenshots.json registra resolución, prueba de origen y SHA256. No son maquetas.
Se incluyen menús y estudio en 720p/1080p, ajustes ES/EN, estados de botón, tres
vestuarios, dos manos, cine, cámara dirigida y tres desenlaces.

## Distribución

La primera pasada de cocinado completó UAT pero omitió los recursos dinámicos de
UI. Se descartó como distribución final. Las carpetas se añadieron a
DirectoriesToAlwaysCook y el script comprueba cada paquete UI en el manifiesto
de cocinado antes de declarar éxito. Esta propiedad admite rutas de paquetes
como /Game, según la [documentación de Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Developer/DeveloperToolSettings/UProjectPackagingSettings).

Pasaron además las pruebas **Windows Development empaquetadas**, fuera del Editor:
Auto con entrada real de Slate y todos los recursos de tema/fuente requeridos;
Police y Civilian con seis turnos, tres camerinos, dos películas, ropa/eventos
independientes, nueva sesión, regreso al menú y cambio español/inglés. Civilian
incluye un lanzamiento físico real. Los informes se llaman StartupPackagedAuto,
FinalUIPackagedPolice y FinalUIPackagedCivilian. La captura PackagedRecordingHUD
muestra la actuación en curso y PackagedResultsEnglish verifica el resultado
traducido. La palabra "packaged" aquí no implica Shipping.

La prueba manual del ejecutable Shipping encontró Windows bloqueado. Se cerró
únicamente el proceso de prueba propio. No se intentó desbloquear la sesión ni
actuar sobre autenticación. El estado de la validación empaquetada y los ZIP
definitivos se registra en el informe de entrega; no se infiere de UAT ni de una
sesión del Editor.
