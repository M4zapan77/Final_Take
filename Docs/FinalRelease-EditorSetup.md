# FINAL TAKE — banco final y ajustes de Editor

Proyecto activo: `C:/Games/OneManStudio/MyProject/MyProject.uproject`, Unreal Engine 5.6.1.
Abrir el mapa existente `/Game/OneManStudio/Maps/Lvl_Bank` y usar Play.
El modo nativo `StudioGameMode` mantiene los vídeos, menú, estudio y el flujo del juego.

El ciclo conserva tres camerinos de hasta 30 s en la primera ronda, seis turnos de
hasta 10 s, estreno y resultados. La segunda ronda conserva los mismos actores,
posiciones e inventario. T termina el turno; E realiza la acción contextual;
Q suelta, clic izquierdo lanza; 1–5 activa gestos; Z/X/C/V/B/N activa sonidos.

## Distribución y puntos de aparición

Se conservan el mapa, sus actores de gameplay, cámara final, meshes y perfiles.
La escala del shell fue ajustada a 19.8 m de largo. Las cajas están a la izquierda,
la espera a la derecha del frente, la bóveda al fondo y producción al fondo derecho.
El suelo continuo y techo crema son elementos locales añadidos al mismo mapa.

Los TargetPoint con tags `StorySpawn.Robber`, `StorySpawn.PoliceOfficer` y
`StorySpawn.Customer` definen las posiciones iniciales de los tres protagonistas.
Los tags `StorySpawn.NPC.Teller`, `.Guard`, `.Witness1` y `.Witness2` sitúan NPCs.
Se consumen únicamente al iniciar una película nueva. Moverlos en Editor sobre el
suelo, con suficiente espacio para una cápsula de radio 42 cm y semialtura 96 cm.
No colocarlos encima de sillas ni taburetes. El banco usa un suelo plano conocido
para evitar que una búsqueda desde arriba coloque personajes sobre paredes.

`StorySceneAnchor` define el centro de la escena. `StoryExit` está tras la puerta
principal; `StorySecure` junto al guardia; `StoryMoneySurface` indica la cara superior
del pedestal de la única bolsa interactuable. Los objetos decorativos no duplican
la bolsa ni cambian la propiedad del dinero.

## Puerta, límites y cámara

`Vault - interactive recorded door` usa `StudioSetDoor`. En Details se pueden
cambiar DoorId, OpenAngle y OpenSeconds; el mesh se asigna en VaultLeaf y su ajuste
local queda separado de la bisagra. E abre/cierra. El cajero también entrega el
dinero y abre la bóveda, conservando la ruta rápida anterior. La puerta rechaza un
cierre si hay un personaje en el paso. Su movimiento sigue el reloj compartido,
se conserva entre turnos y se reproduce desde sus eventos en el estreno.

`Bank invisible safety perimeter` usa `StudioBankBounds`: HalfSize define el borde
exterior y RecoveryDepth la profundidad de recuperación. Sus cajas no bloquean
la cámara. Un actor fuera de límites vuelve a su última pose segura conservando
instancia, turno, vestuario e inventario. Los props fuera de límites se recuperan
sin crear otra copia del dinero.

La cámara vuelve siempre al personaje controlado tras cada camerino. El estreno
mantiene la cámara existente, el director automático y el cine. Los actores con
tag `FinalBankCeiling` se ocultan únicamente en la captura de la película para no
tapar el encuadre superior. El techo permanece visible durante el juego.

## Personalización visual

Los seis materiales nuevos están en
`/Game/OneManStudio/Environment/Bank/FinalRelease`. Se pueden cambiar colores y
roughness. No se sustituyeron texturas ni materiales originales del artista.
Las luces originales se conservan junto a cinco rellenos cálidos bajo el techo.
La exposición fija del banco y la captura del cine deben revisarse juntas si se
cambia la intensidad. El catálogo, perfiles de personajes/audio y UITheme anterior
siguen siendo los puntos de configuración de ropa, caras, audio y UMG.

No ejecutar los scripts históricos Build-BankLevel ni las etapas congeladas sobre
este mapa. Los scripts finales llevan protección SHA256 y sólo sirven para la
reproducción controlada del cambio. Para editar después, usar el Editor normalmente.

## Comprobación manual

1. Iniciar por el lanzador del paquete; comprobar vídeos y salto al menú con un clic.
2. Jugar, leer el estudio y abrir el primer camerino. Cambiar aspecto y pulsar Listo.
3. Acercarse al cajero y pulsar E, o abrir la bóveda y recoger su dinero con E.
4. Probar Q, lanzamiento, gestos y sonido. Confirmar que la siguiente cámara sigue
   al nuevo actor y que la segunda ronda conserva poses, objetos y vestuario.
5. Para escapar, distraer al guardia (puede hacerlo el civil en su turno), llevar
   el dinero a la salida frente a la puerta principal y pulsar E. Si el guardia
   sigue alerta, el HUD explica la condición pendiente.
6. Ver el estreno, resultados y Repetir película; después Nueva película y Volver
   al estudio/menú. Los datos previos deben desaparecer al comenzar otra película.

La copia anterior del mapa está fuera de cachés en
`gamejam/output/recovery/BeforeFinalBank20261115/Lvl_Bank.umap`; el checkpoint Git
`camera-fix-before-final-bank-2026-11-15` y los ZIP históricos también se conservan.
