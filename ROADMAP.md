# Hoja de ruta

Esta hoja de ruta no convierte trabajo planificado en trabajo terminado.

## Prioridad inmediata

1. Mantener A02 reproducible y resolverlo solo con una corrección de producción y regresión equivalente.
2. Repetir la matriz CI en commits actuales y conservar inventario CTest, JUnit y verificaciones de paquetes por plataforma.
3. Completar pruebas nativas en Windows y Linux/Steam Deck y separar claramente build, paquete, ejecución y gameplay.
4. Ampliar pruebas de runtime: DMA/IRQ, PPU, APU, MMC1, audio, importación concurrente y recuperación de caché.
5. Ejecutar sesiones interactivas controladas con la ROM privada del usuario para menú, idioma, controles, sonido, habitaciones, muertes y rutas.
6. Evaluar widescreen, habitaciones verticales/fijas y activación de enemigos sin alterar la protección de contenido público.

## Mods y lanzador 3D

Documentar y probar la API Lua existente antes de ampliar capacidades o compatibilidad. El prototipo 3D debe seguir aislado hasta que exista una decisión explícita de integración, modelo de interacción y pruebas por plataforma.

## Versión 1.0

No hay fecha ni criterio aprobado de publicación 1.0 en el checkout. Antes de proponerla se necesita una matriz de aceptación reproducible: A02 resuelto, CI verde sin ocultar controles, paquetes nativos verificados, importación ABI v3 validada, gameplay/rutas documentados, límites de mods definidos, revisión legal/licencias y un procedimiento de publicación content-free. Hasta entonces, las releases deben seguir marcadas como alpha/experimental.
