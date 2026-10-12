# Contribuir

Lee primero [AGENTS.md](AGENTS.md), [PROJECT_STATUS.md](PROJECT_STATUS.md) y [DEVELOPMENT_SETUP.md](DEVELOPMENT_SETUP.md).

Usa una rama por tarea (`infra/...`, `ci/...`, `fix/...`, `docs/...`). Haz cambios pequeños, explica el motivo y abre un Pull Request; no integres directamente en la rama principal. No se cambian protecciones remotas ni permisos desde este repositorio sin autorización del mantenedor.

Antes del PR, ejecuta el build y CTest aplicables, la auditoría pública y las pruebas del área modificada. Conserva A02 como discrepancia visible y describe sus resultados por separado. En el PR indica commit base, plataforma, comandos, PASS/FAIL/no ejecutado y limitaciones. No incluyas ROMs, módulos, cachés, secretos ni recursos derivados.

Los cambios de runtime, ABI, importador, CI o empaquetado requieren revisión adicional. No mezcles refactors grandes con correcciones de comportamiento. Si encuentras una regresión, detén la ampliación de alcance, añade una reproducción mínima y enlaza el informe técnico correspondiente.
