# Banco Meshy - mapa principal

Proyecto activo: `C:\Games\OneManStudio\MyProject\MyProject.uproject`, Unreal **5.6.1**. La fase 6 de premiere/resultados se termino y verifico antes de esta sustitucion autorizada del escenario. Su implementacion y pruebas originales estan en [Phase6Verification.md](Phase6Verification.md).

## Abrir y jugar

1. Abre el proyecto activo. El mapa de inicio del Editor y del juego es **`/Game/OneManStudio/Maps/Lvl_Bank`**. Si tienes otro nivel abierto, abre **Content > OneManStudio > Maps > Lvl_Bank**.
2. Pulsa **Play** en modo Selected Viewport o New Editor Window, con un jugador. Desde fase 7 aparece el menu: **Play > introduccion > Choose My Costume > vestuario de Take 1**. No necesitas comandos de consola ni crear Blueprints. Guia actual: [MainMenuAndGameFlow.md](MainMenuAndGameFlow.md).
3. Selecciona cosmetics y pulsa Ready, o espera **10 segundos**. Cada toma graba **30 segundos**. La segunda y tercera reproducen las actuaciones anteriores con su propio outfit y eventos.
4. Tras Take 3 aparecen titulo, pelicula desde la camara fija del banco y resultados. Replay Movie conserva la pelicula; Record New Movie vuelve al primer vestuario con datos limpios. Una pelicula con cero objetivos tambien se puede ver y calificar.
5. Controles: WASD/mouse para moverte/mirar, Space para saltar, **E** para recoger/usar el prop que indica el HUD, **1-5** Dance/Point/Scared/Faint/Arrest, **Z/X/C/V/B/N** Laugh/Scream/Cry/Siren/Kiss/Failure, **R** para reiniciar.

**No ejecutes Heist.Demo en este nivel:** las zonas y props ya estan colocados permanentemente. El mapa anterior **`/Game/ThirdPerson/Lvl_ThirdPerson`** sigue disponible, sin modificaciones. Sus comandos temporales de prueba siguen funcionando.

Para acelerar una prueba manual, selecciona **ONE MAN STUDIO - Movie Manager** en el Outliner y cambia **Preparation Duration Seconds = 2** y **Recording Duration Seconds = 5** antes de Play. Los valores guardados del banco son 10/30.

## Escenario y assets

Origen: `C:\Users\diego\Downloads\Meshy_AI_Bank_Shell_Diorama_3D_20261113_texture_fbx.zip`. Contiene un FBX y cuatro texturas; no incluye instrucciones, muebles, animaciones ni un mapa Unreal. El ZIP original se conserva. Los cinco archivos fuente tambien se conservaron en **SourceArt/MeshyBank**.

Nuevos assets en **`/Game/OneManStudio/Environment/Bank`**:

| Asset | Uso |
| --- | --- |
| SM_MeshyBank | Shell importado del FBX, Nanite activado |
| M_MeshyBank | Material PBR de dos caras, base color/normal/metallic/roughness |
| T_Bank_BaseColor | Fuente 8192 x 8192, sRGB |
| T_Bank_Normal | 4096 x 4096, compresion normal, sin sRGB |
| T_Bank_Metallic / T_Bank_Roughness | 4096 x 4096, mascaras lineales sin sRGB |
| MI_BankGround / MI_BankCounter | Materiales simples del suelo exterior y mostrador provisional |

El mesh tiene **936,752 triangulos** en LOD0. Se conserva el fallback completo para que **Use Complex Collision As Simple** use el hueco real de las puertas; no se usa una caja que cierre la entrada. La geometria de colision es de dos caras. Nanite sigue activado para render. Es un escenario estatico: no debe convertirse en objeto con simulacion fisica.

Las texturas tienen **Maximum Texture Size = 4096** para contener memoria de compilacion/cook y runtime. La fuente 8K no se redujo ni se reemplazo: puedes subir ese limite en Texture Editor despues de perfilar tu PC objetivo. No se midio aun un presupuesto de FPS/memoria para hardware minimo.

En el nivel, el actor **BANK - Meshy Diorama** tiene yaw 90 y escala uniforme aproximada **0.094586** despues de la conversion de importacion. El edificio ocupa **18.0 x 13.29 x 6.87 metros**. La puerta azul esta al lado -X y la salida gris al +X. Se verifico que el Character existente atraviesa la entrada caminando; no se reemplazo su rig ni sus animaciones.

## Actores del nivel

| Actor | Configuracion relevante |
| --- | --- |
| Movie player start | (-650, 0, 110), mirando hacia +X, dentro del umbral |
| ONE MAN STUDIO - Movie Manager | Auto Start; 3 roles; 10/30 s; offsets locales de roles Y = 0 / +120 / -120 |
| Movie premiere camera | (100, -500, 950), mira a (100, 50, 120), FOV 100; asignada al manager |
| Bank entry objective | Tag **Bank.Entry**, centro (-450, 0, 110) |
| Bank counter objective | Tag **Bank.Counter**, centro (40, 150, 110) |
| Bank escape objective | Tag **Bank.Escape**, centro (1180, 0, 110), fuera de la puerta trasera |
| Money bag pickup | Pickup ID **Bank.MoneyBag.01**, encima del mostrador |
| Donut pickup / Banana pickup | IDs **Bank.Donut.01 / Bank.Banana.01**, tambien en el mostrador |
| Bank counter / Counter top | Cubos provisionales, colision normal para el jugador |
| Bank daylight / Soft interior fill / Bank ambient sky | Luz y relleno del escenario; editables |
| Bank exposure and clear motion | Post Process Volume unbound; exposicion fija EV100 15 y motion blur desactivado |

World Settings usa **StudioGameMode** y no requiere un Level Blueprint nuevo. Se reutilizan BP_ThirdPersonCharacter y BP_ThirdPersonPlayerController originales. Los tres tags de zona, los IDs de pickups y la camara ya estan conectados a los sistemas existentes. Las zonas de objetivo son invisibles durante juego.

Puedes mover la camara final o retocar las luces en el Editor. Si cambias dimensiones/puertas del edificio, ajusta tambien PlayerStart, zonas, mostrador y props: no estan adjuntos al mesh del banco. Para personalizar scripts/objetivos/outfits, usa **MovieManager > Roles**; para assets de ropa, **Cosmetic Catalog**. Gestos y audio se asignan en ActingComponent como explica [ActingEvents.md](ActingEvents.md).

## Premiere, resultados y UMG

El banco usa el mismo MovieManager, recorder, reloj compartido y ReplayActor. Cada outfit, movimiento y evento de gesto/sonido/prop procede de su performance independiente. La camara fija solo cambia el escenario de presentacion; no se recalculan scores al ver Replay.

La interfaz nativa funciona sin assets adicionales. Para sustituirla, crea un **Widget Blueprint hijo de MoviePresentationWidget**, asignalo en **Movie Presentation Widget Class**, y usa On Presentation Changed/Data y las funciones heredadas Replay Movie/Record New Movie. En **Results Settings** configura titulo, criticas, textos de ratings/premios y reglas de tags; en **Premiere Settings**, fades, bandas, musica, stings y volumen. Fuentes/colores/fondos y botones se editan en la clase del widget. Setup completo: [PremiereAndResults.md](PremiereAndResults.md).

La fase 7 agrega el Main Menu dentro del mismo nivel y habilita Return to Main Menu en resultados. No requiere otro mapa ni asignar Main Menu Map al usar el front end. No se agrego MP4, video encoding ni Scene Capture.

## Pruebas repetibles

Con Unreal Editor cerrado, desde PowerShell:

```powershell
& 'C:\Games\OneManStudio\MyProject\Scripts\Build.ps1' -Target Editor
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-Movie.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-HeistPlay.ps1' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -UseLevelSetup -ReportSuffix Bank -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-PremierePlay.ps1' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -ReportSuffix Bank -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Package-Windows.ps1'
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-PremierePlay.ps1' -PackageDirectory 'C:\Games\OneManStudio\MyProject\Saved\Packages\BankIntegration' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -ReportSuffix Bank -RenderScreenshots
& 'C:\Games\OneManStudio\MyProject\Scripts\Test-HeistPlay.ps1' -PackageDirectory 'C:\Games\OneManStudio\MyProject\Saved\Packages\BankIntegration' -Map '/Game/OneManStudio/Maps/Lvl_Bank' -UseLevelSetup -ReportSuffix Bank -RenderScreenshots
```

Package-Windows compila Game Win64 Development y cocina Lvl_Bank. El launcher manual actual es **`C:\Games\OneManStudio\MyProject\Saved\Packages\Phase7\Windows\Play-ONE-MAN-STUDIO.bat`**. Los paquetes historicos BankIntegration y Phase6 se conservan aparte; los comandos anteriores con PackageDirectory BankIntegration prueban esa version historica. Para la entrega actual usa [MainMenuAndGameFlow.md](MainMenuAndGameFlow.md).

Los runners usan procesos independientes, UMG/render reales y duraciones cortas. El heist con UseLevelSetup usa las zonas y props guardados del banco: primero entra con input W y confirma el objetivo sin teletransporte; luego mueve el fixture para cubrir todos los objetivos. El smoke de premiere configura scores 100/75/50 para probar rating/premios y verifica botones, Replay y una nueva pelicula de cero puntos. No sustituyen una partida completa humana.

Resultados finales, tiempos de build/cook, capturas y auditoria: **[BankVerification.md](BankVerification.md)**.

## Conservacion y limites

- Solo cambiaron dos entradas de Config/DefaultEngine.ini para seleccionar el nuevo mapa; se conserva el resto de su texto. El descriptor .uproject y los assets/Blueprints originales permanecen intactos. Hay copias del mapa anterior y Config en el workspace **Saved/BankIntegration/Backups**.
- La importacion y construccion se hicieron con PythonScriptPlugin/EditorScriptingUtilities habilitados solo en la linea de comandos del Editor; no se agregaron como dependencias del juego ni se modifico el .uproject.
- Scripts/Import-MeshyBank.py y Build-BankLevel.py documentan la generacion. El segundo comprueba el hash del nivel y se niega a sobrescribirlo si lo editaste por tu cuenta. Usa el Editor normalmente para tus ajustes artisticos; no vuelvas a generar el nivel como paso rutinario.
- El FBX incluye avisos de smoothing groups/normales casi nulas. Se importo y renderizo, y la colision de la puerta paso la prueba. Si aparecen artefactos de superficie en tu acabado, corrige normales en el archivo fuente y reimporta solo este mesh.
- Mostrador, props y cosmetics siguen siendo placeholders; el archivo recibido solo aporta el shell del banco y sus texturas. No se inventaron assets finales.
- Pruebas con **NoSound**: verifican eventos, IDs y sincronizacion, pero falta escuchar la mezcla con tus clips finales. Shipping, otras PCs/GPU y resoluciones diferentes no estan verificados.
