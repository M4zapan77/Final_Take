> HISTÓRICO: anterior al banco y correcciones finales. Leer DELIVERY.md.

# FINAL TAKE 1.0 — entrega final

Checkpoint de código y configuración del paquete: `063c78040fe12ebb14a06d2c45f54eec0c61e785`.
Rama local completa: `final-take/final-ui`. UE 5.6.1, Windows 64 bits.

## Entrega jugable y proyecto

- Carpeta completa: `output/release/FINAL_TAKE_Final_UI_2026-11-15/Windows` del espacio de trabajo.
- ZIP jugable: `FINAL_TAKE_Final_UI_Windows.zip`, **573.97 MB**, 32 archivos, CRC verificado.
- SHA256: `be2b12a2eaa0e8834597486a901ac7e7cfb2a65cee4d1fd80e5bbec57ec7217e`.
- Proyecto editable: `FINAL_TAKE_Final_UI_Project.zip`, con MyProject.uproject, Source, Content, SourceArt, configuración, scripts, traducciones y documentación. Su recibo JSON y SHA256 están junto al ZIP; no incluye .git, Saved, Binaries, Intermediate ni cachés.
- Iniciar: extraer toda la carpeta Windows y ejecutar Play-FINAL-TAKE.bat. En el espacio de trabajo también existe JUGAR_FINAL_TAKE.bat.

Los archivos de distribución están fuera de los cachés limpiados. Se conservan
los respaldos PreGameplayUpdate y Gameplay_Optimized como versiones históricas,
además del archivo de recuperación de la limpieza de recursos.

## Resultados ejecutados

- Compilación Development Editor aprobada; 69 pruebas automáticas aprobadas (68 éxitos y un éxito con advertencia esperada).
- Windows Shipping empaquetado con Pak/IoStore y compresión: UAT terminó con código 0.
- Los 47 paquetes UI están en el manifiesto cocinado; no falta ninguno.
- Tras limpiar cachés, el proyecto volvió a abrir y pasaron de nuevo las 69 pruebas. El Shipping final permaneció activo 20.76 segundos en arranque offscreen sin intervención; eso sólo verifica estabilidad de arranque.
- 12 ejecuciones nativas aprobadas: vídeos, saltos y fallos de medios; menús a 720p/1080p; personalización; animación/audio; tres desenlaces en Editor; vídeos y dos historias en Windows Development empaquetado.
- En las historias se verifican seis turnos, tres camerinos, dos películas, gestos/sonidos/ropa por actor, eventos sin duplicar, puntuación estable y nueva sesión limpia. El civil lanza un objeto físico real. El caso sin objetivos sigue siendo visible y puntuable.
- Cobertura ES/EN: 305 traducciones, cero ausentes. El idioma se cambia durante resultados y se conserva al iniciar otra película.
- 21 capturas reales, incluida UI empaquetada, HUD de actuación, resultados en inglés y película de cero objetivos.

La prueba manual de Shipping con ratón y teclado quedó **pendiente porque Windows
estaba bloqueado**. La copia Development empaquetada sí completó pruebas automáticas
fuera del Editor. No se presenta esa ejecución como una partida humana en Shipping,
ni se afirma haber probado otro ordenador o una instalación limpia de Windows.

## Limpieza y tamaños

La limpieza final eliminó **13.69 GB** de paquetes antiguos,
staging, temporales e intermediarios regenerables. Se comprobaron rutas absolutas,
ausencia de enlaces/junctions y ausencia de archivos seguidos por Git. Se conservaron
Source, Content, SourceArt, Binaries, .git, DDC, preferencias, guardados, autosaves,
logs, informes y cachés útiles de shaders. El próximo build completo regenerará
intermediarios; el proyecto editable se distribuye sin ellos.

La limpieza anterior auditó y retiró 16 assets de plantilla (2 210 671 bytes) y
45 copias de archivos ZIP/RAR (1 736 161 593 bytes). Todos están en un ZIP de
recuperación con CRC/SHA256 fuera del proyecto; los FBX/OBJ/texturas extraídos siguen
disponibles para reimportación. Se retuvieron 80 recursos de maniquí para fallback
y retargeting. Sólo se desactivó el plugin GameplayStateTree sin dependencias.

El ZIP Windows original era 629.34 MB; el final es 573.97 MB.
El proyecto editable contiene arte nuevo y evidencias: no se afirma que su fuente
canónica sea más pequeña por añadir funciones. El recibo de su ZIP contiene su
tamaño exacto y el desglose de carpetas.

## Medidas de rendimiento y sus límites

En el mismo caso estático M7, los buffers de poses pasaron de 283136
a 19040 bytes, con trayectorias de props conservadas.
Es una medición de grabación estática, no de toda la RAM. La corrección final
mantiene la captura de ajustes activa para que gestos/sonidos funcionen.

Windows Development empaquetado, DX12 SM6, 1280×720 offscreen, Ryzen 7 7840HS,
16 GB y RTX 4060 Laptop seleccionada:

| Caso | Grabación media/p95 | Película media/p95 | Pico de RAM observado |
| --- | --- | --- | --- |
| Policía | 24.32/31.51 ms | 13.95/15.36 ms | 2057.86 MiB |
| Civil | 19.46/18.25 ms | 13.82/15.10 ms | 2078.33 MiB |

Se observaron picos de carga/cambio de actor y capturas, incluidos fotogramas de
aproximadamente un segundo. Las medias incluyen esos picos. No se demuestra una
mejora general de FPS ni se garantizan 60 FPS; no hay benchmark Shipping completo.
Los JSON conservan las muestras resumidas, máximos y percentiles reales.
En el arranque Shipping de 20 segundos se observaron 2234.79 MiB
de pico de memoria de proceso. Es una observación de inicio, no de toda la película.

## Configuración pendiente y límites

- Falta una portada oficial de la película del banco; el logo aprobado sirve de alternativa. Asignar MovieCover en DA_FinalTakeUITheme es opcional.
- El retrato usa el modelo 3D real sobre fondo negro. Encuadre, colores, texturas, iconos y widgets derivados siguen configurables por el artista.
- No hacen falta Widget Blueprints manuales para jugar. La guía Docs/FinalUI-EditorSetup.md detalla clases y conexiones si se sustituye la disposición nativa.
- Los documentos históricos pueden describir el antiguo modo de tres tomas; README y los informes actuales describen el modo normal de seis turnos.
- Las fuentes de reimportación están incluidas; si se extrae el proyecto a otra ubicación, Unreal puede requerir reasignar rutas de reimportación a SourceArt.
- No hay multijugador, dependencia de API de IA en tiempo de ejecución ni exportación MP4.
- GitHub recibe código/configuración/traducciones/evidencias. No se suben los varios GB de arte/vídeos ni ZIP de distribución al repositorio ordinario. El ZIP local es el proyecto completo.

Consulta UI/Integration.md para archivos modificados, UI/Testing.md para alcance
de cada prueba y Optimization/FinalStorageCleanup.json para cada ruta eliminada.
