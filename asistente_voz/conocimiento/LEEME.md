# Lo que sabe el cubo

Esta es la base de conocimiento de **Cali Abstracto**, la que usa el asistente. Está en GitHub
para que la Pi la reciba con `git pull`: se edita aquí (en el Mac), se sube, y en la Pi se baja.
La plantilla vacía está en `../conocimiento_ejemplo/`.

**La base de productos (`productos.xlsx`) nunca se sube**, porque tiene costos internos. En su
lugar se sube `productos_publicos.csv` (desde la v5.3): solo los productos activos con su precio
de venta, sin costo, precio al por mayor, proveedor ni códigos. Se genera en el Mac con
`python productos.py --publico ~/Downloads/Users.xlsx`. No se suben `productos.json` ni
`catalogo.md`, que la Pi genera al arrancar.
Este repositorio es público: no pongas aquí nada que no quieras que se vea.

| Archivo | Qué es |
|---|---|
| `personalidad.md` | Quién es Cubo y cómo habla. Reemplaza las `INSTRUCCIONES` de `config.py` |
| `negocio.md` | Todo sobre la tienda: horarios, productos, precios, artesanos, pagos, envíos |
| `cali.md` | Guía turística de Cali (**borrador para revisar**) |
| `preguntas_frecuentes.md` | Preguntas con respuesta exacta, que se dicen tal cual y al instante |
| `productos.xlsx` (o `.csv`) | **Tu base de productos** (v5.1). La copias tú; no está en la plantilla |
| `nombres_productos.md` | Cómo se dice cada categoría, material y ciudad de la base (v5.1) |
| `catalogo.md`, `productos.json` | Los genera el asistente desde la base. No los edites |
| Cualquier otro `.md` o `.txt` | También se usa. Por ejemplo, `productos_navidad.md` |

## Cómo escribir
- Usa títulos con `##` para separar temas. Cada sección es lo que el cubo puede encontrar de una
  vez, así que **cada sección debe entenderse sola**: repite el nombre del producto o del lugar,
  y no escribas solo "abre a las 9".
- Frases cortas y completas, como se las dirías a un cliente.
- Los párrafos que tienen `[COMPLETAR` se ignoran: así puedes ir llenando la plantilla de a poco.
- Lo que va entre `<!--` y `-->` es un comentario y el cubo no lo lee.

## La base de productos (v5.1)
Exporta la base desde tu sistema y cópiala aquí como `productos.xlsx`, con la hoja `Producto`
y las columnas `Producto`, `Diseño`, `Material`, `Ciudad`, `Precio` y `Activo`. Al arrancar,
si la base es más nueva que `productos.json`, el asistente la importa solo:
- usa solo los productos activos con precio, y quita las filas vacías y los servicios;
- **descarta el costo, el precio al por mayor, el proveedor, el QR y la imagen**, que nunca
  llegan al cubo;
- genera `catalogo.md` (un resumen por categoría) y `productos.json` (para buscar precios
  exactos).

Para importarla a mano: `.venv/bin/python productos.py`.

## Después de editar
Reinicia el asistente. Al arrancar vuelve a leer la carpeta, y solo calcula de nuevo las
secciones que cambiaron.

Para revisar qué encuentra con distintas preguntas, corre `.venv/bin/python diagnostico.py` (la
prueba "Conocimiento (v5)"). También puedes mirar el log del asistente: cada respuesta muestra
de qué secciones sacó la información.
