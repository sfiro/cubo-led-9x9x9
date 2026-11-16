# Lo que sabe el cubo

Esta carpeta es la **plantilla**. La que usa el asistente es `asistente_voz/conocimiento/`:
`instalar.sh` la crea copiando esta la primera vez. Si `conocimiento/` no existe, el asistente usa
esta. `conocimiento/` no se sube a GitHub, igual que `config.py`. Así los datos de tu tienda
quedan solo en la Pi. Guarda una copia aparte.

| Archivo | Qué es |
|---|---|
| `personalidad.md` | Quién es Cubo y cómo habla. Reemplaza las `INSTRUCCIONES` de `config.py` |
| `negocio.md` | Todo sobre la tienda: horarios, productos, precios, artesanos, pagos, envíos |
| `cali.md` | Guía turística de Cali (**borrador para revisar**) |
| `preguntas_frecuentes.md` | Preguntas con respuesta exacta, que se dicen tal cual y al instante |
| Cualquier otro `.md` o `.txt` | También se usa. Por ejemplo, `productos_navidad.md` |

## Cómo escribir
- Usa títulos con `##` para separar temas. Cada sección es lo que el cubo puede encontrar de una
  vez, así que **cada sección debe entenderse sola**: repite el nombre del producto o del lugar,
  y no escribas solo "abre a las 9".
- Frases cortas y completas, como se las dirías a un cliente.
- Los párrafos que tienen `[COMPLETAR` se ignoran: así puedes ir llenando la plantilla de a poco.
- Lo que va entre `<!--` y `-->` es un comentario y el cubo no lo lee.

## Después de editar
Reinicia el asistente. Al arrancar vuelve a leer la carpeta, y solo calcula de nuevo las
secciones que cambiaron.

Para revisar qué encuentra con distintas preguntas, corre `.venv/bin/python diagnostico.py` (la
prueba "Conocimiento (v5)"). También puedes mirar el log del asistente: cada respuesta muestra
de qué secciones sacó la información.
