# FINAL TAKE — integración visual final

Se integra el arte aprobado sobre los sistemas existentes de Unreal Engine 5.6.1.
No se cambia el rig, las grabaciones, el banco, el cine ni el sistema de puntuación.

## Interfaz y navegación

- Menú vertical: Jugar, Ajustes, Créditos y Salir, sobre el mismo vídeo en bucle.
- Jugar abre el estudio de selección de película; no inicia una toma directamente.
- El estudio muestra El robo perfecto, reparto, instrucciones, ayuda y ajustes.
  Comenzar película usa el fundido y viaje reales al banco.
- El camerino reutiliza la previsualización 3D y el catálogo: tarjeta crema a la
  izquierda, ocho categorías y cuadrícula desplazable a la derecha. Las tarjetas
  conservan sus centros crema y la selección dorada. Hay dos manos, Restablecer,
  Aleatorio y Listo. Sólo aparece en la primera visita a cada personaje.
- HUD compacto con REC, papel, ronda/turno, tiempo real, objetivos, contexto,
  inventario y controles. Se oculta durante el estreno.
- Resultados con desenlace real, desglose 40/30/20/10, objetivos, premios y estrellas
  derivadas de la puntuación sin redondear. Ver otra vez conserva la película;
  Grabar otra limpia la sesión; Volver al estudio abre la selección de película.
- Español e inglés conservan los textos FText y la preferencia guardada.

## Arte recibido

Se importaron 40 PNG únicos, una fuente Bungee y un Data Asset de tema. Se omitió
una copia del logo de baja resolución. El original usado sigue siendo 4096×2252.
Los PNG tienen alfa y no requieren eliminar verde. Los píxeles originales no se
modificaron. Fuente SIL OFL incluida con el proyecto y el juego.

`UI/ImportReceipt.json` contiene las rutas y hashes de origen. Los únicos nuevos
paquetes visuales están en `/Game/OneManStudio/UI/Common`, `MainMenu` y `Wardrobe`.
No se editaron mapas, personajes, materiales de arte o vídeos en esta fase.

No se recibió una portada oficial de la película del banco. El estudio usa el
logo aprobado como alternativa; `DA_FinalTakeUITheme.MovieCover` permite asignarla.
No se reutilizó el póster ilustrativo de la referencia como arte final.

## Correcciones de integración

- Foco y cursor tras cargar el banco: el fundido ya no devuelve entrada de juego
  mientras una pantalla de introducción o camerino sigue abierta.
- Los botones mantienen su zona de clic durante las animaciones de contenido.
- El mismo reproductor de vídeo continúa al abrir/cerrar paneles del estudio.
- El título del escenario se captura para que estudio, estreno y resultados coincidan.
- Ajustes y navegación de regreso conservan la pantalla de origen.
- La prueba ampliada encontró una regresión de optimización: los turnos compactos
  omitían las definiciones usadas por gestos, sonidos y pasos del actor en vivo.
  El turno activo vuelve a capturarlas; los segmentos terminados siguen evitando
  copias redundantes. Se conserva la compresión de poses estáticas.

## Archivos principales

| Área | Archivos |
| --- | --- |
| Tema y botones | `StudioUITheme.h/.cpp` nuevos |
| Menú/estudio y viaje | `StudioFlowWidget`, `StudioFlowManager`, `StartupTravelSubsystem` |
| Camerino/retrato | `UnifiedWardrobeWidget`, `WardrobePreviewActor` |
| HUD/resultados | `ObjectiveHUDWidget`, `MoviePresentationWidget` |
| Captura de actuación | `ActingRecorderComponent`, `StorySessionComponent` |
| Pruebas | `StudioFlowTests`, `StorySessionTests`, `StartupVideoPlayTest`, `CustomizationSurvey`, `StoryScenarioPlayTest` |
| Herramientas | `Import-FinalUI.py`, `Test-StoryScenarios.ps1`, `Test-Movie.ps1`, `Package-Windows.ps1` |
| Configuración | `DefaultGame.ini`, traducciones y recursos compilados ES/EN |

Las rutas de código anteriores corresponden a `Source/MyProject/Public` y
`Source/MyProject/Private`. `Docs/FinalUI-EditorSetup.md` explica las propiedades,
clases base y conexiones para personalizar la interfaz sin cambiar C++.

Los informes de pruebas y distribución se documentan por separado. Esta lista de
cambios no sustituye los resultados ejecutados de cada prueba.
