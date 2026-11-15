# Fase 3 - Resultados reales de verificacion

Fecha: 12 de noviembre de 2026. Proyecto: `C:\Games\OneManStudio\MyProject\MyProject.uproject`.

## Entorno y compilacion

- Motor instalado: **Unreal Engine 5.6.1**, changelist 44394996, compatible con 5.6.0. Instalacion: `C:\Program Files\Epic Games\UE_5.6`.
- Visual Studio 2022 / MSVC 14.44.35230, Windows SDK 10.0.22621.0 y .NET incluido en el motor.
- **MyProjectEditor / Win64 / Development: Succeeded.** La ultima compilacion incremental tardo 17.99 s. DLL actualizada: `Binaries\Win64\UnrealEditor-MyProject.dll`.
- **MyProject / Win64 / Development: Succeeded.** La ultima compilacion incremental tardo 34.46 s. Ejecutable actualizado: `Binaries\Win64\MyProject.exe`. No se realizo empaquetado/cook.
- Logs: `Saved\Logs\Build-MyProjectEditor.log` y `Saved\Logs\Build-MyProject.log`.
- UBT mantiene su advertencia previa: MSVC 14.44 no es la version preferida para este motor (recomienda 14.38). No impidio compilar ni enlazar.

## Automatizacion

Comando ejecutado en el proyecto activo: `Scripts\Test-Movie.ps1`.

Reporte final generado el **2026.11.12-20.15.20 UTC**: `Saved\Automation\Movie\index.json`.

**14 Success, 0 succeededWithWarnings, 0 failed, 0 notRun, 0 inProcess.** El script comprobo que el reporte fuera nuevo y que estuvieran presentes las catorce pruebas requeridas. La simulacion de los tests duro 0.505 s; los tiempos de juego se avanzaron mediante ticks, incluyendo el bucle 10/30 completo.

Se preservaron las nueve regresiones existentes: RecordingData, RecorderSampling, SharedClockAndReplay, ReplayCollision, ConcurrentRecordingAndReplay, ThreeTakeLoop, RestartAndCleanup, DefaultTimingAndFinishedRestart y CharacterMovementLifecycle.

Las cinco pruebas de ActingEvents verificaron:

| Prueba | Evidencia |
| --- | --- |
| DataValidation | Timestamps validos, orden de eventos simultaneos, limites del clip, rechazo de NaN/infinito/IDs vacios/tipos desconocidos, copias independientes y compatibilidad con tomas sin eventos |
| CaptureAndGating | Cinco gestos y seis sonidos aceptados, registro una vez por accion, timestamp del reloj compartido incluso en pausa, configuracion copiada por toma, cancelacion explicita, bloqueo de input fuera de grabacion y rechazo de grabar actores replay |
| PlaybackTimeline | Preparacion silenciosa, evento de tiempo cero una vez, eventos simultaneos y catch-up de un frame largo, dos actores independientes, pausa sin duplicados, rewind, evento final, cursor reiniciado y listener que detiene el replay durante un callback |
| ThreeTakeMovieAndRestart | Eventos distintos en las tres tomas, replays previos actuando mientras el jugador registra otra toma, tres reproducciones en premiere, reinicio sin eventos anteriores, disponibilidad por fase y teclas sin duplicarse |
| AnimationPresentation | Secuencia y montage transitorio con slot personalizado, tiempo absoluto/play rate, transform del actor controlado por el recording, evaluacion real de huesos, pausa de pose incluso durante blend-in, root motion ignorado y restaurado, parada del montage propio y colision desactivada |

La prueba de animacion cargo para lectura `SKM_Quinn_Simple` y `MF_Unarmed_Walk_Fwd` del template existente. Uso esa secuencia como gesto de prueba, no como animacion artistica de baile. El montage se creo en memoria; no se guardo ningun asset.

El log completo de la ultima ejecucion esta en `Saved\Logs\MovieTests.log`. El motor emite tres lineas `LogAutomationTest: Error: Condition failed` durante su arranque, antes de iniciar la suite; las mismas lineas ya estaban en el log de la fase 2. No aparecen como fallos ni advertencias de estas catorce pruebas. No se alteraron plugins del Editor para investigar ese mensaje previo.

## Archivos protegidos

Se compararon hashes SHA-256 de **250 archivos en Content y Config antes y despues: cero diferencias**, sin archivos nuevos ni eliminados en esos arboles. `MyProject.uproject` tambien conserva su contenido. Los cambios se limitaron a Source, el script de pruebas y documentacion. Se conservaron copias de los archivos originales en el workspace de desarrollo.

## Lo que requiere revision manual

Las pruebas se ejecutaron con **NullRHI y NoSound**. Verificaron los eventos de sonido y su sincronizacion logica; **no verificaron salida audible, atenuacion, concurrencia ni mezcla de voces**. La evaluacion CPU de las poses paso, pero no se revisaron imagenes renderizadas con los assets finales ni el encuadre del mapa del director.

Las cinco animaciones de gesto y los seis sonidos finales quedan **sin asignar**. Antes de verificar la presentacion en Play, el director debe asignarlos en ActingComponent y conectar los slots necesarios en su Anim Blueprint. Hasta entonces, los eventos se graban y notifican al HUD, y OnPresentationIssue indica las referencias que faltan.

Para completar la revision manual, sigue [ActingEvents.md](ActingEvents.md): `Movie.Test 2 10`, gesto 1 y sonido Z en la primera toma, acciones distintas en las siguientes y R durante una grabacion o la premiere. Revisa tambien una sirena en loop al reiniciar: ninguna voz anterior debe continuar. No se implementaron vestuario, objetivos, puntuacion ni efectos cinematicos adicionales.
