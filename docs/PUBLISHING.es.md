# Publicación

Este export se preparó mediante una lista explícita de archivos. No se copió el proyecto privado completo. Las fuentes de dependencias se extrajeron de sus archivos oficiales con checksum verificado.

Contenido permitido: motor y servicios, traductor genérico, metadata de direcciones y hash, parches propios, fuentes de SDL2/Lua, pruebas sintéticas, documentación, icono original y workflow de compilación.

Contenido excluido: ROM, ASM recuperado, CFG con bytes o operandos, bancos C generados, módulos importados, cachés, mapas, colisiones extraídas, CHR, audio, capturas, vídeos, snapshots y la alfa anterior con contenido incrustado.

Antes del primer commit, desde esta carpeta:

```sh
python3 -B tools/public/audit_public.py .
python3 -B tests/public_import_checks.py
git init
git add .
git status --short
git commit -m "Add native runtime with user ROM import"
```

No ejecutes esos comandos en la carpeta privada de investigación. No adjuntes la caché de importación ni el paquete de la alfa 0.1 a una issue o release.

El workflow `Build content-free frontends` se ejecuta al subir cambios o manualmente desde Actions. Se pueden descargar sus artefactos y probarlos antes de crear una release. Los runners no tienen una ROM del juego. Las pruebas de CI no acreditan una partida completa.

La auditoría automática detecta archivos y directorios privados comunes, campos inesperados del perfil y, si se aporta localmente `--reference-rom`, bloques binarios del archivo original. No demuestra la ausencia de cualquier posible obra derivada ni sustituye una revisión jurídica. La distribución de una herramienta sin ROM no garantiza por sí sola ausencia de reclamaciones.

Mantén los nombres e iconos públicos propios del proyecto. La mención al juego en README identifica la revisión compatible. MIT se aplica sólo a las contribuciones propias; los recursos del juego no pasan a estar bajo MIT al importarlos.
