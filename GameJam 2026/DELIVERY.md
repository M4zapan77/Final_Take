# FINAL TAKE — entrega final GameJam 2026

Unreal Engine 5.6.1, Windows 64 bits. Código del paquete: `0882f316a8fc4ce1f29001fdd766162a78b40d6e`.
Rama local completa: `final-take/final-ui`. El mapa y binarios de esta entrega
corresponden al mismo checkpoint; los commits posteriores añaden documentación.

## Jugar y recuperar el proyecto

- Extraer **FINAL_TAKE_GameJam2026_FINAL_Windows.zip** completo y ejecutar
  `Windows/Play-FINAL-TAKE.bat`. También hay un lanzador JUGAR_FINAL_TAKE.bat en el espacio de trabajo.
- ZIP Windows: **574.20 MB**, 33 archivos, CRC verificado.
- SHA256 Windows: `09c7131917ba0dac5b78bd6a88a5b03fd9a92462e47afa1fbf792cdf3286f831`.
- **FINAL_TAKE_GameJam2026_FINAL_Project.zip** contiene MyProject.uproject, Source,
  Content, SourceArt, configuración, traducciones, scripts y documentación. Los
  recibos JSON y SHA256 externos registran el tamaño e integridad de ambos ZIP.
  No incluye .git, Binaries, Intermediate, Saved ni otros cachés.
- Proyecto activo: `C:/Games/OneManStudio/MyProject/MyProject.uproject`.
- [Configuración exacta del banco en Editor](../Docs/FinalRelease-EditorSetup.md).
- [Tema, widgets y asignación del arte UI](../Docs/FinalUI-EditorSetup.md).

## Cambios finales

Se conservó la arquitectura de grabación, reloj, seis turnos, tres camerinos,
gestos, sonidos, ropa, objetos, objetivos, puntuación, director y cine.

- La cámara vuelve al personaje controlado al cerrar cada camerino. La captura
  del cine marca los cortes para no reutilizar la historia visual anterior.
- El mismo Lvl_Bank tiene escala corregida, suelo continuo, techo crema, rellenos
  cálidos, exterior verde y entrada alineada con su hueco real. Cajas a la izquierda,
  espera al frente derecho, bóveda al fondo y producción al fondo derecho.
- Siete puntos de aparición con espacio de cápsula comprobado. Los protagonistas
  comienzan en lugares distintos y sólo se inicializan una vez por película.
- La hoja de bóveda importada ahora tiene bisagra, movimiento con reloj compartido,
  bloqueo de cierre si hay alguien en el paso y eventos de reproducción. Su cuarto
  se completó con paredes, techo, pedestal y muebles existentes: el modelo aportado
  es una hoja sólida, no un interior transitable completo.
- E abre/cierra la bóveda; se puede recoger su bolsa única o pedir el dinero al
  cajero. Para huir, el guardia debe estar distraído; el HUD aclara esa condición.
- Límites y suelo invisibles independientes de la cámara. Recuperación de la
  misma instancia del personaje o prop sin reiniciar turno, vestuario o inventario
  ni duplicar el dinero. La recuperación también se registra.
- Se mantienen los assets y widgets del artista. Sólo se añadieron seis materiales
  locales de acabado; no se sustituyó el rig ni se reescribieron los sistemas.

La relación de archivos está en [FilesModified.txt](FinalRelease/FilesModified.txt).
Los cambios de código principales son MovieManager, StudioCinemaPresenter,
StorySessionWorld/Props/Set, StudioBankBounds, StudioSetDoor y sus pruebas.

## Pruebas ejecutadas

- Development Editor compilado; **73 pruebas** aprobadas: 72
  éxitos y 1 con advertencia esperada de configuración inválida.
- Shipping reempaquetado: UAT **código 0**, 140.30 s.
  Development final: UAT **código 0**, 145.43 s.
  Pak/IoStore comprimido, prerrequisitos incluidos. La primera compilación completa
  previa al ajuste de spawn tardó 240.97 s / 214.45 s respectivamente.
- Cocinados y comprobados: **47 paquetes UI y seis materiales nuevos**, ninguno ausente.
- **13 ejecuciones nativas aprobadas**: cuatro historias en Editor, cuatro en
  Windows Development empaquetado, cuatro pruebas de vídeo empaquetadas y una
  repetición completa de Policía después del último ajuste de su punto inicial.
- Historias Policía, Civil, Escape y Sin objetivos: seis turnos, tres camerinos,
  cámara del rol correcto, ropa independiente, dinero único, gestos y sonidos,
  dos reproducciones, puntuación estable, nueva película limpia y regreso al HUB.
  El civil lanza un objeto físico; Escape abre la bóveda, toma su dinero, atraviesa
  la entrada y huye después de distraer al guardia. Sin objetivos sigue siendo
  visible y puntuable. La segunda ronda conserva poses e inventario.
- Vídeos empaquetados: reproducción automática de ambos intros, salto desde
  cualquiera consumiendo el clic, HUB en bucle y paneles interactivos, fundido,
  viaje real, audio detenido, entrada de juego y alternativa para medios fallidos.
- **Shipping probado con ratón y teclado**, sin harness ni consola de desarrollo:
  Menú y ajustes, camerinos y selección real, seis turnos, gesto/sonido, estreno, resultados, repetir con el mismo 4/100, nueva película limpia con 0/100, regreso al estudio, menú y Salir; cámara del policía revisada tras el ajuste final.
  El ciclo visual completo se comprobó antes del último desplazamiento de 220 cm
  del spawn policial; después se volvió a comprobar esa cámara en Shipping final.
  Las doce ejecuciones anteriores se conservan con su procedencia; la repetición
  de Policía corresponde al mapa definitivo. El código de gameplay no cambió en
  ese último ajuste de colocación.
- Español/inglés: **309 traducciones**, cero ausentes. Los casos
  cambian idioma en resultados sin alterar grabaciones o puntuación y conservan
  la elección al iniciar otra película.

Los JSON/CSV están en [Tests/FinalRelease](Tests/FinalRelease); capturas reales en
[Screenshots/FinalRelease](Screenshots/FinalRelease). La comprobación manual tiene
su propia secuencia de observaciones y capturas, separada de las pruebas nativas.
Los fallos de colocación inicial del cajero y el primer guion de escape se
corrigieron y se volvieron a ejecutar; no figuran como aprobados en la evidencia final.

## Rendimiento y limpieza

Windows Development empaquetado, DX12 SM6, 1280×720 offscreen; Ryzen 7 7840HS,
16 GB, RTX 4060 Laptop. Las capturas y cargas forman parte de estas mediciones:

| Caso | Grabación media / p95 ms | Película media / p95 ms | Pico RAM MiB |
| --- | --- | --- | --- |
| Police | 19.437 / 16.1064 | 14.3724 / 15.8832 | 2233.78 |
| Civilian | 18.7334 / 16.123 | 14.2115 / 15.7531 | 2072.91 |
| Escape | 14.6299 / 16.1356 | 14.6017 / 16.8213 | 2050.29 |
| Failure | 11.502 / 14.3365 | 14.5019 / 17.293 | 2031.33 |

No es un benchmark Shipping ni garantiza 60 FPS. Hay picos de carga y cambio de
personaje, superiores a un segundo en algunos casos; los JSON conservan máximos
y percentiles. Los buffers estáticos M7 anteriores bajaron de 283136 a 19040 bytes;
esa reducción no representa toda la RAM ni una mejora general demostrada de FPS.

La limpieza anterior retiró **13,693,407,363 bytes** de cachés temporales históricos;
la recompilación regenera parte de ellos. Se conservan sus recibos y no se presenta
esa suma como espacio libre neto final. Las fuentes de arte, pruebas, configuración,
autosaves y respaldos están preservados.

## Respaldos, publicación y límites

- Checkpoint previo al banco: `5b424423f7d4215a9941b23f8291a3c65290d4bc`, tag
  `camera-fix-before-final-bank-2026-11-15`; mapa anterior y SHA fuera de cachés en
  `gamejam/output/recovery/BeforeFinalBank20261115`. ZIP históricos conservados.
- [GitHub](https://github.com/M4zapan77/Final_Take), rama `main`: snapshot de código,
  configuración, traducciones y evidencias. El recibo externo de publicación da
  el hash remoto verificado. Los varios GB de arte/vídeos y los ZIP completos se
  entregan localmente; clonar únicamente el snapshot no recupera todos los assets.
- Falta una portada oficial del banco; se utiliza el logo aprobado y MovieCover
  sigue configurable. La interfaz nativa funciona sin crear UMG manualmente.
- No se probó una instalación limpia ni otro PC. Los prerrequisitos de Unreal
  están incluidos. No se hizo una revisión auditiva humana de la mezcla; los
  eventos de sonido y reproductores se comprobaron en ejecución.
- VS 2022 14.44 compiló correctamente; Unreal recomienda 14.38 y conserva ese aviso.
  Los avisos de InputKey afectan al harness de Development, excluido de Shipping.
- Si el proyecto se extrae en otra ruta, pueden requerirse nuevas rutas de
  reimportación a SourceArt. Los assets cocinados del juego no dependen de ellas.
- Sin multijugador, API de IA en ejecución, exportación MP4 ni codificación de vídeo.

Ésta es la entrega de cierre. No se inicia otra fase automáticamente.
