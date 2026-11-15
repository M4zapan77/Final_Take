# FINAL TAKE — interfaz final y configuración en Unreal 5.6

La interfaz nativa se crea automáticamente. No hace falta construir un Widget
Blueprint para jugar. El proyecto conserva el personaje, su rig, las animaciones,
los vídeos y las grabaciones existentes.

## Abrir y probar

1. Abre `MyProject.uproject` con Unreal Engine 5.6.1.
2. Abre `/Game/OneManStudio/Maps/Lvl_Bank`.
3. En World Settings, verifica que GameMode Override sea `StudioGameMode` o una
   subclase configurada de este. En el ejecutable, `Play-FINAL-TAKE.bat` ya especifica
   ese GameMode; el nombre interno del proyecto se conserva.
4. Usa Play > Standalone Game para comprobar vídeos, ratón y viaje de nivel.
5. Jugar abre el estudio. Comenzar película carga el banco y abre el primer camerino.
   Hay tres camerinos de hasta 30 segundos, seis turnos de hasta 10 segundos y dos
   rondas. En la segunda ronda no se vuelve a elegir vestuario.

## Arte compartido

Abre `/Game/OneManStudio/UI/Common/DA_FinalTakeUITheme` en el Content Browser.

- **Art** contiene las texturas suministradas, con claves estables para logo,
  botones, tarjetas, pestañas, paneles, ocho iconos y estrellas.
- **Heading Face** apunta a `FF_Bungee`. La licencia SIL OFL está en
  `SourceArt/FinalUI20261115/Font/OFL.txt` y se incluye en la distribución.
- **Navy, Gold, Cream y Red** controlan la paleta de textos y superficies nativas.
- **Movie Cover** admite la portada oficial cuando esté disponible. No se recibió
  un póster del banco: se usa el logo aprobado, manteniendo su proporción.
- **Hub Description** permite reemplazar el texto breve. Déjalo vacío para usar
  el texto nativo localizado. Los nuevos textos del artista deben incorporarse
  también a su flujo de localización de Unreal.

Las texturas se importaron con alfa, UserInterface2D, grupo UI, sin mipmaps y filtro
bilineal. Los paneles y botones utilizan márgenes para conservar sus bordes al
redimensionarse. El PNG original del logo sigue siendo de 4096×2252; no se retocaron
los píxeles de los originales. Reinicia PIE después de cambiar referencias o fuente.

## Personalizar la disposición sin C++

Se pueden crear Widget Blueprints derivados de estas clases. Si el Blueprint tiene
un árbol de Designer propio, el código conserva ese árbol.

| Pantalla | Clase base | Conexión |
| --- | --- | --- |
| Menú y estudio | `StudioFlowWidget` | `StudioFlowManager.WidgetClass` |
| Camerino | `UnifiedWardrobeWidget` | `MovieManager.WardrobeWidgetClass` |
| HUD de actuación | `ObjectiveHUDWidget` | `MovieManager.ObjectiveHUDClass` |
| Estreno y resultados | `MoviePresentationWidget` | `MovieManager.MoviePresentationWidgetClass` |

Cada clase expone **UITheme**, estilos y datos/eventos para Designer. Las funciones
existentes siguen disponibles: `PlayMovie`, `BeginMovie`, `AudioSettings`, `Back`,
`SelectLanguage`, `SelectTab`, `ChooseItem`, `ChooseSkin`, `SelectHand`, `Ready`,
`RandomizeAll`, `ResetAll`, `ReplayMovie` y `RecordNewMovie`.

En la presentación nativa, el botón de resultados etiquetado Volver al estudio usa
`StudioFlowManager.ReturnToHub`. El nombre histórico `MovieIntroduction` del enum
representa ahora la selección/preparación de película del estudio, para conservar
compatibilidad con los Blueprints existentes.

No añadas un segundo MovieManager, reloj, inventario o controlador de turnos al
conectar una UI propia. Usa los ViewData y delegados existentes. El botón Listo
conserva la protección `PreparationId`; el tiempo y el botón confirman por la misma
ruta. Los menús no pausan el mundo ni graban eventos.

## Cosméticos y controles

Las ocho pestañas siguen usando el catálogo actual de `WardrobeComponent` y los
IDs guardados en cada interpretación. Las miniaturas usan `CosmeticDefinition.Icon`
o `FaceTexture`. Los colores de piel son muestras de color; Ninguno es una selección
vacía válida. No se añadieron iconos emoji ni objetos ficticios.

El retrato es el `WardrobePreviewActor` existente, con el mismo perfil y selección
del actor jugable. Las opciones izquierda/derecha conservan las dos manos. Cambiar
de ronda no repone dinero confiscado ni objetos soltados.

El HUD muestra el objetivo, la consecuencia y la interacción válidos del estado
real. E interactúa; Q suelta; clic izquierdo lanza; T termina el turno. Las teclas
configurables de soltar/lanzar/cortar se leen del MovieManager. 1–5 son los gestos y
Z/X/C/V/B/N los sonidos. El HUD de actuación se oculta durante la película.

## Vídeos, audio e idioma

En una subclase de `StudioFlowManager`, selecciona el componente **StartupVideos**.
Su struct **Settings** conserva StudioIntro, GameIntroduction, HubBackground,
VideoMaterial, StartingGameplayLevel, FadeSeconds y niveles de audio. No reemplaces
el Media Player para abrir opciones: el mismo reproductor sigue en bucle.

Las opciones existentes controlan volumen general, música, efectos/director,
ambiente, interfaz y español/inglés. El idioma se guarda entre sesiones. Los textos
nativos usan FText; `Scripts/Build-Localization.ps1` actualiza y compila los catálogos
revisados de `Config/Localization/FinalTakeTranslations.json`.

## Distribuir

Con el Editor cerrado, `Scripts/Package-Windows.ps1 -Configuration Shipping
-ArchiveDirectory <carpeta>` cocina banco, cine, UI, cosméticos, audio, vídeos y
localización. Conserva toda la carpeta Windows, no sólo el ejecutable. Usa su
`Play-FINAL-TAKE.bat`. No se implementa exportación MP4.
