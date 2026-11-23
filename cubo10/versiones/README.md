# Versiones del cubo 10×10×10

Cada versión queda guardada en GitHub con una **etiqueta** (`cubo10-v1.0`, `cubo10-v2.0`…) y
tiene su propio documento aquí: qué cambió, cómo se usa y cómo probarla.

| Versión | Etiqueta | Fecha | Qué trae |
|---|---|---|---|
| [1.0](v1.md) | `cubo10-v1.0` | 2026-11-22 | El driver de la Pico para el 10×10×10 (16 niveles de brillo), el diagnóstico y 5 animaciones |

## Volver a una versión anterior
```bash
git fetch --tags
```
```bash
git checkout cubo10-v1.0
```
Para volver a la última:
```bash
git checkout main
```

## Cómo se agrega una versión
1. Hacer los cambios y probarlos.
2. Escribir `versiones/vN.md` con el mismo formato, y agregar la fila en esta tabla.
3. Commit, y luego la etiqueta, y subir las dos:
   ```bash
   git tag -a cubo10-vN.0 -m "Cubo 10x10x10 vN.0"
   ```
   ```bash
   git push origin main cubo10-vN.0
   ```
