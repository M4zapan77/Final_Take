# FINAL TAKE — GameJam 2026

La versión actual usa tres personajes persistentes, dos rondas y seis turnos
cronológicos. Hay tres camerinos de hasta 30 segundos y turnos de hasta 10 segundos
con corte anticipado. Movimiento, gestos, sonidos, ropa, inventario, dinero,
consecuencias y trayectorias físicas alimentan la película real.

## Informes actuales

- [Integración del arte y archivos modificados](UI/Integration.md).
- [Pruebas ejecutadas y límites de su alcance](UI/Testing.md).
- [Capturas reales](Screenshots/FinalUI) y [JSON de pruebas](Tests/FinalUI).
- [Configuración exacta en Unreal Editor](../Docs/FinalUI-EditorSetup.md).
- [Optimización de grabaciones y auditoría de recursos](Optimization/RecordingAndCleanup.md).

La UI aprobada conserva el vídeo de fondo, añade selección de película en el
estudio, integra las ocho categorías del camerino y muestra objetivos, controles,
resultado real, puntuación y premios. El idioma español/inglés se conserva entre
sesiones. No se generó arte nuevo, no se sustituyó el rig y no se editaron mapas o
vídeos en esta fase. Falta una portada oficial del banco: se usa el logo aprobado.

Las pruebas incluyen 69 casos automáticos (68 éxitos y uno con la advertencia
esperada de configuración inválida), menús en 1280×720 y 1920×1080, tres desenlaces
con dos reproducciones, eventos independientes, vestuario y nueva sesión limpia.
También se ejecutó una copia Windows Development fuera del Editor con vídeo,
clics en Slate, viaje de nivel, seis turnos, repetición y cambio de idioma.

La configuración de distribución es Shipping con compresión Pak/IoStore. El script
comprueba los 47 paquetes de UI en el manifiesto: no basta con que UAT finalice.
La comprobación manual del ejecutable Shipping quedó pendiente al encontrar
Windows bloqueado; la validación automática empaquetada se identifica como
Development, no como una partida humana en Shipping. El informe de entrega
registra los ZIP definitivos, los tamaños y la limpieza realizada.

## Versiones recuperables y repositorio

Se conserva la versión anterior al rediseño y el checkpoint anterior a la UI,
fuera de los cachés del proyecto. Son versiones históricas. La versión final
corrige una regresión de gestos/sonidos encontrada después de la optimización M7;
se recomienda usar la distribución final para jugar.

Repositorio: [M4zapan77/Final_Take](https://github.com/M4zapan77/Final_Take).
La rama local completa es `final-take/final-ui`; los hashes reales de entrega se
registran en sus informes. La publicación remota de progreso usa una instantánea
separada de código, configuración, traducciones y evidencias. No contiene el arte
y los vídeos de varios GB: el ZIP completo del proyecto es la entrega editable.
No se suben cachés ni paquetes Windows directamente al repositorio.
