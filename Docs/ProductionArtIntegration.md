# ONE MAN STUDIO — Banco, cine y vestuario final

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. Unreal Engine 5.6.1, Windows 64 bits.

## Abrir y jugar

Abre el proyecto existente y el mapa `/Game/OneManStudio/Maps/Lvl_Bank`. Su GameMode sigue siendo `StudioGameMode`. Pulsa Play: videos de estudio y presentación, HUB, introducción de la película, camerino y tres tomas. Los valores guardados siguen siendo **10 segundos de preparación y 30 de grabación**.

La nueva versión ejecutable está en `Saved\Packages\ProductionArt\Windows\Play-ONE-MAN-STUDIO.bat`. Copia la carpeta **Windows completa** para probarla en otra PC. El paquete anterior `UnifiedCustomization` permanece disponible como respaldo.

Controles: WASD, ratón y espacio; E recoge/usa el objeto derecho; 1–5 ejecutan Dance, Point, Scared, Faint y Arrest; Z/X/C/V/B/N activan risa, grito, llanto, sirena, beso y fracaso. R reinicia la película. El camerino permite escoger objetos para cada mano de forma independiente.

## Inventario importado

Se conservaron los cinco archivos originales y sus extracciones en `SourceArt\ProductionArt20261113`. Hay **57 FBX originales, 45 entradas de arte, 39 nuevas mallas estáticas y 183 texturas de origen**. Las seis prendas se aplican como materiales sobre `SK_Base3`; no se crearon seis cuerpos ni se sustituyeron su esqueleto o animaciones. Los FBX de ropa de mayor resolución repiten la misma geometría/UV; permanecen como fuentes.

| Grupo | Modelos/variantes disponibles | Uso |
|---|---|---|
| Banco, 16 | Banquito_Amarillo, Escritorio, Estanteria, Laptop, Libreta, Libro, Libros_Estanteria, Libros_Estanteria2, Planta_1/2/3/4, Puerta_Vidrio, Sillon_Amarillo, Sillon_Azul, Tasa | Mobiliario del banco |
| Rodaje, 5 | Camara1_Alta, Camara2_Baja, CamaraMan, Clapperboard, Silla_Director | Cámaras, claqueta y silla colocadas; CamaraMan queda en biblioteca |
| Cine, 3 | Lampara_Colgante1, Palomitas, Sillon_SalaDeCine | Luminarias, palomitas y 12 asientos |
| Ropa, 6 | Casual verde, uniforme, traje, vestido rosa, rayas de ladrón, pijama azul | Variantes de material sobre UV0 original |
| Gorros, 5 | Policía, corona rosa, pollo, tejido, vaquero | Camerino y grabaciones |
| Accesorios faciales, 4 | Antifaz, bigote, lentes redondos, lentes oscuros | Camerino; el antifaz conserva el ID Head.RobberMask |
| Objetos, 6 | Plátano, micrófono, dona, bolsa de dinero, patito, megáfono | Ambas manos; los primeros cuatro conservan sus IDs |

El inventario completo con rutas, materiales, texturas, procedencia y polígonos está en `Saved\ProductionArt\AssetInventory.csv`. Los prefijos de carpetas son:

- `/Game/OneManStudio/Environment/Bank/ProductionArt`
- `/Game/OneManStudio/Environment/FilmEquipment/ProductionArt`
- `/Game/OneManStudio/Environment/Cinema/ProductionArt`
- `/Game/OneManStudio/Cosmetics/ProductionArt`

Los seis objetos originales tenían aproximadamente 1.9–2 millones de triángulos cada uno. Se generaron FBX derivados de **24,000 triángulos**, conservando UVs y originales, antes de importarlos. Los otros accesorios tienen un máximo de 24,000 y el mobiliario un máximo de 60,000 en LOD0. Blender portátil se utilizó únicamente como herramienta de preparación; el ejecutable no lo necesita.

## Banco colocado

Se conserva la envolvente y escala que ya tenía tu mapa, el PlayerStart, MovieManager, sus tres inicios, las zonas originales y la cámara final. La entrada es X negativo; el fondo/bóveda es X positivo. Y negativo es el lateral de las cajas; Y positivo contiene espera y rodaje.

- Tres escritorios con frentes azul marino, laptops, bancos, letreros, libros y café a la izquierda.
- Espera compacta al frente-derecha con banca azul, mesa, banco y plantas.
- Puerta de vidrio decorativa abierta en la entrada; no bloquea al jugador.
- Bóveda al fondo, bolsa interactiva y bolsas decorativas, casilleros y barras de oro. Los casilleros dejan libre el paso central hacia la salida.
- Producción al fondo-derecha: riel, plataforma móvil y cámara, segundo trípode, silla de director, claqueta, luces y cajas de equipo.
- Marcadores para tres ubicaciones alternativas de cajero, tres clientes y un guardia. No se generan NPCs nuevos.

Solo se retiraron las dos piezas provisionales `Bank counter` y `Counter top`. Los tres pickups conservan sus actores e identificadores; ahora usan las mallas recibidas. La bolsa está en `(700,0,82)`, la dona en `(10,-400,124)` y el plátano en `(250,-400,124)`.

La zona original `Bank.Counter` se conserva. Una segunda zona con el mismo tag cubre el frente de los nuevos escritorios; el sistema existente acepta cualquiera de ambas. Muebles grandes bloquean cuando corresponde. Plantas, decoraciones, riel y cámaras no generan obstáculos; los replays siguen sin colisión física.

No venían modelos independientes de puerta de bóveda, riel/dolly, pantalla, cortinas, cajas de rodaje o lingotes. Esas piezas son construcciones sencillas editables con geometría de Unreal. No se presentan como modelos finales recibidos. `CamaraMan` se importó pero no se colocó como NPC: el futuro reparto ambiental puede reutilizar Base3.

## Ajustar cosméticos en Unreal Editor

1. Selecciona `ONE MAN STUDIO - Movie Manager` en el banco. En `Cosmetic Catalog`, busca la entrada por **Id**, no por número de fila. Allí puedes cambiar Static Mesh, Materials, Icon y transformaciones sin C++.
2. `/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics` contiene las 21 definiciones suministradas como catálogo reutilizable. Los defaults nativos lo combinan con los faciales existentes. Si cambias ese asset con el editor abierto, vuelve a cargar los defaults/reinicia el editor; una instancia con overrides en `Cosmetic Catalog` debe actualizarse también. Editar directamente el catálogo del MovieManager es la vía inmediata para PIE.
3. Los IDs anteriores permanecen: `Head.PoliceCap`, `Head.RobberMask`, `Head.Crown`, `Head.Chicken`; `Body.Casual/Uniform/Suit/Dress/None`; `Face.Glasses/Mustache`; `Prop.Banana/Donut/MoneyBag/Microphone`. Se agregaron `Head.Beanie/Cowboy`, `Face.Sunglasses`, `Body.Robber/Pajamas` y `Prop.RubberDuck/Megaphone`. El catálogo completo tiene **43 entradas**, incluyendo los 21 faciales previos.
4. Los accesorios se ajustan en la **pose de referencia** y se unen a `Head`, `RightHand` o `LeftHand`. Se corrigió el ajuste que enterraba los lentes/antifaz en la cabeza durante idle. Mantén `Fit In Actor Space` desactivado en estas entradas importadas.
5. `Relative Transform` mueve el objeto de la mano derecha o cabeza. Para objetos, `Use Left Hand Transform` y `Left Hand Relative Transform` permiten un ajuste izquierdo distinto usando el mismo modelo/ID. El FBX tiene escala 100 en huesos: aproximadamente **0.01 unidades de traslación local = 1.05 cm visibles**. Los valores de escala local también compensan ese factor.
6. Para obtener un nuevo ajuste desde medidas cómodas en centímetros, usa `StudioCharacterAssetTools.ReferenceBoneTransform(SK_Base3, Bone, Profile.MeshTransform)` y `Make Relative Transform(DesiredActorTransform, BoneTransform)`. El script `Configure-ProductionCosmetics.py` contiene ejemplos reproducibles.
7. La ropa utiliza `MI_Outfit_*` y `M_ProductionOutfit`: textura original, `SkinColor`, `Color` para tinte opcional de ropa, `OutfitStyle` y `Roughness`. El selector de piel modifica solo la piel. `Body.None` vuelve al cuerpo blanco/faceless cuando se restablece la selección.
8. El perfil `DA_Base3Character > Face > Relative Transform` tiene escala 1.015 para separar ligeramente el dibujo facial de la piel y reducir parpadeos. Su versión previa está respaldada. No se editaron las texturas faciales ni la malla/esqueleto Base3.

Los lentes redondos conservan las lentes opacas del modelo/material suministrado. Puedes asignar un material de cristal o una variante sin lentes desde la entrada correspondiente. Los ajustes están hechos para Base3; otra anatomía requerirá nuevos offsets. Reimportar un FBX de origen requiere volver a aplicar su normalización a 100 cm; para sustituir arte, también puedes asignar una nueva malla y ajustar su definición.

## Personalizar el camerino

El widget nativo `UnifiedWardrobeWidget` sigue siendo el activo. Incluye los ocho tabs, selección por mano, preview 3D, giro, reset, aleatorio, countdown y Ready. Se redujo la fuente de las opciones para evitar nombres recortados y el preview encuadra también los gorros altos.

Para cambiar colores/fuentes sin código, crea un Widget Blueprint derivado de `UnifiedWardrobeWidget`, conserva vacío el Designer para utilizar el layout nativo y modifica sus Class Defaults: Navy, Purple, Cream, Gold, Heading Font, Option Font Size, Button Style, Card Size y Preview Actor Class. Asigna esa clase en `MovieManager > Wardrobe Widget Class`.

Para una composición UMG propia, usa un Designer con tu árbol y conecta `OnWardrobeViewChanged(Data)` a los textos y countdown; `ChooseItem(Category, Id)`, `ChooseSkin`, `SelectHand`, `ResetAll`, `RandomizeAll`, `RotatePreview` y `Ready` a los botones. `GetOptions` entrega los IDs e iconos. `GetPreviewMaterial` entrega el material para el Image de preview después de abrirse. Mantén la transición de Ready en el manager; no crees otro temporizador ni otro recorder en el widget.

## Cine y cámara de riel

`Lvl_Cinema` es un subnivel **Always Loaded** del banco, situado a Y=5000. No se viaja a otro mundo al terminar la película: permanecen vivos los mismos datos, replays y reloj. Ábrelo solo para editar el decorado; inicia la partida desde `Lvl_Bank`.

`ONE MAN STUDIO - Theater screen` (`StudioCinemaPresenter`) escucha al MovieManager. Captura su cámara final existente con SceneCapture2D y proyecta la imagen en la pantalla; el controller mira desde la cámara del público. El listener de audio sigue a la cámara del rodaje para escuchar los sonidos grabados. Al reiniciar o volver al menú se detienen captura y override del listener.

En Details puedes cambiar `Enabled`, `Screen Material`, `Capture Width/Height`, `Audience Field Of View`, `Audience Exposure EV`, `Flip Screen U/V` y `Use Film Audio Listener`. Los eventos `OnCinemaStarted/Stopped` permiten personalización adicional. **Enabled=false** conserva la reproducción final a pantalla completa mediante la cámara original.

El cine usa el canal de iluminación **1**; el banco conserva el 0. Así las luces direccionales de relleno del banco no atraviesan el teatro. Si agregas muebles/NPCs al cine, asigna sus meshes y luces al canal 1.

`Production camera rail` (`StudioRailRig`) lee el mismo reloj y mueve plataforma, modelo y CineCamera juntos. Ajusta Start Offset, End Offset, Travel Seconds, Look At World y Focal Length. Por defecto recorre 550 cm en 30 segundos, paralelo a la pared derecha. No tiene colisión y no escribe en el reloj. `Use Optional Rail Angle` en el presentador permite usarlo como ángulo alternativo; queda **desactivado por defecto** para conservar tu cámara final.

## Reproducir las verificaciones

Con Unreal Editor cerrado, desde PowerShell en el directorio del proyecto:

```powershell
.\Scripts\Build.ps1
.\Scripts\Test-Customization.ps1 -Scenario Survey
.\Scripts\Test-Customization.ps1 -Scenario Motion
.\Scripts\Test-StudioFlow.ps1 -EnableAudio -ReportSuffix ProductionArt
.\Scripts\Package-Windows.ps1
.\Scripts\Test-StudioFlow.ps1 -PackageDirectory .\Saved\Packages\ProductionArt -UseDefaultTimings -EnableAudio -ReportSuffix ProductionArt
.\Scripts\Test-StartupVideos.ps1 -PackageDirectory .\Saved\Packages\ProductionArt -Scenario Auto
python .\Scripts\Report-ProductionArt.py
```

Los tests de audio usan tonos sintéticos **solo en la prueba**, no nuevos assets finales. Asigna los seis sonidos finales en ActingComponent; música/locución se mantienen configurables en StudioFlow y Premiere Settings. No se generaron locuciones, efectos finales ni música nuevos en esta integración.

`Prepare-ProductionArt.py`, `Optimize-ProductionProps.py`, `Import-ProductionArt.py`, `Configure-ProductionCosmetics.py` y `Build-ProductionSets.py` son herramientas de importación/regeneración, no pasos necesarios para jugar. El constructor de escenas verifica hashes antes de reescribir mapas propios. Conserva tus cambios manuales y usa las propiedades de los actores para el trabajo artístico diario.

Resultados reales y archivos modificados: `Docs\ProductionArtVerification.md`, `Saved\ProductionArt\FilesChanged.json` y `Saved\ProductionArt\PlacementReport.json`.
