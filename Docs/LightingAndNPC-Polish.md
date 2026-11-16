# FINAL TAKE — corrección urgente de iluminación y NPC

La versión POLISHED conserva el banco, cámaras, vestuario, grabaciones y puntuación.

- La captura real de la película tiene exposición propia fija: FilmExposureEV=11.75.
- El cine usa AudienceExposureCompensation=1.5 y dos luces de relleno suaves que
  sólo se encienden durante el estreno. StopPresentation las apaga al salir.
- Cada película nueva elige ojos, boca y cejas válidos para sus cuatro NPC.
  El uniforme del rol se conserva. La apariencia se congela y se restaura al repetir.
- Los NPC usan el Idle y Walk existentes de Base3, miran alrededor y hacen gestos.
  Los testigos dan pasos cortos de hasta45cm, con consulta de colisión estática.
  Cajero y guardia conservan sus puestos. No se añadió navegación o IA externa.
- La trayectoria de cada NPC se registra con el reloj compartido y se interpola
  al reproducir. Los gestos reutilizan NPCReaction y no generan puntos del jugador.
- El camerino congela los movimientos y la animación; una película nueva destruye
  los componentes y datos anteriores. El root motion permanece ignorado.

Configuración: seleccionar el actor Theater screen, Cinema/Lighting, para ajustar
FilmExposureEV (menor valor = más brillo), AudienceExposureCompensation y
PremiereFillLumens. En MovieManager/StorySession, Story/NPC controla
bRandomNPCFaces, bAnimateNPCs y NPCVariationSeed. Cero renueva las caras; un valor
fijo permite repetir el reparto durante edición. No requiere UMG nuevo.

Verificación ejecutada: compilación Editor correcta;76 pruebas aprobadas y cuatro
historias completas (Police, Civilian, Escape, Failure), cada una con dos películas,
reinicio y HUB. Las nuevas comprobaciones verifican caras congeladas y diversas,
poses exactas de NPC al terminar, pausas y limpieza de las luces.

Después se ajustó la exposición de11 a11.75, se suavizaron los pasos y se redujo
la velocidad del ciclo de caminar. Esa revisión compiló correctamente. Por petición
expresa del director se canceló la siguiente ronda de pruebas y se dio prioridad
al paquete Shipping. No se afirma una nueva partida manual de esa revisión.

El proyecto activo C:/Games/OneManStudio/MyProject contiene la fuente actualizada.
El ZIP editable FINAL anterior corresponde al checkpoint anterior; se conserva
como respaldo. El nuevo ZIP Windows POLISHED contiene esta corrección.
