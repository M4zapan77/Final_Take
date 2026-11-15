# Fase 4 - Resultados reales y archivos cambiados

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. Fecha: **12 de noviembre de 2026**, America/Mexico_City.

## Resultado

Vestuario antes de las tres tomas, incluida Take 1; seleccion y preview inmediatos; Ready o timeout mediante una transicion protegida; outfits independientes dentro de cada recording; restauracion en replays y premiere; reinicio sin las selecciones de la pelicula anterior. Se conservaron movimiento, reloj, actuacion, sonidos y camara de premiere.

Hay una **UMG nativa funcional**. No se creo ni modifico ningun Widget Blueprint, mapa ni asset artistico. La alternativa editable en UMG, los assets del catalogo y toda la configuracion estan descritos en [Wardrobe.md](Wardrobe.md).

## Compilacion real

Motor instalado: Unreal Engine **5.6.1**, compatible con 5.6.0, en `C:\Program Files\Epic Games\UE_5.6`. Visual Studio 2022 / MSVC 14.44.35230, Windows SDK 10.0.22621.0 y .NET incluido en el motor.

| Destino | Resultado final | Ultima compilacion incremental |
| --- | --- | --- |
| MyProjectEditor / Win64 / Development | Succeeded, DLL actualizada | 12.42 s |
| MyProject / Win64 / Development | Succeeded, ejecutable actualizado | 57.05 s |

Logs: `Saved\Logs\Build-MyProjectEditor.log` y `Saved\Logs\Build-MyProject.log`. No se hizo empaquetado/cook. Persiste la advertencia previa de UBT: MSVC 14.44 no es su version preferida (recomienda 14.38); ambos destinos compilaron y enlazaron.

## Pruebas automaticas

`Scripts\Test-Movie.ps1` valido un reporte nuevo del **2026.11.12-21.34.23 UTC** (15:34:23 en Mexico): **20 Success, 0 succeededWithWarnings, 0 failed**, sin casos pendientes. Duracion simulada de los casos: 0.652 s. Las nueve regresiones de recorder/MovieManager y las cinco de actuacion de fases anteriores siguen pasando.

| Caso nuevo | Verificado |
| --- | --- |
| Wardrobe.CatalogAndFallback | IDs unicos, categorias, default valido, reordenar catalogo sin cambiar IDs, rechazo de entradas invalidas y parametro real de color del material placeholder |
| Wardrobe.SnapshotIsolation | Outfits, componentes y tintes independientes por replay, copia de MIDs externos, snapshot invalido rechazado sin perder el anterior, cambios de ropa sin fuga de componentes |
| Wardrobe.ReadyAndTimeout | Preparacion antes de Take 1 y Take 2, contador con reloj de escena en cero, bloqueo de actuacion, seleccion, Ready temprano, timeout, confirmaciones duplicadas/obsoletas rechazadas, gameplay restaurado |
| Wardrobe.ThreeTakeOutfits | Tres outfits distintos, replays previos sin cambiar durante otro vestuario, premiere con todos los outfits y eventos, restart y restauracion del material original al detener |
| Wardrobe.NativeWidget | Arbol UMG nativo, datos de rol/toma/contador, botones reales OnClicked de seleccion y Ready, preview actualizado y rechazo de un widget anterior |
| Wardrobe.ExistingSkeleton | Hat en head, prop en hand_r, fit inicial derecho, offset resuelto guardado, prenda modular con leader pose, rig y AnimInstance originales conservados, cosmetics sin colision |

Reporte: `Saved\Automation\Movie\index.json`. Log: `Saved\Logs\MovieTests.log`. Los casos del bucle 10/30 avanzan el tiempo con ticks; no requieren esperar 150 segundos reales.

## Prueba de Play e inspeccion grafica

Tambien se ejecuto **Test-WardrobePlay.ps1 -RenderScreenshots** con un proceso separado `-game`, LocalPlayer y viewport reales, el mapa existente y render D3D12 offscreen a 1280x720. **PASS**:

- Tres menus UMG visibles, antes de sus respectivas tomas, con roles, seleccion, contador actualizado y preview.
- Dos comienzos por el boton Ready y uno por timeout; las confirmaciones duplicadas fueron rechazadas.
- Movimiento/mirada bloqueados en vestuario, camara de preview activa, reloj de escena en cero y sin actuaciones grabadas durante preparacion.
- Retiro de la UI y restauracion de input/camara al grabar.
- Outfits independientes en los actores anteriores y los tres en la premiere; gestos y sonidos conservan sus eventos.
- Restart elimina grabaciones/replays y rechaza el token anterior; la preparacion nueva recibe el default del rol.

Esta prueba usa **3 s de vestuario y 2 s de grabacion** para revisar rapido; los defaults de juego siguen siendo **10/30**. Las catorce regresiones y el caso de tiempos completos verifican los valores normales.

Reporte: `Saved\Automation\WardrobePlay\result.json`: passed=true, wardrobeScreens=3, readyStarts=2, timeoutStarts=1. Log: `Saved\Logs\WardrobePlay.log`.

Se inspeccionaron las capturas para corregir ejes de los placeholders, encuadre, luz de preview y anclaje de props. La luz temporal se desactiva al cerrar el menu; no cambia la iluminacion guardada ni la toma cinematica.

Capturas finales: [Take 1](../Saved/Automation/WardrobePlay/Take1.png), [Take 2](../Saved/Automation/WardrobePlay/Take2.png), [Take 3](../Saved/Automation/WardrobePlay/Take3.png), [Premiere](../Saved/Automation/WardrobePlay/Premiere.png).

## Archivos cambiados

Rutas relativas al proyecto activo:

| Archivo(s) | Cambio |
| --- | --- |
| Source/MyProject/Public/WardrobeTypes.h y Private/WardrobeTypes.cpp | Nuevos: IDs, seleccion, catalogo, snapshots, validacion y placeholders |
| Source/MyProject/Public/WardrobeComponent.h y Private/WardrobeComponent.cpp | Nuevos: equipamiento, materiales privados, attachments, leader pose y copia/restauracion |
| Source/MyProject/Public/WardrobeWidget.h y Private/WardrobeWidget.cpp | Nuevos: UMG funcional y API/eventos para Widget Blueprint propio |
| Source/MyProject/Private/Tests/WardrobeTests.cpp | Nuevo: seis pruebas de vestuario/UMG/rig |
| Source/MyProject/Private/Testing/WardrobePlayTest.cpp | Nuevo: prueba opcional de integracion, protegida por -game y -WardrobeSmoke; no se activa en Play normal ni Shipping |
| Source/MyProject/Public/MovieManager.h y Private/MovieManager.cpp | Roles/instrucciones/defaults, confirmacion protegida, UI/input, preview y restauracion |
| Source/MyProject/Public/ActingRecording.h y Private/ActingRecording.cpp | Snapshot de apariencia y validacion compatible con recordings anteriores |
| Source/MyProject/Private/ActingRecorderComponent.cpp | Captura y borrado del snapshot junto a movimiento/eventos |
| Source/MyProject/Public/ReplayActor.h y Private/ReplayActor.cpp | Componente de vestuario y restauracion por recording |
| Source/MyProject/MyProject.Build.cs | Modulos estandar necesarios para UI: UMG, SlateCore y Slate; ninguna dependencia externa |
| Scripts/Test-Movie.ps1 | Exige veinte casos y reporte nuevo |
| Scripts/Test-WardrobePlay.ps1 | Nuevo: prueba real de juego y capturas opcionales |
| Docs/Wardrobe.md, Docs/Phase4Verification.md | Nuevos: setup preciso, API y resultados |
| Docs/MovieManager.md, Docs/ActingEvents.md | Enlaces/contexto de fase 4; conservan su guia anterior |

El README del workspace apunta a esta fase y al proyecto activo. Binaries/Intermediate/Saved contienen los productos normales de compilacion y pruebas. Se respaldaron los archivos reemplazados en `gamejam\Saved\Phase4\Backups` y se uso control de hashes para evitar pisar cambios ajenos en Source/Scripts/Docs.

## Proteccion de Content y limites

SHA-256 de **250 archivos de Content y Config: cero diferencias** antes/despues, sin archivos nuevos o eliminados en esos arboles. El descriptor MyProject.uproject conserva su contenido. No se tocaron mapas ni Blueprints.

- Los cosmetics son primitivas del motor. Asigna tus meshes/materiales/iconos, sockets y transforms con [Wardrobe.md](Wardrobe.md). El fit de placeholders no sustituye el ajuste artistico de las prendas finales.
- No se genero un Widget Blueprint editable. La UI nativa funciona de inmediato; para layout/texturas propios hay instrucciones exactas para un Blueprint hijo de WardrobeWidget. El Designer de ese hijo se conserva.
- La integracion activa los delegates OnClicked de botones UMG de forma programatica. No hubo una sesion manual con raton/gamepad ni una validacion de tu futuro Widget Blueprint personalizado.
- Se uso **NoSound**: se verificaron datos y disparo de eventos de audio, no la escucha/mezcla. Esa revision sigue siendo manual con tus sonidos asignados.
- Un item por Head, Body, Prop y Face. El slot Face admite glasses o mustache. Las prendas modulares requieren el mismo Skeleton; no hay cloth, ragdoll, pickups ni comportamiento del prop.
- El sistema vuelve a Game Only al cerrar el menu y restaura los flags de cursor/click/mouse-over. Un HUD externo puede establecer su propio input mode al volver a Idle.
- Las capturas usan opciones de calidad reducidas solo en ese proceso. Los logs contienen avisos del motor por dos iconos VisionOS ausentes; no afectan al juego Windows. El arranque de la suite conserva las tres lineas Condition failed previas a los tests que ya estaban en fases anteriores; el reporte final registra veinte casos exitosos sin advertencias de la suite.
- No se agregaron objetivos, scoring, persistencia SaveGame ni sistemas de gameplay adicionales.
