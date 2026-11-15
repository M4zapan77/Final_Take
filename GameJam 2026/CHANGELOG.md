# Cambios

## 2026-11-15 — FINAL TAKE 1.0, integración y verificación final

- Se integraron 40 PNG únicos, la fuente Bungee con licencia y un tema editable.
  Menú vertical, estudio de película, camerino de ocho categorías, HUD compacto y
  resultados conservan la arquitectura y los assets existentes.
- Se corrigieron el foco tras cargar niveles, la navegación de regreso y las zonas
  de clic durante la respuesta visual. Los paneles conservan el mismo vídeo.
- Se corrigió una regresión de M7: el registro compacto había omitido ajustes
  necesarios para ejecutar gestos, sonidos y pasos. Se restauró el snapshot activo,
  manteniendo la compresión de poses y evitando duplicados en segmentos terminados.
- Se corrigió el cocinado de UI cargada dinámicamente y se añadió validación de
  los 47 paquetes del directorio UI. No se fuerzan escenarios sin uso al paquete.
- Se excluyeron de Git los registros generados FileOpenOrder y se limitaron las
  pruebas de previsualización de idioma a compilaciones Editor.
- Pasaron 69 pruebas automáticas, menús 720p/1080p, personalización de 39 pasos,
  animación/audio y tres historias con repetición y reset. Se añadieron pruebas
  empaquetadas de UI y del ciclo completo fuera del Editor.
- El flujo normal conserva seis turnos, dos rondas, tres camerinos, roles, objetos,
  NPCs, objetivos, puntuación 40/30/20/10, cine y localización ES/EN.
- La limpieza de recursos M7 eliminó 16 assets de plantilla auditados y 45 copias
  ZIP/RAR; todos están en una recuperación verificada. Se preservan los originales
  extraídos, el arte aprobado, el historial y las versiones recuperables.

Los informes de distribución y limpieza distinguen las acciones ejecutadas de
las comprobaciones manuales pendientes. No se promete 60 FPS ni prueba en otro PC.

## 2026-11-14 — checkpoint jugable antes del rediseño

- Se corrigieron dimensiones, apoyos, superficies, encuadre y asientos del banco/cine.
- Se adaptaron copias de máscara y gafas a la cabeza redonda, conservando fuentes,
  rig, animaciones y seis máscaras de vestuario.
- Se importaron 34 recursos de audio, mezcla global por categorías y pasos grabados.
- Se verificaron 52 pruebas unitarias y un flujo Windows de tres tomas con replay,
  nueva película, personalización, vídeo, arte, movimiento y audio.
- Se preservó la carpeta Windows completa y un ZIP con CRC y SHA256 fuera de cachés.
