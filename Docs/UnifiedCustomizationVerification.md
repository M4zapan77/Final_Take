# Verificación del camerino unificado

Entrega verificada con Unreal Engine **5.6.1**, Windows 11, DX12, MSVC **14.44.35230** y Windows SDK **10.0.22621.0**. El proyecto activo es `C:\Games\OneManStudio\MyProject\MyProject.uproject`. La guía de uso, propiedades y configuración UMG está en [UnifiedCustomization.md](UnifiedCustomization.md).

## Resultado implementado

- Un solo camerino nativo UMG reemplaza al anterior en el flujo normal: ocho pestañas, seis colores de piel configurables, miniaturas faciales, dos manos, rotación del retrato, Listo/Aleatorio/Restablecer y countdown.
- Se reutiliza `FCosmeticSelection`; los IDs anteriores de prendas y objetos se conservan. La selección completa y el ajuste facial se congelan en cada performance. Los MIDs pertenecen a cada actor.
- El `SK_Base3` existente es el modelo visual del jugador, preview y replays. Se conserva el pawn, controller, cápsula, movimiento, MovieManager, grabador, reloj y cámara final.
- Se añadieron IK Rigs/Retargeter, ocho animaciones retargeteadas, un ABP editable y cuatro gestos provisionales; Faint usa la caída retargeteada. Root motion desactivado.
- Los 22 PNG se importaron sin alterar su archivo fuente. Se generó una superficie facial curva a partir de la geometría frontal de la cabeza; sigue Head y usa tres capas alpha independientes. No se alteraron el mesh, UVs o Skeleton originales.
- Los 10 segundos de preparación y 30 de grabación siguen siendo los valores normales. Preparación detiene el reloj de escena, las acciones y la reproducción; sólo la UI/retrato y el countdown continúan. Ready y timeout comparten la transición protegida anterior.

## Compilación y pruebas reales

| Verificación | Resultado |
|---|---|
| Development Editor, compilación final | Correcta. `Saved/Logs/Build-MyProjectEditor.log`. |
| Automatización `OneManStudio.*` | **48 Success, 0 warnings, 0 failures** en la última ejecución. Incluye grabación/replay, gestos, vestuario, objetivos, scoring, estreno, menú, videos y cuatro pruebas nuevas de personalización. |
| Editor, Survey renderizado | **39 pasos y 10 capturas**, todos correctos. Clics Slate reales en pestañas, colores, ambas manos, aleatorio y reset; un único widget, cero puntos/eventos y reloj detenido en preparación. |
| Editor, Motion renderizado | Idle/walk/run/jump observados con entrada real; cinco gestos grabados. En Take 2, el replay reproduce el salto y exactamente cinco eventos mientras el nuevo grabador permanece sin eventos. La cara y piel anteriores se conservan. |
| Editor, ciclo completo rápido | Correcto. Menús, introducción, controles, 15 objetivos, tres actores, estreno, replay inmutable, nueva película, película de cero puntos y regreso al menú. Audio sintético de prueba. |
| Windows Development, Build/Cook/Stage/Archive | **BUILD SUCCESSFUL**, UAT ExitCode 0. Primera construcción completa: **157.71 s**; actualización final: **42.75 s**. Se incluyen las tres películas de inicio y las carpetas de personalización en el cook. |
| Windows, Survey renderizado | Correcto: **39 pasos y 10 capturas**, con el nuevo modelo, capas faciales, piel/ropa y ambas manos. |
| Windows, Motion + replay concurrente | Correcto: idle, caminar, correr, salto y cinco gestos; replay del salto/cinco eventos y cero eventos duplicados en Take 2. |
| Windows, ciclo con tiempos normales | Correcto: **10/30 s**, 8 aperturas del camerino, 6 inicios por Ready y 1 por timeout, 3 title cards, 3 reproducciones y 6 transiciones. Primera película **100/100, 5 estrellas**; replay conserva datos; la siguiente película de cero objetivos recibe **0/100, 1 estrella**. Restart, New Movie, regreso al menú, volver a jugar y Quit comprobados. |
| Windows, mapa guardado durante la sesión | **Correcto** en el paquete final: ciclo completo rápido con los mismos 8 camerinos/3 reproducciones, 15 objetivos, replay inmutable, película de cero puntos, nueva película, menú y Quit. Motion/replay y Auto de videos también pasaron otra vez sobre esta versión del mapa. |
| Windows, videos de inicio | Casos **Auto** y **SkipGame** correctos: intro 1→intro 2→HUB, fondo en loop durante opciones, clic de salto consumido, fundido a negro, carga real de nivel, audio detenido y entrada de gameplay restaurada. |

Los tests renderizados ejecutan el juego real en procesos separados con `-RenderOffscreen`; no son simulaciones del material ni mocks del MovieManager. El driver del ciclo completo usa clics de los botones y controles normales; también reposiciona actores para ejercitar las condiciones de objetivos. No sustituye una sesión manual de pulido artístico.

Durante la sesión se observó un guardado interactivo de `Lvl_Bank.umap` a las **22:30:18 del 13 de noviembre**. El log de esa sesión lleva sólo el proyecto en la línea de comandos, muestra el diálogo Guardar contenido y `OBJ SAVEPACKAGE`; es distinto de los procesos de prueba. Se conservó el archivo guardado. La prueba Windows de tiempos normales anterior corresponde al mapa previo a ese guardado; el archivo actualizado está incluido en el paquete final y pasó nuevamente el ciclo completo rápido, Motion y entrada desde los videos. La evidencia del guardado se conserva en `Saved/UnifiedCustomization/ObservedEditorSave.log`.

Las últimas capturas de frente, tres cuartos, perfil, ropa y objetos se revisaron visualmente. Los rasgos están sobre la cabeza, con transparencia y sin fondos verdes o rectángulos. Las capturas de gameplay y estreno muestran Base3 y selecciones distintas entre actores. Se conserva el encuadre de la cámara de estreno existente, incluido su alcance limitado a ciertas zonas del banco.

## Evidencia dentro del proyecto

- `Saved/Automation/Movie/index.json` — 48 pruebas automatizadas.
- `Saved/Automation/CustomizationPlaySurvey/result.json` y sus diez PNG.
- `Saved/Automation/CustomizationPlayMotion/result.json` — locomoción y cinco gestos con replay concurrente.
- `Saved/Automation/StudioPlayUnifiedCustomization/result.json` — ciclo rápido del Editor.
- `Saved/Automation/CustomizationPackagedSurvey/result.json` y PNG — UI del paquete.
- `Saved/Automation/CustomizationPackagedMotion/result.json` — movimiento y replay en el paquete.
- `Saved/Automation/StudioPackagedFullTimingUnifiedCustomization/result.json` — ciclo Windows de tiempos normales; `Playback.png`, `Results.png`, `ZeroResults.png` y otras vistas.
- `Saved/Automation/StudioPackagedUnifiedCustomization/result.json` — ciclo completo del paquete final con el mapa guardado durante la sesión.
- `Saved/Automation/StartupPackagedAuto/result.json` y `StartupPackagedSkipGame/result.json`.
- `Saved/Logs/Package-Windows.log` — salida de UAT.
- `Saved/UnifiedCustomization/FinalAudit.json` y `FilesChanged.json` — hashes e inventario final.

Ejecutable para jugar: **Saved/Packages/UnifiedCustomization/Windows/Play-ONE-MAN-STUDIO.bat**. No activa comandos de prueba ni requiere Unreal Editor. Conserva los paquetes históricos anteriores.

## Archivos C++ cambiados

Todos están bajo `Source/MyProject/`:

| Archivos | Cambio |
|---|---|
| `Public/StudioCharacterProfile.h`, `Private/StudioCharacterProfile.cpp` | Nuevo perfil visual configurable, adaptación de skeleton/gestos y ajustes de primitivas. |
| `Public/StudioAnimInstance.h`, `Private/StudioAnimInstance.cpp` | Entradas de locomoción compartidas para vivo, retrato y replay. |
| `Public/StudioCharacterAssetTools.h`, `Private/StudioCharacterAssetTools.cpp` | Herramientas sólo funcionales en Editor: inspección UV/mesh, retarget, superficie facial, ABP, clips y materiales. |
| `Public/UnifiedWardrobeWidget.h`, `Private/UnifiedWardrobeWidget.cpp` | UI única de ocho categorías y estilos Blueprint. |
| `Public/WardrobePreviewActor.h`, `Private/WardrobePreviewActor.cpp` | Retrato 3D aislado con SceneCapture, sin recorder, colisión ni scoring. |
| `Public/WardrobeTypes.h`, `Private/WardrobeTypes.cpp` | Selección facial/piel/manos, catálogo de 36 elementos, datos de capas y máscara de ropa. |
| `Public/WardrobeComponent.h`, `Private/WardrobeComponent.cpp` | Aplicación, reset/random, MIDs independientes y snapshot de la apariencia completa. |
| `Public/WardrobeWidget.h`, `Private/WardrobeWidget.cpp` | Base compatible extensible; se conserva la UI anterior fuera del flujo activo. |
| `Public/MovieManager.h`, `Private/MovieManager.cpp` | Perfil por defecto, clase de camerino unificada, snapshots y visibilidad del HUD en preparación. |
| `Public/ActingTypes.h`, `Private/ActingComponent.cpp` | Captura/restauración de la clase de animación y retorno a locomoción después de gestos. |
| `Private/ReplayActor.cpp` | Uso de la clase de animación capturada y pausa de animaciones con el reloj. |
| `Private/PropInteractionComponent.cpp` | Conserva IDs/reglas; E usa derecha o izquierda si la derecha está vacía. |
| `Private/StudioGameMode.cpp`, `Private/StudioFlowWidget.cpp` | Modelo final desde el comienzo; caption del director no tapa el nuevo camerino. |
| `Private/Testing/CustomizationSurvey.cpp`, `Private/Testing/StudioFlowPlayTest.cpp` | Drivers opt-in renderizados; checks de apariencias independientes y eventos. |
| `Private/Tests/UnifiedCustomizationTests.cpp`, `Private/Tests/WardrobeTests.cpp` | Cuatro pruebas nuevas y adaptación del catálogo/fixtures anteriores. |
| `MyProject.Build.cs` | AnimGraphRuntime; módulos estándar de Unreal sólo para las herramientas Editor. Sin dependencias externas. |

## Scripts, documentación y Content

- Nuevos scripts: `Inspect-Customization.py`, `Create-Base3Rig.py`, `Create-FacialAssets.py`, `Create-CharacterConfiguration.py`, `Test-Customization.ps1`.
- Actualizados: `Package-Windows.ps1`, `Test-Movie.ps1`.
- Nuevos documentos: esta verificación y `UnifiedCustomization.md`. Actualizados `Base3Character.md`, `Wardrobe.md`, `MainMenuAndGameFlow.md`.
- **44 assets nuevos**: 22 texturas en `Content/OneManStudio/Cosmetics`; 22 assets en `Content/OneManStudio/Characters/Base3/Customization` (3 rigs/retargeter, 8 clips retargeteados, 4 gestos, ABP, perfil, superficie facial, 3 materiales y render target).
- **22 PNG fuente** en `SourceArt/FacialAssets`. Sus hashes coinciden con el pack proporcionado.
- No se reimportaron ni guardaron el mesh/Skeleton existentes. La auditoría conserva **271 de 272 archivos originales de Content, Config y .uproject sin cambios**. La única diferencia es el guardado interactivo del mapa observado arriba; se preservó su nueva versión. Los scripts de esta integración no guardan mapas y no se modificaron Blueprints artísticos existentes.
- Los intentos intermedios y permisos temporales de generación quedaron fuera del flujo activo en `Saved/UnifiedCustomization/Retired`; no se empaquetan.

## Limitaciones y trabajo de arte

- Es un paquete **Windows Development** comprobado en este equipo; no se construyó Shipping ni se certificaron otros equipos. El aviso de que MSVC 14.44 no es la versión preferida de UE 5.6 no impidió compilar o empaquetar.
- Los sombreros, accesorios y objetos siguen usando primitivas provisionales. Las cuatro prendas son variantes de material sobre el cuerpo; un vestido con silueta propia requiere una prenda modular del artista. Los íconos de esas categorías tienen marcadores provisionales reemplazables mediante `Icon`.
- Dance/Point/Scared/Arrest son animaciones provisionales; Faint y locomoción son copias retargeteadas. El skeleton de 19 huesos carece de dedos e IK del mannequin: hace falta pulido artístico para poses finales. La locomoción del replay se deriva de los transforms grabados; la detección de salto usa la altura respecto al inicio, adecuada al banco plano actual.
- Las fuentes de sonido, música y montages siguen configurables y se conservan las asignaciones existentes. Las pruebas de mezcla/cleanup usaron tonos sintéticos; no se añadieron audios finales nuevos.
- Se entregó UI nativa funcional. Para un diseño UMG propio hay instrucciones exactas y funciones/eventos Blueprint en la guía; no es obligatorio crear un Widget Blueprint para probar.
- No se añadieron expresiones automáticas por gesto, objetivos nuevos, scoring nuevo, exportación MP4 ni sistemas opcionales.
