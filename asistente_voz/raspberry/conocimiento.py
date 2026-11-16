# Lo que sabe el cubo (v5): busca en los documentos de ../conocimiento/ la
# informacion que sirve para cada pregunta (RAG) y responde al instante las
# preguntas frecuentes.
#
#   - Cada seccion "## ..." de cada .md es un fragmento.
#   - Al arrancar, Ollama calcula un vector por fragmento con un modelo de
#     embeddings (bge-m3, que entiende espanol). Se guardan en un archivo para
#     no recalcular lo que no cambio.
#   - Por cada pregunta se buscan los fragmentos mas parecidos (coseno entre
#     vectores, mas un poco de coincidencia de palabras) y solo esos van al modelo.
#   - Si el modelo de embeddings no esta, funciona igual buscando por palabras.

import glob
import hashlib
import json
import math
import os
import re
import urllib.request
from collections import Counter

import numpy as np

from comandos import normalizar

AQUI = os.path.dirname(os.path.abspath(__file__))
ESPECIALES = {"preguntas_frecuentes.md", "personalidad.md", "nombres_productos.md", "leeme.md", "readme.md"}
MAX_CARACTERES = 900        # una seccion mas larga se parte por parrafos
_CACHE = os.path.join(AQUI, ".cache_conocimiento.json")
_PALABRAS_VACIAS = set(
    "a al algo como con cual cuales cuando de del donde el ella en es esta este esto hay la "
    "las le les lo los me mi mas muy no o para pero por que se si sin son su sus te tu un una "
    "uno unos unas y ya yo hola cubo puedo puedes quiero sabes dime me tienen tiene hay".split())

REGLAS = (
    "\n\nPara datos de la tienda (horarios, precios, productos, dirección, pagos, envíos) y de "
    "Cali (lugares, eventos, comida, transporte) usa SOLO la información que viene con la "
    "pregunta. Si la respuesta no está ahí, di con naturalidad que no tienes ese dato y sugiere "
    "preguntar en caja. Nunca inventes precios, horarios, direcciones ni fechas. "
    "Para los productos usa solo los de la lista \"Productos de la tienda\", con su precio "
    "exacto; si son muchos, menciona dos o tres y el rango de precios. No sabes si quedan "
    "unidades: para eso, que pregunten en caja. No des precios al por mayor.")


def carpeta_conocimiento(ruta=None):
    # La carpeta que se usa: la de config, o ../conocimiento, o la plantilla
    for candidata in ([ruta] if ruta else []) + ["../conocimiento", "../conocimiento_ejemplo"]:
        completa = os.path.normpath(os.path.join(AQUI, candidata))
        if os.path.isdir(completa):
            return completa
    return None


def _sin_comentarios(texto):
    return re.sub(r"<!--.*?-->", "", texto, flags=re.S)


class Fragmento:
    def __init__(self, archivo, titulo, texto):
        self.archivo = archivo
        self.titulo = titulo
        self.texto = texto

    def nombre(self):
        return "%s > %s" % (self.archivo, self.titulo) if self.titulo else self.archivo

    def para_modelo(self):
        return "(%s) %s" % (self.nombre(), self.texto)


def leer_fragmentos(carpeta):
    # Devuelve (fragmentos, parrafos sin completar)
    fragmentos, pendientes = [], 0
    rutas = sorted(glob.glob(os.path.join(carpeta, "*.md")) + glob.glob(os.path.join(carpeta, "*.txt")))
    for ruta in rutas:
        archivo = os.path.basename(ruta)
        if archivo.lower() in ESPECIALES:
            continue
        with open(ruta, encoding="utf-8") as f:
            texto = _sin_comentarios(f.read())
        principal, titulo, parrafos = "", "", []

        def cerrar():
            # Junta los parrafos de la seccion en fragmentos de hasta MAX_CARACTERES
            actual = ""
            for p in parrafos:
                if actual and len(actual) + len(p) > MAX_CARACTERES:
                    fragmentos.append(Fragmento(archivo, titulo or principal, actual))
                    actual = ""
                actual = (actual + "\n" + p).strip()
            if actual:
                fragmentos.append(Fragmento(archivo, titulo or principal, actual))
            parrafos.clear()

        for bloque in re.split(r"\n\s*\n", texto):
            bloque = bloque.strip()
            if not bloque:
                continue
            m = re.match(r"^(#+)\s*(.+)$", bloque.splitlines()[0])
            if m:
                cerrar()
                if len(m.group(1)) == 1:
                    principal = m.group(2).strip()
                else:
                    titulo = m.group(2).strip()
                bloque = "\n".join(bloque.splitlines()[1:]).strip()
                if not bloque:
                    continue
            if "[COMPLETAR" in bloque:
                pendientes += 1
                continue
            parrafos.append(" ".join(bloque.split()))
        cerrar()
    return fragmentos, pendientes


def leer_preguntas(carpeta):
    # preguntas_frecuentes.md -> lista de (formas de preguntar, respuesta)
    ruta = os.path.join(carpeta, "preguntas_frecuentes.md")
    if not os.path.exists(ruta):
        return []
    with open(ruta, encoding="utf-8") as f:
        texto = _sin_comentarios(f.read())
    preguntas = []
    for seccion in re.split(r"^##\s+", texto, flags=re.M)[1:]:
        lineas = [l.strip() for l in seccion.strip().splitlines() if l.strip()]
        formas = [lineas[0]] + [l[1:].strip() for l in lineas[1:] if l.startswith("-")]
        respuesta = " ".join(l for l in lineas[1:] if not l.startswith("-"))
        respuesta = re.sub(r"^Respuesta:\s*", "", respuesta).strip()
        if respuesta and "[COMPLETAR" not in respuesta:
            preguntas.append((formas, respuesta))
    return preguntas


def cargar_personalidad(carpeta):
    ruta = os.path.join(carpeta, "personalidad.md") if carpeta else ""
    if ruta and os.path.exists(ruta):
        with open(ruta, encoding="utf-8") as f:
            texto = _sin_comentarios(f.read()).strip()
        return texto or None
    return None


def _palabras(texto):
    return [p for p in normalizar(texto).split() if p not in _PALABRAS_VACIAS and len(p) > 1]


class _Palabras:
    # Busqueda por palabras (BM25), sin modelo
    def __init__(self, textos):
        self.docs = [Counter(_palabras(t)) for t in textos]
        self.largo = [sum(d.values()) for d in self.docs]
        self.promedio = (sum(self.largo) / len(self.largo)) if self.largo else 1
        n = len(self.docs)
        apariciones = Counter(p for d in self.docs for p in d)
        self.idf = {p: math.log(1 + (n - c + 0.5) / (c + 0.5)) for p, c in apariciones.items()}

    def puntajes(self, pregunta):
        consulta = _palabras(pregunta)
        res = np.zeros(len(self.docs))
        for i, d in enumerate(self.docs):
            for p in consulta:
                if p in d:
                    f = d[p]
                    res[i] += self.idf[p] * f * 2.2 / (f + 1.2 * (0.25 + 0.75 * self.largo[i] / self.promedio))
        return res


class Conocimiento:
    def __init__(self, carpeta, url="http://localhost:11434", modelo="bge-m3", fragmentos=3,
                 umbral=0.35, umbral_exacta=0.88):
        self.carpeta = carpeta
        self.url = url
        self.modelo = modelo
        self.k = fragmentos
        self.umbral = umbral
        self.umbral_exacta = umbral_exacta
        self.fragmentos, self.pendientes = leer_fragmentos(carpeta)
        self.preguntas = leer_preguntas(carpeta)
        self._formas = [(normalizar(f), i) for i, (formas, _) in enumerate(self.preguntas) for f in formas]
        textos = [f.nombre() + ". " + f.texto for f in self.fragmentos]
        self._bm25 = _Palabras(textos)
        self._vectores = self._vectores_formas = None
        self.aviso = None
        try:
            self._vectores = self._embeber(textos, "documento")
            self._vectores_formas = self._embeber([f for f, _ in self._formas], "pregunta")
        except Exception as e:
            self.aviso = ("sin el modelo de embeddings %s (%s): busco solo por palabras. "
                          "Instalalo con: ollama pull %s" % (modelo, e, modelo))
            self._vectores = self._vectores_formas = None

    # ---- vectores -----------------------------------------------------------

    def _prefijo(self, tipo):
        # nomic-embed-text pide indicar si es una pregunta o un documento
        if "nomic" in self.modelo:
            return "search_query: " if tipo == "pregunta" else "search_document: "
        return ""

    def _pedir(self, textos):
        pedido = urllib.request.Request(self.url + "/api/embed",
                                        data=json.dumps({"model": self.modelo, "input": textos}).encode(),
                                        headers={"Content-Type": "application/json"})
        with urllib.request.urlopen(pedido, timeout=300) as r:
            return json.loads(r.read())["embeddings"]

    def _embeber(self, textos, tipo):
        if not textos:
            return np.zeros((0, 1), np.float32)
        textos = [self._prefijo(tipo) + t for t in textos]
        try:
            with open(_CACHE, encoding="utf-8") as f:
                cache = json.load(f)
        except (OSError, ValueError):
            cache = {}
        guardados = cache.setdefault(self.modelo, {})
        claves = [hashlib.sha1(t.encode()).hexdigest() for t in textos]
        faltan = [i for i, c in enumerate(claves) if c not in guardados]
        for i in range(0, len(faltan), 16):
            grupo = faltan[i:i + 16]
            for j, v in zip(grupo, self._pedir([textos[j] for j in grupo])):
                guardados[claves[j]] = v
        if faltan:
            try:
                with open(_CACHE, "w", encoding="utf-8") as f:
                    json.dump(cache, f)
            except OSError:
                pass
        m = np.array([guardados[c] for c in claves], np.float32)
        return m / np.maximum(np.linalg.norm(m, axis=1, keepdims=True), 1e-9)

    def _vector(self, texto, tipo):
        v = np.array(self._pedir([self._prefijo(tipo) + texto])[0], np.float32)
        return v / max(float(np.linalg.norm(v)), 1e-9)

    # ---- consultas ----------------------------------------------------------

    def con_vectores(self):
        return self._vectores is not None

    def pregunta_exacta(self, pregunta):
        # (respuesta, forma parecida, puntaje) si se parece mucho a una pregunta frecuente
        if not self._formas:
            return None
        if self.con_vectores():
            puntajes = self._vectores_formas @ self._vector(pregunta, "pregunta")
            i = int(np.argmax(puntajes))
            puntaje, umbral = float(puntajes[i]), self.umbral_exacta
        else:
            consulta = set(_palabras(pregunta))
            def jaccard(f):
                forma = set(_palabras(f))
                return len(consulta & forma) / max(1, len(consulta | forma))
            similitudes = [jaccard(f) for f, _ in self._formas]
            i = int(np.argmax(similitudes))
            puntaje, umbral = similitudes[i], 0.8
        if puntaje >= umbral:
            forma, cual = self._formas[i]
            return self.preguntas[cual][1], forma, puntaje
        return None

    def buscar(self, pregunta):
        # Los fragmentos que sirven: lista de (puntaje, Fragmento), del mejor al peor
        if not self.fragmentos:
            return []
        palabras = self._bm25.puntajes(pregunta)
        if palabras.max() > 0:
            palabras = palabras / palabras.max()
        if self.con_vectores():
            puntajes = self._vectores @ self._vector(pregunta, "pregunta") + 0.15 * palabras
            umbral = self.umbral
        else:
            puntajes, umbral = palabras, 0.2
        orden = np.argsort(-puntajes)[:self.k]
        return [(float(puntajes[i]), self.fragmentos[i]) for i in orden if puntajes[i] >= umbral]

    def contexto(self, pregunta):
        # (texto para el modelo, nombres de las fuentes para el log)
        encontrados = self.buscar(pregunta)
        if not encontrados:
            return "", []
        texto = "\n".join(f.para_modelo() for _, f in encontrados)
        return texto, ["%s %.2f" % (f.nombre(), p) for p, f in encontrados]
