# Productos de la tienda (v5.1): importa la base de productos (productos.xlsx o
# .csv en ../conocimiento/) y busca los productos que nombra un cliente, con su
# precio real.
#
#   python productos.py                      importa la base de ../conocimiento/
#   python productos.py ruta/a/la/base.xlsx  importa ese archivo
#   python productos.py --publico base.xlsx  crea ../conocimiento/productos_publicos.csv (v5.3)
#
# El importador deja en la carpeta de conocimiento:
#   - productos.json: solo lo que el cubo puede decir (categoria, diseno,
#     material, ciudad de origen y precio). El costo, el precio al por mayor,
#     el proveedor, el QR y la imagen NO salen de aqui.
#   - catalogo.md: un resumen por categoria, que entra en la busqueda de la v5.
# El asistente lo corre solo al arrancar si la base es mas nueva que productos.json.
#
# productos_publicos.csv (v5.3) es la lista que va en GitHub: solo productos
# activos, con su precio de venta y la ciudad de origen cuando es una ciudad. Sin
# costo, precio al por mayor, proveedor, codigos ni imagenes. Se importa igual
# que la base completa.

import csv
import difflib
import json
import os
import re
import sys
from collections import Counter

from comandos import normalizar

BASES = ("productos.xlsx", "productos.csv", "productos_publicos.csv")
PUBLICA = "productos_publicos.csv"
# Columnas que se leen. Las demas (Costo, Precio_B2B, User_id, QR, Imagen...) se ignoran.
_COLUMNAS = ("Producto", "Diseño", "Material", "Ciudad", "Precio", "Activo")
_GENERICAS = set(
    "cuanto cuesta cuestan vale valen precio precios tienen tiene hay venden vende quiero "
    "comprar compro busco buscando algun alguna algunos algunas me gustaria regalo regalar "
    "muestrame ver tienda pesos plata barato barata baratos caro cara caros de del la el los "
    "las un una unos unas en con para por que y o a al mas muy hola cubo como esta estas "
    "estoy bien gracias eres quien donde cuando puedo puedes sabes dime cuentame algo otro "
    "otra otros otras cosa cosas".split())
# Sin nombrar una categoria, solo se buscan productos si la frase suena a compra:
# "que tienen de Frida" si, "que es el champus" no.
_INTENCION = set(
    "tienen tiene hay venden vende cuanto cuesta cuestan vale valen precio precios comprar "
    "compro busco buscando quiero regalo regalar llevar".split())


# ---- nombres ----------------------------------------------------------------


def leer_nombres(carpeta):
    # nombres_productos.md -> ({categoria: (singular, plural, [alias])},
    #                          {valor: material dicho}, {valor: ciudad})
    categorias, materiales, ciudades = {}, {}, {}
    ruta = os.path.join(carpeta, "nombres_productos.md")
    if not os.path.exists(ruta):                # si no lo copiaron, el de la plantilla
        ruta = os.path.join(os.path.dirname(carpeta), "conocimiento_ejemplo", "nombres_productos.md")
    if not os.path.exists(ruta):
        return categorias, materiales, ciudades
    with open(ruta, encoding="utf-8") as f:
        texto = re.sub(r"<!--.*?-->", "", f.read(), flags=re.S)
    destino = categorias
    for linea in texto.splitlines():
        linea = linea.strip()
        if linea.startswith("##"):
            titulo = linea.lower()
            destino = ciudades if "ciudad" in titulo else materiales if "material" in titulo else categorias
            continue
        if ":" not in linea or linea.startswith("#"):
            continue
        clave, valores = linea.split(":", 1)
        valores = [v.strip() for v in re.sub(r"\(revisar\)", "", valores).split(",") if v.strip()]
        if destino is not categorias:
            destino[clave.strip().lower()] = valores[0] if valores else ""   # vacio = no decirlo
            continue
        if not valores:
            continue
        else:
            singular = "" if valores[0] == "-" else valores[0]
            plural = valores[1] if len(valores) > 1 else singular
            categorias[clave.strip()] = (singular, plural, valores[2:])
    return categorias, materiales, ciudades


def _nombres_de(categoria, nombres):
    if categoria in nombres:
        return nombres[categoria]
    n = categoria.strip().lower()
    return n, n, []


# ---- importar ---------------------------------------------------------------


def _filas(ruta):
    # Devuelve las filas como diccionarios {columna: valor}
    if ruta.lower().endswith(".csv"):
        with open(ruta, encoding="utf-8-sig", newline="") as f:
            yield from csv.DictReader(f)
        return
    from openpyxl import load_workbook
    libro = load_workbook(ruta, read_only=True, data_only=True)
    hoja = libro["Producto"] if "Producto" in libro.sheetnames else libro.worksheets[0]
    filas = hoja.iter_rows(values_only=True)
    encabezado = [str(c).strip() if c is not None else "" for c in next(filas)]
    for fila in filas:
        yield dict(zip(encabezado, fila))
    libro.close()


def _numero(valor):
    try:
        return float(str(valor).replace(",", "."))
    except (TypeError, ValueError):
        return None


def _activo(valor):
    # Excel guarda VERDADERO/FALSO; un CSV puede traer 1, "1.0", "true" o "si"
    if isinstance(valor, bool):
        return valor
    if _numero(valor) is not None:
        return _numero(valor) == 1
    return str(valor).strip().lower() in ("true", "verdadero", "si", "sí")


def importar(ruta, carpeta):
    # Lee la base y escribe productos.json y catalogo.md en la carpeta.
    # Devuelve un resumen de lo que hizo.
    nombres, materiales, ciudades = leer_nombres(carpeta)
    productos, vacias, inactivas, sin_precio = [], 0, 0, 0
    vistos = set()
    for fila in _filas(ruta):
        categoria = (fila.get("Producto") or "")
        categoria = str(categoria).strip()
        if not categoria:
            vacias += 1
            continue
        if not _activo(fila.get("Activo")):
            inactivas += 1
            continue
        precio = _numero(fila.get("Precio"))
        material = str(fila.get("Material") or "").strip().lower()
        if not precio or material == "servicio":
            sin_precio += 1
            continue
        diseno = str(fila.get("Diseño") or "").strip()
        clave = (categoria.lower(), diseno.lower(), material.lower(), precio)
        if clave in vistos:
            continue
        vistos.add(clave)
        singular, plural, _ = _nombres_de(categoria, nombres)
        productos.append({
            "categoria": categoria, "singular": singular, "plural": plural,
            "diseno": diseno, "material": materiales.get(material, material),
            "origen": ciudades.get(str(fila.get("Ciudad") or "").strip().lower(), ""),
            "precio": int(round(precio)),
        })
    with open(os.path.join(carpeta, "productos.json"), "w", encoding="utf-8") as f:
        json.dump(productos, f, ensure_ascii=False, indent=0)
    with open(os.path.join(carpeta, "catalogo.md"), "w", encoding="utf-8") as f:
        f.write(_catalogo(productos))
    return ("%d productos para el cubo (%d filas vacias, %d inactivas, %d sin precio o "
            "servicios)" % (len(productos), vacias, inactivas, sin_precio))


def _pesos(n):
    # "7000 pesos": sin separador de miles, que la voz lee mejor
    return "%d pesos" % n


def _material(m):
    # "madera" -> "de madera"; "impreso en 3D" queda igual
    return m if " " in m else "de " + m


def _rango(precios):
    a, b = min(precios), max(precios)
    return "a %s" % _pesos(a) if a == b else "de %s a %s" % (_pesos(a), _pesos(b))


def _lista(cosas):
    cosas = list(cosas)
    return cosas[0] if len(cosas) == 1 else ", ".join(cosas[:-1]) + " y " + cosas[-1]


def _catalogo(productos):
    # Un resumen por categoria, de la que tiene mas disenos a la que tiene menos
    grupos = {}
    for p in productos:
        grupos.setdefault(p["categoria"], []).append(p)
    partes = ["# Catálogo de productos\n",
              "<!-- Generado por productos.py desde la base de productos. No lo edites a mano: "
              "se sobrescribe cada vez que se importa la base. -->\n"]
    for categoria, lista in sorted(grupos.items(), key=lambda g: -len(g[1])):
        p0 = lista[0]
        disenos = [d for d, _ in Counter(p["diseno"] for p in lista if p["diseno"]).most_common(8)]
        materiales = [m for m, _ in Counter(p["material"] for p in lista if p["material"]).most_common(4)]
        origenes = sorted({p["origen"] for p in lista if p["origen"]})
        texto = "En la tienda hay %d %s de %s, %s." % (
            len(lista), "diseño" if len(lista) == 1 else "diseños", p0["plural"], _rango([p["precio"] for p in lista]))
        if materiales:
            texto += " Materiales: %s." % _lista(materiales)
        if disenos:
            texto += " Algunos diseños: %s." % _lista(disenos)
        if origenes:
            texto += " Algunos vienen de %s." % _lista(origenes)
        partes.append("## %s\n%s\n" % (p0["plural"].capitalize(), texto))
    return "\n".join(partes)


def preparar(carpeta):
    # Importa la base mas reciente (la completa o la publica) si es mas nueva que
    # productos.json. Devuelve un mensaje o None.
    salida = os.path.join(carpeta, "productos.json")
    bases = [os.path.join(carpeta, n) for n in BASES if os.path.exists(os.path.join(carpeta, n))]
    if not bases:
        return None
    base = max(bases, key=os.path.getmtime)
    nombres = os.path.join(carpeta, "nombres_productos.md")
    ultima = max(os.path.getmtime(base),
                 os.path.getmtime(nombres) if os.path.exists(nombres) else 0)
    if not os.path.exists(salida) or os.path.getmtime(salida) < ultima:
        return "Base de productos importada desde %s: %s" % (os.path.basename(base),
                                                              importar(base, carpeta))
    return None


def exportar_publico(ruta, carpeta):
    # Crea productos_publicos.csv: la lista que se puede publicar en GitHub
    _, _, ciudades = leer_nombres(carpeta)
    filas, vistos = [], set()
    for fila in _filas(ruta):
        categoria = str(fila.get("Producto") or "").strip()
        precio = _numero(fila.get("Precio"))
        material = str(fila.get("Material") or "").strip()
        if not categoria or not _activo(fila.get("Activo")) or not precio \
                or material.lower() == "servicio":
            continue
        diseno = str(fila.get("Diseño") or "").strip()
        ciudad = str(fila.get("Ciudad") or "").strip()
        ciudad = ciudad if ciudad.lower() in ciudades else ""      # el proveedor no se publica
        clave = (categoria.lower(), diseno.lower(), material.lower(), ciudad.lower(), int(round(precio)))
        if clave in vistos:
            continue
        vistos.add(clave)
        filas.append([categoria, diseno, material, ciudad, int(round(precio)), 1])
    filas.sort(key=lambda f: (f[0].lower(), f[1].lower(), f[4]))
    destino = os.path.join(carpeta, PUBLICA)
    with open(destino, "w", encoding="utf-8", newline="") as f:
        escritor = csv.writer(f)
        escritor.writerow(["Producto", "Diseño", "Material", "Ciudad", "Precio", "Activo"])
        escritor.writerows(filas)
    return "%d productos en %s" % (len(filas), destino)


# ---- buscar -----------------------------------------------------------------


def _parecida(a, b):
    # 1 = igual o plural ("gato" ~ "gatos"), 0.6 = parecida ("gato" ~ "gato3",
    # "chontaduro" ~ "chontaduros"), 0 = distinta ("gato" ~ "gatorade")
    if a == b:
        return 1.0
    corta, larga = sorted((a, b), key=len)
    if len(corta) >= 3 and larga[len(corta):] in ("s", "es") and larga.startswith(corta):
        return 1.0
    if len(a) >= 4 and difflib.SequenceMatcher(None, a, b).ratio() >= 0.84:
        return 0.6
    return 0.0


class Catalogo:
    def __init__(self, carpeta, maximo=5):
        ruta = os.path.join(carpeta, "productos.json")
        with open(ruta, encoding="utf-8") as f:
            self.productos = json.load(f)
        self.maximo = maximo
        nombres, _, _ = leer_nombres(carpeta)
        # formas de nombrar cada categoria (normalizadas), de la mas larga a la mas corta.
        # Las palabras extra ("cafe" para los alimentos) tambien sirven para buscar el
        # diseno, asi que no se quitan del texto.
        formas, self._extra = {}, set()
        for p in self.productos:
            singular, plural, alias = _nombres_de(p["categoria"], nombres)
            for forma in [singular, plural, p["categoria"]] + alias:
                if normalizar(forma):       # el singular "-" queda vacio
                    formas.setdefault(normalizar(forma), set()).add(p["categoria"])
            self._extra.update(normalizar(a) for a in alias)
        self._extra -= {normalizar(f) for p in self.productos
                        for f in _nombres_de(p["categoria"], nombres)[:2] + (p["categoria"],)}
        self._formas = sorted(formas.items(), key=lambda f: -len(f[0]))
        self._palabras = [set(normalizar("%s %s %s" % (p["diseno"], p["material"], p["origen"])).split())
                          for p in self.productos]
        cuenta = Counter(w for ws in self._palabras for w in ws)
        n = len(self.productos)
        self._idf = {w: 1 + (n / c) ** 0.5 / 10 for w, c in cuenta.items()}

    def _categorias(self, t):
        # Categorias nombradas en el texto normalizado, y el texto sin esas palabras
        encontradas = set()
        for forma, categorias in self._formas:
            if re.search(r"\b%s\b" % re.escape(forma), t):
                encontradas |= categorias
                if forma not in self._extra:
                    t = re.sub(r"\b%s\b" % re.escape(forma), " ", t)
        return encontradas, t

    def buscar(self, pregunta):
        # Devuelve (texto para el modelo, resumen para el log), o ("", "")
        categorias, resto = self._categorias(normalizar(pregunta))
        if not categorias and not _INTENCION & set(normalizar(pregunta).split()):
            return "", ""
        claves = [w for w in resto.split() if w not in _GENERICAS and len(w) > 2]
        candidatos = []
        for i, p in enumerate(self.productos):
            if categorias and p["categoria"] not in categorias:
                continue
            puntaje = 0.0
            for clave in claves:
                # igual o plural: vale lo raro que es la palabra (idf, 1 o mas); parecida: 0.5
                mejor = max(((self._idf[w] if _parecida(clave, w) == 1 else 0.5)
                             for w in self._palabras[i] if _parecida(clave, w)), default=0)
                puntaje += mejor
            if puntaje > 0 or (categorias and not claves):
                candidatos.append((puntaje, i))
        if not candidatos:
            if categorias and claves:       # nombro una categoria y un diseno que no hay
                return self._resumen(categorias, "No hay ese diseño exacto. "), \
                    "%s, sin '%s'" % ("/".join(sorted(categorias)), " ".join(claves))
            return "", ""
        if categorias and not any(p for p, _ in candidatos):
            return self._resumen(categorias), "%s (resumen)" % "/".join(sorted(categorias))
        candidatos.sort(key=lambda c: (-c[0], self.productos[c[1]]["precio"]))
        elegidos = [self.productos[i] for _, i in candidatos[:self.maximo]]
        lineas = ["- %s%s%s: %s" % (
            ("%s %s" % (p["singular"], p["diseno"])).strip(), ", %s" % _material(p["material"]) if p["material"] else "",
            ", hecho en %s" % p["origen"] if p["origen"] else "", _pesos(p["precio"]))
            for p in elegidos]
        mas = len(candidatos) - len(elegidos)
        if mas > 0:
            lineas.append("(y %d productos más que también coinciden)" % mas)
        return ("Productos de la tienda que coinciden, con su precio real:\n" + "\n".join(lineas),
                "%d coinciden: %s" % (len(candidatos), ", ".join(
                    ("%s %s %d" % (p["singular"], p["diseno"], p["precio"])).strip() for p in elegidos[:3])))

    def _resumen(self, categorias, antes=""):
        partes = []
        for categoria in sorted(categorias):
            lista = [p for p in self.productos if p["categoria"] == categoria]
            disenos = [d for d, _ in Counter(p["diseno"] for p in lista if p["diseno"]).most_common(5)]
            partes.append("Hay %d diseños de %s, %s. Por ejemplo: %s." % (
                len(lista), lista[0]["plural"], _rango([p["precio"] for p in lista]), _lista(disenos)))
        return "Productos de la tienda (precios reales): " + antes + " ".join(partes)


if __name__ == "__main__":
    import conocimiento
    carpeta = conocimiento.carpeta_conocimiento()
    if len(sys.argv) > 2 and sys.argv[1] == "--publico":
        print(exportar_publico(sys.argv[2], carpeta))
    elif len(sys.argv) > 1:
        print(importar(sys.argv[1], carpeta))
    else:
        print(preparar(carpeta) or "productos.json ya esta al dia (o no hay ninguna base)")
    print("Escrito en", carpeta)
