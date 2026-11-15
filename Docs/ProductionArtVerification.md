# Verificación — integración de arte, 14 de noviembre de 2026

Proyecto real: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. UE **5.6.1**, Windows x64, compilador MSVC 14.44 y Windows SDK 22621.

## Resultado

El proyecto compila y hay un paquete **Windows Development** funcional en `Saved\Packages\ProductionArt\Windows`. Se verificó el ciclo completo dentro del ejecutable con los valores reales de **10 segundos de camerino y 30 segundos por toma**, incluyendo repetición, nueva película y vuelta al menú. El paquete anterior no se eliminó.

| Comprobación ejecutada | Resultado real | Evidencia en Saved |
|---|---|---|
| Development Editor / UnrealBuildTool | Correcto | Logs/Build-MyProjectEditor.log |
| Suite de lógica OneManStudio | **49 correctas, 0 fallos, 0 advertencias, 0 sin ejecutar** | Automation/ProductionArtUnitsFinal/index.json |
| Flujo completo en Editor, audio sintético | Correcto: 15 objetivos, 100/100, 5 estrellas; replay inmutable, película de cero objetivos, nuevo juego, menú y Quit | Automation/StudioPlayProductionArt/result.json |
| BuildCookRun Win64, cook SM5/SM6, stage, pak/IoStore y archive | **BUILD SUCCESSFUL, ExitCode=0**, 7 min 29 s | Logs/Package-Windows.log |
| Flujo completo empaquetado, tiempos reales y audio sintético | Correcto | Automation/StudioPackagedFullTimingProductionArt/result.json |
| Arte en ejecutable: 22 variantes/modelos del camerino y manos independientes | Correcto; **25 screenshots**, tres tomas, pantalla de cine, eventos independientes y reinicio | Automation/ProductionArtPackagedSurvey/result.json |
| Controles reales de ratón en el camerino empaquetado | Correcto: tabs, selecciones, ambos objetos, giro, aleatorio, restablecer | Automation/CustomizationPackagedSurvey/result.json |
| Movimiento, salto y cinco gestos grabados/reproducidos en el ejecutable | Correcto | Automation/CustomizationPackagedMotion/result.json |
| Videos: reproducción automática de las dos introducciones, HUB en bucle y opciones | Correcto; carga real del nivel y limpieza de medios | Automation/StartupPackagedAuto/result.json |
| Click durante introducción de estudio | Correcto: omite el resto y consume el click | Automation/StartupPackagedSkipStudio/result.json |
| Click durante introducción del juego | Correcto: omite el resto y consume el click | Automation/StartupPackagedSkipGame/result.json |
| Videos ausentes/fallidos y fondo del HUB fallido | Correcto: avanza/mantiene menú operativo, fade y carga de gameplay | Automation/StartupPackagedFailures/result.json |
| Protección de archivos originales | Correcto: ninguna pérdida ni cambio inesperado de Content | ProductionArt/FilesChanged.json |

Después de mover las lámparas del cine a los laterales para despejar la pantalla, se repitió BuildCookRun: **39.77 s, BUILD SUCCESSFUL, ExitCode=0**. El paquete resultante volvió a pasar las 22 selecciones, tres tomas, eventos, cine y reinicio, con 25 capturas en `Automation/ProductionArtPackagedFinal`. No se modificó código de gameplay en ese ajuste final de decorado.

El compilador instalado produce el aviso conocido de que 14.44 no es la versión preferida por UE 5.6. **No impidió compilar ni empaquetar**. No se cambió la instalación de Unreal ni Visual Studio.

La prueba completa empaquetada observó ocho aperturas de camerino entre sus escenarios, seis confirmaciones tempranas y una por timeout, tres tarjetas de título y tres reproducciones. Verificó que Replay conserva exactamente grabaciones/resultados, que Record New Movie elimina datos anteriores y que la película sin objetivos recibe una estrella. Los tonos de música, anuncios y acting de esta prueba se generan únicamente por el driver opt-in; no son audio final del juego.

## Qué cambió

### Source

Nuevos:

- `Public/ProductionArtCatalog.h`, `Private/ProductionArtCatalog.cpp`: catálogo de referencias importadas que conserva IDs y tags existentes.
- `Public/StudioCinemaPresenter.h`, `Private/StudioCinemaPresenter.cpp`: pantalla de cine y cámara del público sobre la reproducción existente, listener de audio y limpieza.
- `Public/StudioRailRig.h`, `Private/StudioRailRig.cpp`: plataforma/cámara que lee el reloj compartido.
- `Private/Testing/ProductionArtSurvey.cpp`: driver opt-in para inspección visual, materialización del catálogo cocinado y prueba de cine/reinicio.

Modificados:

- `MyProject.Build.cs`: módulo estándar CinematicCamera; sin dependencias externas de runtime.
- `WardrobeTypes.h/.cpp`: offset independiente de mano izquierda, validación y combinación de arte sobre IDs existentes.
- `WardrobeComponent.cpp`: conserva/restaura el ajuste izquierdo al copiar apariencias.
- `WardrobePreviewActor.cpp`: encuadra los bounds del cuerpo y cosméticos para no cortar gorros altos.
- `UnifiedWardrobeWidget.h/.cpp`: tamaño de texto de opciones configurable y ajuste de iconos/texto.
- `StudioCharacterAssetTools.h/.cpp`: utilidades de normalización, thumbnails y transformación de referencia; funciones de importación protegidas para Editor.
- `Testing/StudioFlowPlayTest.cpp`: comprueba la proyección, cámara original, captura y limpieza al cambiar estados.
- `Tests/UnifiedCustomizationTests.cpp`, `Tests/WardrobeTests.cpp`: catálogo ampliado, assets reales, independencia de manos y fixtures de rig genérico.

El recorder, ReplayActor, reloj compartido, objetivos, cálculo de puntuación, MovieManager y control de videos **no se reescribieron**.

### Content

Se agregaron **301 archivos de Content**: mallas/materiales/texturas/iconos nuevos, catálogo y `Lvl_Cinema`. El inventario detallado está en `Saved/ProductionArt/AssetInventory.csv` y la lista exacta de archivos en `FilesChanged.json`.

Solo dos archivos de Content que ya existían cambiaron:

1. `Content/OneManStudio/Maps/Lvl_Bank.umap`: colocación de arte, remplazo de las dos piezas de counter, actualización visual de pickups, zona adicional de counter y subnivel de cine.
2. `Content/OneManStudio/Characters/Base3/Customization/DA_Base3Character.uasset`: separación de la superficie facial a escala 1.015 para reducir z-fighting.

**No cambió ningún Blueprint existente**, textura facial, malla/esqueleto Base3 ni animación previa. Los originales están respaldados bajo `Saved/ProductionArt/Backups`; el informe compara hashes con el estado anterior a la integración. No se eliminaron archivos originales.

### Scripts y documentación

Nuevos: `Prepare-ProductionArt.py`, `Inspect-FbxGeometry.py`, `Optimize-ProductionProps.py`, `Import-ProductionArt.py`, `Configure-ProductionCosmetics.py`, `Build-ProductionSets.py`, `Inspect-ProductionScene.py`, `Finalize-ProductionArt.py`, `Report-ProductionArt.py` y esta documentación. `Package-Windows.ps1` ahora archiva en ProductionArt e incluye explícitamente las carpetas de entorno en el cook.

## Límites y trabajo artístico restante

- El paquete se probó en esta PC; no se hizo una instalación en una segunda PC limpia ni una certificación de hardware mínimo. Es Development, no una publicación Shipping ni una subida a la jam.
- Se conservó el encuadre original de tu cámara final. El riel existe y sigue el reloj; su ángulo alternativo queda opcional/desactivado. Las pruebas del ciclo usan la cámara final original.
- Bóveda/riel/dolly/pantalla/cortinas/cajas/lingotes emplean geometría sencilla donde el archivo suministrado no tenía modelos independientes.
- Las variantes de ropa son texturas compatibles con la misma malla; el vestido no incorpora una falda geométrica nueva. Los lentes conservan el aspecto opaco del material recibido. Puedes refinar materiales y offsets en el catálogo.
- Las animaciones provisionales y referencias de audio existentes se preservaron. Los seis SFX, música y locuciones finales deben asignarse desde las interfaces ya expuestas; el test con tonos prueba el mecanismo y la limpieza, no su calidad artística.
- Los marcadores de público/cajero/clientes/guardia son preparación para NPCs; no se agregó IA ni otro personaje jugable.
- La pantalla reproduce los actores en tiempo real: no produce un archivo MP4.

No quedaron bloqueos de compilación, empaquetado ni del ciclo principal en las pruebas ejecutadas. La guía de edición y asignación está en `Docs/ProductionArtIntegration.md`.
