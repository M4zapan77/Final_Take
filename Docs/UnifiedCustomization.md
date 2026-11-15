# Camerino unificado y personaje Base3

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal Engine 5.6. El mapa del banco, el pawn, el controller, la cápsula y los Blueprints anteriores se conservan. `StudioGameMode` aplica el perfil visual al jugador durante la ejecución. El Blueprint original puede seguir mostrando Quinn en su editor; Play usa Base3.

## Probar en Unreal Editor

1. Abre el proyecto y `/Game/OneManStudio/Maps/Lvl_Bank`. Usa **Play > New Editor Window** o Standalone para comprobar el encuadre completo.
2. Termina o salta las introducciones con clic izquierdo. **Start Game** funde a negro, carga el banco y abre el camerino. Si desactivaste Startup Videos, el flujo anterior muestra primero la introducción de texto: léela y confirma.
3. Antes de cada toma aparece un único **CAMERINO DEL ACTOR**. El tiempo normal es **10 segundos**. Ojos, Boca, Cejas, Piel, Ropa, Sombreros, Accesorios y Objetos comparten la misma selección. Las flechas giran solamente la cámara del retrato.
4. Prueba Ojos/Clásicos, Boca/Sonrisa, Cejas/Neutrales; cambia Piel y Ropa. En Objetos selecciona una mano y luego el objeto. Puedes equipar el mismo ID en las dos manos. **Ninguno** quita la categoría; **Sin ropa** quita el material de vestuario conservando piel y cara.
5. **RESTABLECER** deja el cuerpo blanco, sin cara, sombrero, accesorio ni objetos. **ALEATORIO** selecciona una combinación válida. **¡LISTO!** y el timeout llaman a la misma transición protegida.
6. Ya grabando, WASD mueve, ratón gira, Espacio salta; 1–5: Dance, Point, Scared, Faint y Arrest. Z/X/C/V/B/N: risa, grito, llanto, sirena, beso y fallo. E recoge/usa el objeto de la mano derecha; si está vacía usa la izquierda. El HUD conserva los objetivos y controles existentes.
7. En las siguientes tomas, cambia todos los colores y rasgos. Los anteriores actores deben conservar sus selecciones y eventos. Comprueba estreno, Replay Movie, Record New Movie y regreso al menú. Elegir una bolsa en el camerino no equivale a recogerla en el mundo.

En un MovieManager colocado/configurado por Blueprint se mantienen **Preparation Duration Seconds = 10**, **Recording Duration Seconds = 30**, tres Roles y el mismo Premiere Camera. `bUseUnifiedCustomization = true` hace que la antigua clase nativa de vestuario se sustituya por la unificada. El modo nativo crea su manager si el nivel no lo tiene.

## Assets creados y personaje

En Content Browser abre **OneManStudio/Characters/Base3/Customization/DA_Base3Character**:

- **Body Mesh**: el `SK_Base3` ya importado; no se duplicaron su esqueleto ni el mesh.
- **Animation Class**: `Animations/ABP_Base3`, derivado de `StudioAnimInstance`. Su grafo editable mezcla idle/walk/run/jump/fall y termina en `DefaultSlot`.
- **Mesh Transform**: yaw -90°, escala 1.05. **Fit Feet To Capsule** sitúa los pies respecto a la cápsula existente.
- **Skin Material**: `Materials/M_Base3Skin`.
- **Face**: `Meshes/SM_Base3_FaceFront`, `Materials/M_Base3Face`, `Cosmetics/T_Face_None`, hueso **Head**.
- **Default Gestures**: cinco gestos compatibles. Dance, Point, Scared y Arrest son clips provisionales creados sobre este esqueleto; Faint usa la caída retargeteada. Reemplázalos con tus animaciones finales o montages compatibles en `DefaultSlot`. Prefiere animaciones in-place, sin root motion.

`Rigs/IK_Quinn_Source`, `IK_Base3_Target` y `RTG_Quinn_To_Base3` quedan editables. Se retargetearon copias de las animaciones existentes; las fuentes permanecen intactas. El destino tiene 19 huesos y carece de dedos/IK auxiliares del mannequin, por lo que las poses siguen siendo aproximaciones hasta el pulido artístico.

## Cara, piel y ropa

Los 22 PNG originales se copiaron sin alterar a `SourceArt/FacialAssets` y se importaron a **OneManStudio/Cosmetics/Eyes**, **Mouths**, **Eyebrows** y `T_Face_None`. Son RGBA 512×512, clamp, sRGB y compresión UI. Los IDs son `Eyes.Classic`, `Eyes.Happy`, etc.; `Mouth.Smile`, etc.; `Brows.Neutral`, etc. `None` es un ID vacío que resuelve la textura transparente, sin entradas duplicadas de catálogo.

Se inspeccionaron vértices, triángulos y UV0: el mesh tiene un solo atlas corporal. Por ello se generó una superficie curva a partir del frente real de la cabeza, separada 0.035 cm y unida a **Head**. No se modificaron UVs, huesos, morph targets ni el mesh original. La cara no es una tarjeta plana.

En **DA_Base3Character > Face**, cada capa tiene **Position**, **Scale** y **Visible**, guardadas junto a cada toma. Valores iniciales (UV normalizadas de la superficie facial):

| Capa | Position | Scale |
|---|---|---|
| Eyes | 0.50, 0.42 | 0.90, 0.70 |
| Mouth | 0.50, 0.72 | 0.62, 0.65 |
| Eyebrows | 0.50, 0.22 | 0.85, 0.80 |

El material combina alpha independiente de `EyesTexture`, `MouthTexture` y `EyebrowsTexture`. Cada actor tiene su propio MID. La piel utiliza `SkinColor`; la ropa usa `Color` y `OutfitStyle` (0 sin ropa; 1 casual; 2 uniforme; 3 traje; 4 vestido). Los materiales actuales son variantes provisionales sobre el cuerpo, no prendas finales modeladas. Cabeza/manos permanecen separadas del color de ropa.

Para una máscara artística precisa, asigna **Clothing Mask** al elemento Body del catálogo: canal rojo blanco = ropa, negro = piel, usando el UV0 original del cuerpo. `UseClothingMask` se configura automáticamente. Sin máscara se utiliza el ajuste procedural inicial basado en posición del mesh antes de animar. Puedes sustituir la variante por un material compatible o una prenda modular con el mismo Skeleton y **Use Leader Pose**.

## Catálogo, sombreros y ambas manos

Reutiliza el **Catalog** de WardrobeComponent o el **Cosmetic Catalog** de MovieManager según `Use Performer Cosmetic Catalog`. Cada `FCosmeticDefinition` expone ID, categoría, nombre, **Icon**, meshes, materiales, tinte, socket, transform y tags. No cambies IDs de props si deben seguir resolviendo los objetivos existentes: `Prop.Banana`, `Prop.Donut`, `Prop.MoneyBag`, `Prop.Microphone`.

La estructura existente `FCosmeticSelection` se extendió. Para compatibilidad, **HeadId = Hat ID**, **BodyId = Outfit ID**, **FaceId = Face Accessory ID**, **PropId = Right Hand Prop ID**. Se agregaron EyeId, MouthId, EyebrowId, SkinColor y LeftHandPropId; no hay un segundo estado de personalización dentro del widget.

Sockets Base3: **Head**, **LeftHand**, **RightHand**, **Spine1**, **Hips**. Se adaptan los nombres anteriores `hand_l`, `hand_r`, `spine_03` y `pelvis`. Sus huesos importados llevan escala 100: el componente convierte y congela el ajuste para mantener tamaños en centímetros en vivo y replay. Configura Relative Transform en tus piezas finales; revisa ambos lados al asignar nuevos props. Los meshes de replay/preview y sus accesorios no tienen colisión física.

## Personalizar UMG sin C++

**No hay pasos manuales obligatorios para que aparezca la interfaz.** La clase nativa `UnifiedWardrobeWidget` es funcional. Para cambiar solamente la presentación:

1. Crea un Widget Blueprint cuyo parent sea **UnifiedWardrobeWidget** y asígnalo a **MovieManager > Wardrobe Widget Class**. Mantén `Use Unified Customization` activo. No agregues otra llamada Create Widget para el vestuario.
2. Sin árbol Designer propio, hereda la interfaz nativa y cambia Navy, Purple, Cream, Gold, Red, Heading Font, Font Size, Logo, Card Size, Button Style y Skin Presets en Class Defaults. Cambia las imágenes por `Catalog[].Icon`; los PNG faciales ya son miniaturas.
3. Para un diseño completamente propio, crea tu árbol UMG. El padre lo conserva y no añade su interfaz nativa encima. Usa **On Wardrobe Opened** y **On Wardrobe View Changed** para refrescar el take, rol, selección y countdown desde `ViewData`. `GetMovieManager` da título, guion y ObjectiveManager para tus objetivos.
4. Botones: **Choose Item(Category, StableID)**, **Choose Skin(Color)**, **Reset All**, **Randomize All**, **Ready**. Para los objetos usa categorías **Prop** (derecha) y **LeftProp** (izquierda), con los mismos IDs del catálogo. **GetOptions** devuelve opciones por categoría. Guarda la pestaña activa en el widget, nunca una copia paralela del outfit.
5. El retrato se crea durante la preparación incluso con un árbol Designer propio. En el evento de refresco, asigna **Get Preview Material** a un Image con Set Brush From Material cuando sea válido. Usa una caja cuadrada/Scale To Fit. **Rotate Preview(±25)** gira la cámara. Puedes asignar un Blueprint derivado de **WardrobePreviewActor** en **Preview Actor Class**: Camera Distance, Field Of View y Exposure Compensation ajustan el retrato, y Capture expone el postproceso. No le añadas recorder, ActingComponent ni objetivos.

Todos los tiempos, snapshots y cambios de toma siguen siendo responsabilidad del MovieManager. El retrato se destruye al cerrar el widget. No se detiene el mundo entero para mostrar el camerino.

## Compilar y verificar

Desde PowerShell dentro del proyecto:

```powershell
& .\Scripts\Build.ps1
& .\Scripts\Test-Movie.ps1
& .\Scripts\Test-Customization.ps1 -Scenario Survey
& .\Scripts\Test-Customization.ps1 -Scenario Motion
& .\Scripts\Test-Customization.ps1 -Scenario Full
& .\Scripts\Package-Windows.ps1
```

Survey pulsa controles Slate reales y guarda diez vistas en `Saved/Automation/CustomizationPlaySurvey`. Motion ejercita entrada real, cinco gestos y su reproducción durante una segunda toma, incluido el salto y la ausencia de eventos duplicados. Full usa el ciclo existente completo con tiempos reducidos y audio sintético de prueba; para tiempos normales usa `Test-StudioFlow.ps1 -UseDefaultTimings -EnableAudio -ReportSuffix UnifiedCustomizationFullTiming`.

El paquete nuevo se archiva en **Saved/Packages/UnifiedCustomization**. El mismo test acepta `-PackageDirectory` para la carpeta archivada. Los drivers de prueba son opt-in y quedan excluidos de Shipping. Los scripts Create-* son herramientas de creación del desarrollador con protecciones contra sobrescritura; no hace falta ejecutarlos para jugar ni para empaquetar.

Las limitaciones artísticas y los resultados reales de esta entrega están en **UnifiedCustomizationVerification.md**.
