# ONE MAN STUDIO — correcciones finales y audio

Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`. Unreal Engine **5.6.1**, Windows x64. Fecha: 14 de noviembre de 2026.

## Cambios guardados

- `Lvl_Bank`: muebles ajustados individualmente por dimensiones y apoyo real; laptops, libros y tazas sobre sus mesas; plantas, puerta, bancos, cajas, cámaras, riel y dolly apoyados. Se conservan los actores de pickups, sus IDs, los spawns, los roles y los triggers.
- `Lvl_Cinema`: doce butacas orientadas a la pantalla; filas traseras elevadas, filas delanteras bajas; pasillo central; palomitas sobre apoyos; luminarias laterales. La pantalla sigue capturando la cámara original y el mismo reloj.
- La cámara **Movie premiere camera** conserva su identidad/referencia. Posición `(250,1140,850)`, mira a `(250,-70,80)`, FOV 90. Se comprobó el recorrido de objetivos al ajustar su encuadre.
- La puerta nueva procede de `bank vault interior 3d model.zip`: el archivo contiene una **puerta con marco sólido**, sin un interior transitable. Se integró en el actor decorativo existente, abierta respecto al recorrido. Original: 367.750 triángulos; LOD de juego: **60.000**. FBX y dos texturas originales conservados.
- Antifaz: copia adaptada a la superficie real de Base3, con aberturas redondas alineadas con los ojos. Se conserva la malla original. El ID antiguo sigue funcionando; `Face.RobberMask` permite combinarlo con un gorro. Se evita dibujarlo dos veces.
- Gorra, gorro tejido, corona, vaquero y pollo: ajustes individuales contra la pose de referencia. Se resuelve una sola vez la escala 100 de los huesos importados. `Head`, `RightHand` y `LeftHand` se usan igual en preview, jugador y replays.
- Lentes claros, lentes oscuros y bigote: ajustes propios; cristal transparente en material separado y copias con patillas adaptadas a la cabeza sin orejas. Seis props con agarres para ambas manos; colisión desactivada.
- Seis máscaras de ropa sobre **UV0 original**, con cobertura distinta para mangas, manos y pies. Las franjas blancas y el cuello de camisa conservan su color al cambiar la piel. Se mantiene el cuerpo, esqueleto y Anim Blueprint existentes.
- Acabados mates para ropa/decorado, tapizado verde azulado y luz cálida en el cine. Se conservan las texturas fuente.

## Audio integrado

Se importaron los **34 archivos** de `efectos_musicas.zip`. Originales en `SourceArt/FinalAudio20261114/Original`; WAV preparados de 48 kHz en `Prepared`. Los cinco MP3 de voz se convirtieron; efectos/voz mono, música/ambiente estéreo. El inventario registra fuente, SHA-256, duración, conversión y ganancia.

| Uso | Asignación |
|---|---|
| HUB e introducción del guion | Backstage Bounce |
| Camerino | Dressing Room Frolic; continúa desde la posición guardada al volver |
| Tomas y película | Bassline Getaway |
| Resultados | Brassy Buzzer Blast |
| Ambiente | Bank Room Tone / Cinema Room Tone, una fuente global |
| Director | Las cinco voces: costume, action, cut, next actor, wrap |
| Soundboard | EvilLaugh, Scream, Cry, PoliceSiren, Kiss, Failure |
| Movimiento y actuación | Cinco pasos, impacto de desmayo, claqueta |
| Interacción | Recoger dinero y morder dona, asociados al evento existente |
| Interfaz y estreno | Click, fanfarria de estreno, fanfarria de resultados y aplausos |
| Alternativas disponibles | Segundo impacto, segundo pickup y dos fanfarrias alternativas |

No se añadieron sonidos inexistentes de alarma del banco, apertura de bóveda o público riéndose. La sirena disponible pertenece al soundboard. Los nombres truncados del pack se clasificaron por sus grupos y contenido preparado; las asignaciones se pueden cambiar en el perfil.

La música tiene una fuente global, cambios con fundidos y menor volumen durante la voz del director. El audio del video del HUB se silencia al usar la pista del menú; las introducciones conservan su audio. Los efectos de actores son espaciales. Los pasos usan un tipo de evento separado: no cuentan como soundboard ni aumentan premios/puntuaciones. Música y ambiente no se copian a las actuaciones.

## Abrir y configurar en Unreal Editor

1. Abre `MyProject.uproject` con UE 5.6 y carga `/Game/OneManStudio/Maps/Lvl_Bank`.
2. Para jugar, pulsa Play. El flujo habitual muestra videos/HUB, camerino y tres tomas. **No ejecutes los scripts de construcción antiguos sobre estos mapas corregidos.** Los cambios ya están guardados.
3. Cosméticos: abre `/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics`. Cada entrada tiene ID, malla/material, hueso y transform. Los transforms locales son pequeños porque el rig importado escala los huesos por 100; evita volver a multiplicarlos. Para sustituir arte, edita su entrada y conserva el ID.
4. `MovieManager > Cosmetic Catalog` contiene las mismas definiciones guardadas en el mapa. Al editar el catálogo de arte, actualiza también esa lista del manager; no cambies solo un actor del preview.
5. Máscaras de prendas: `/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/Outfits`. Blanco = ropa, negro = piel. Asigna `Clothing Mask` en la entrada Body y conserva UV0. `Body.None`, sin rasgos ni accesorios, permite el personaje blanco sin cara.
6. Audio: abre `/Game/OneManStudio/Audio/DA_StudioAudio`. `Flow Audio` controla menú/camerino/tomas, ambientes, claqueta y director; `Premiere Audio` aporta valores predeterminados para estreno/resultados. `Soundboard`, `Footsteps`, `Faint Impacts` y props son independientes. Los campos ya asignados en el manager/componente prevalecen sobre los predeterminados.
7. Para cambiar música/fanfarria del estreno, usa `MovieManager > Premiere Settings`; se conserva esa interfaz existente. `Actor Attenuation` configura distancia y espacialización de actuaciones.
8. El menú Audio conserva sus controles y añade únicamente **Ambiente** e **Interfaz** en la misma página desplazable. Las preferencias son de sesión. El HUD, camerino, tabs, layout y textos existentes se conservan; no se creó arte final de UI.
9. Camerino/toma siguen por defecto en **10 / 30 segundos** y tres roles. Las duraciones breves usadas por pruebas pertenecen a procesos de prueba y no se guardan en el mapa.

## Verificación y evidencia

Los resultados finales y rutas de capturas se documentan en `FinalCorrectionsVerification.md`. `Saved/FinalCorrections` contiene recibos de importación, ajustes, inventario de audio y comparación SHA-256 de archivos. El respaldo previo está en `Saved/FinalCorrections/Backup`. No se modificaron Blueprints de arte ni animaciones del director.

Los vestidos del pack son variantes de textura sobre Base3; no incluyen una falda separada con simulación. Los props siguen las manos del rig existente y pueden atravesar el cuerpo en poses extremas de animación. La bóveda no incorpora una nueva interacción de puerta. Los marcadores reservan posiciones para NPCs; no se añadió un sistema de público.
