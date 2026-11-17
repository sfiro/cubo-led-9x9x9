# Versiones del asistente de voz

Cada versión queda guardada en GitHub con una **etiqueta** (`asistente-v1.0`, `asistente-v2.0`…)
y tiene su propio documento aquí: qué cambió, cómo se usa y cómo probarla. El código siempre
está en un solo lugar, `asistente_voz/`. La etiqueta es una foto de cómo estaba en esa versión.

| Versión | Etiqueta | Fecha | Qué trae |
|---|---|---|---|
| [5.3](v5.3.md) | `asistente-v5.3` | 2026-11-16 | La lista pública de productos en el repo (sin costos ni proveedores): la Pi ya no necesita el Excel |
| [5.2](v5.2.md) | `asistente-v5.2` | 2026-11-16 | La base de conocimiento de Cali Abstracto en el repo, para bajarla con `git pull` |
| [5.1](v5.1.md) | `asistente-v5.1` | 2026-11-16 | La base de productos: precios reales de cada producto, sin costos ni datos internos |
| [5.0](v5.md) | `asistente-v5.0` | 2026-11-16 | El cubo de la tienda: responde sobre la tienda y sobre Cali con documentos (RAG) y preguntas frecuentes |
| [4.1](v4.1.md) | `asistente-v4.1` | 2026-11-16 | El modo demo dice el nombre de cada animación antes de mostrarla |
| [4.0](v4.md) | `asistente-v4.0` | 2026-11-16 | Se despierta con "hola cubo" y queda atento para conversar |
| [3.0](v3.md) | `asistente-v3.0` | 2026-11-03 | 16 niveles de brillo (BAM): bordes suaves, estela en las animaciones y brillo por voz |
| [2.0](v2.md) | `asistente-v2.0` | 2026-11-02 | Controlar las animaciones del cubo con la voz, sin dejar de conversar |
| [1.0](v1.md) | `asistente-v1.0` | 2026-11-02 | Asistente de voz local; el cubo se ilumina al ritmo de la voz |

El programa dice su versión al arrancar (`Asistente de voz v5.3`), y el diagnóstico también.

## Volver a una versión anterior

En la Pi, dentro de `~/cubo-led-9x9x9`:
```bash
git fetch --tags
```
```bash
git checkout asistente-v1.0
```
Para volver a la última:
```bash
git checkout main
```

`config.py`, las voces y el entorno de Python no están en el repo, así que no cambian al pasar
de una versión a otra. Una versión nueva puede agregar opciones a `config_ejemplo.py`; si tu
`config.py` no las tiene, el programa usa sus valores predeterminados.

También se puede descargar cada versión como `.zip` desde la página de etiquetas del repo:
<https://github.com/sfiro/cubo-led-9x9x9/tags>.

## Cómo se agrega una versión

1. Hacer los cambios y probarlos.
2. Subir `VERSION` en `raspberry/asistente.py`.
3. Escribir `versiones/vN.md` con el mismo formato de las anteriores, y agregar la fila en esta
   tabla.
4. Commit, y luego la etiqueta, y subir las dos:
   ```bash
   git tag -a asistente-vN.0 -m "Asistente de voz vN.0"
   ```
   ```bash
   git push origin main asistente-vN.0
   ```
