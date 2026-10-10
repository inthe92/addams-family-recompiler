# Estado de verificación

[English](VERIFICATION.md)

Preparado el 10 de octubre de 2026. Los resultados locales exactos se guardan en `verification-public.json`, junto al ZIP público y los paquetes de cada sistema.

El motor compila sin ROM. Las pruebas públicas usan datos sintéticos y comprueban el perfil, los opcodes oficiales y el rechazo de archivos incompatibles antes de generar contenido. Otra prueba compila y carga un módulo sintético con las herramientas incluidas en el sistema donde se ejecuta.

Se han compilado los frontends para macOS ARM64, Windows x64 y Linux x64. Windows y Linux se compilan desde macOS; la compilación e inspección del paquete no acreditan su ejecución en esos sistemas. La prueba de importación y arranque en macOS utiliza una ROM privada, que no se incluye en las fuentes ni los paquetes. El informe identifica las comprobaciones ejecutadas realmente.

Pendiente: pruebas en equipos reales de Windows y Steam Deck, partida completa, rutas y fidelidad de audio, todos los casos de recuperación ante muerte y el panorámico con enemigos y habitaciones verticales o de ancho fijo. Mac Intel está configurado en CI y no se ha probado localmente. Esta entrega es una alfa, no un port terminado.
