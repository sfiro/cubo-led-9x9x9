# Control de las animaciones con la voz (v2).
#
# Dos caminos:
#   1. Comandos directos ("pon la lluvia", "para", "escribe HOLA", "que hora es"):
#      se reconocen aqui con reglas simples, sin pasar por el modelo. Son
#      instantaneos y no fallan.
#   2. Todo lo demas va al modelo. En las instrucciones se le da la lista de
#      animaciones, y si la conversacion lo pide agrega una etiqueta al final
#      de su respuesta: [ANIMACION: nombre], [TEXTO: palabras] o [PARAR].
#      voz.Frases quita las etiquetas antes de hablar y las junta.

import random
import re
import time
import unicodedata

# nombre: (como se dice, descripcion para el modelo, otras formas de pedirla)
CATALOGO = {
    "marquesina": ("la marquesina", "texto que corre por las 4 caras", ["letrero"]),
    "cubo_giratorio": ("el cubo giratorio", "cubo de alambre que gira",
                       ["cubo que gira", "cubo girando"]),
    "cajas": ("las cajas", "cajas de alambre que crecen", ["caja"]),
    "lluvia": ("la lluvia", "gotas de lluvia que caen", ["llueva", "llover", "lloviendo"]),
    "rayo": ("la tormenta", "nubes y rayos", ["rayos", "tormenta", "relampago", "relampagos"]),
    "fuegos": ("los fuegos artificiales", "cohetes que explotan",
               ["fuegos artificiales", "pirotecnia", "cohetes"]),
    "carita": ("la carita", "carita feliz, corazon e invasor del espacio",
               ["cara", "corazon", "invasor"]),
    "helice": ("la hélice", "doble helice de ADN girando", ["adn"]),
    "matrix": ("Matrix", "lluvia de codigo como en la pelicula", ["codigo"]),
    "planos": ("los planos", "planos que se mueven", []),
    "envios": ("los envíos", "LEDs que viajan de un lado al otro", []),
    "rebotes": ("las pelotas", "pelotas que rebotan", ["pelotas", "pelota", "rebote"]),
    "pacman": ("Pac-Man", "Pac-Man comiendo puntos", ["pac man", "comecocos"]),
    "disparos": ("los disparos", "proyectiles que chocan", ["balas", "disparo"]),
    "cinta": ("la cinta", "cinta retorcida", []),
    "olas": ("las olas", "olas como en el agua, relajante", ["mar", "oleaje"]),
    "espiral": ("la espiral", "gusano que baja en espiral", []),
    "tornado": ("el tornado", "particulas girando en remolino", ["remolino", "huracan"]),
    "esfera_movil": ("la burbuja", "esfera que viaja por el cubo",
                     ["esfera movil", "burbuja", "burbujas"]),
    "fuente": ("la fuente de agua", "chorro de agua", ["fuente de agua", "chorro"]),
    "nieve": ("la nieve", "copos de nieve que se acumulan", ["nevando", "nieve", "copos", "nieva"]),
    "vida": ("el juego de la vida", "automata celular en 3D", ["juego de la vida"]),
    "espectro": ("el ecualizador", "barras de ecualizador", ["ecualizador"]),
    "arbol": ("el árbol de navidad", "arbol de navidad con luces",
              ["arbol de navidad", "navidad", "arbolito"]),
    "reloj": ("el reloj", "la hora dando vueltas", ["la hora"]),
    "bordes": ("los bordes", "aristas del cubo", ["aristas"]),
    "flash": ("el destello", "todo el cubo parpadea", ["destello", "parpadeo", "parpadear"]),
    "llenar_vaciar": ("llenar y vaciar", "el cubo se llena y se vacia", ["llenar y vaciar"]),
    "onda": ("la onda", "onda que recorre el cubo", []),
    "aleatorio": ("las luces al azar", "LEDs al azar", ["al azar", "luces locas"]),
    "serpiente": ("la serpiente", "serpiente que recorre el cubo", ["culebra", "snake"]),
    "esfera": ("la esfera", "esfera que crece", []),
    "ascensores": ("los ascensores", "columnas que suben y bajan", ["ascensor"]),
    "demo": ("todas las animaciones", "recorre todas, una tras otra",
             ["modo demo", "todas las animaciones", "las animaciones", "todas"]),
}

_VERBOS = (r"pon|ponme|ponle|ponla|ponlo|ponga|pongas|pones|poner|ponerme|ponerle|pondrias|"
           r"coloca|colocame|colocale|colocar|coloque|coloques|colocas|"
           r"muestra|muestrame|mostrar|mostrarme|muestres|ensename|ensenar|"
           r"activa|activame|activar|cambia|cambiala|cambiar|enciende|prende|haz|hazme|"
           r"reproduce|inicia|empieza|arranca|lanza|quiero ver|dejame ver|"
           r"vuelve a|regresa a|pasa a|sigue con|sorprendeme")
# v5.11: "quiero la lluvia", "dame la nieve": solo si lo que sigue es una animacion, para que
# "quiero un llavero de corazon" no ponga la carita
_QUIERO = re.compile(r"(?:yo )?(?:quiero|quisiera|queremos|dame|regalame)(?: ver)? "
                     r"(?:(?:el|la|los|las|un|una) )?(.+?)(?: por favor)?")
_PARAR = re.compile(
    r"(?:oye cubo )?(?:por favor )?(para|deten|detente|basta|quita|apaga|termina|stop)"
    r"(?: (?:ya|eso|esto|todo|la animacion|las animaciones|las luces|el cubo|la luz))*"
    r"(?: por favor)?")
_ESCRIBIR = re.compile(
    r".*?\b(?:escribe|escribeme|escribir|pon el texto|muestra el texto|pon el mensaje|"
    r"muestra el mensaje)\s+(?:la palabra |el texto |el mensaje |que diga |esto )?(.+)")
_HORA = re.compile(r"\b(que hora es|que horas son|dime la hora|me dices la hora|sabes la hora)\b")
_VERBO = re.compile(r"\b(%s)\b" % _VERBOS)
_BRILLO = re.compile(r"\b(brillo|luz|luces|intensidad)\b")
_BRILLO_MAX = re.compile(r"\b(maximo|maxima|al tope|todo el brillo|a tope)\b")
_BRILLO_MIN = re.compile(r"\b(minimo|minima)\b")
_BRILLO_BAJA = re.compile(r"\b(baja|bajale|bajar|bajalo|menos|reduce|disminuye|mas oscuro|mas tenue)\b")
_BRILLO_SUBE = re.compile(r"\b(sube|subele|subir|subelo|mas|aumenta)\b")
_NUMERO = re.compile(r"\b(\d{1,3})\b")
BRILLO_MINIMO = 0.25        # menos que esto y los LEDs quedan en el nivel 0 (apagados)

# Formas de pedirla -> nombre, de la mas larga a la mas corta: "esfera movil"
# tiene que ganarle a "esfera", y "fuegos artificiales" a "fuegos".
_ALIAS = sorted(((alias, nombre) for nombre, (_, _, otros) in CATALOGO.items()
                 for alias in otros + [nombre.replace("_", " ")]),
                key=lambda par: -len(par[0]))


def normalizar(texto):
    # minusculas, sin tildes ni signos: "¡Pon la LLUVIA!" -> "pon la lluvia"
    texto = unicodedata.normalize("NFD", texto.lower())
    texto = "".join(ch for ch in texto if unicodedata.category(ch) != "Mn")
    return " ".join(re.sub(r"[^a-z0-9: ]", " ", texto).split())


def _como_suena(palabra):
    # v5.11: Whisper escribe "yuvia", "matris", "nieb"... Se comparan las palabras como
    # suenan en espanol: ll=y, v=b, z/ce/ci=s, qu/ca/co/cu=k, sin h.
    p = palabra.replace("ll", "y").replace("v", "b").replace("z", "s").replace("h", "")
    p = re.sub(r"c(?=[ei])", "s", p)
    p = re.sub(r"qu(?=[ei])", "k", p)
    p = p.replace("c", "k").replace("x", "ks")
    return re.sub(r"(.)\1+", r"\1", p)            # letras repetidas: "fuegoss" -> "fuegos"


# Para la busqueda aproximada: alias de una sola palabra, largos (no "mar" ni "cara")
_PALABRAS_ANIMACION = [(_como_suena(alias), nombre) for alias, nombre in _ALIAS
                       if " " not in alias and len(alias) >= 4]


# v5.12: "detener animaciones", "apaga la lluvia", "ya no mas", "quita el demo", "cancela eso".
# Cuenta solo si TODAS las palabras son de parar: asi "¿cuando termina la feria?" o "¿para
# que sirve?" no detienen nada.
_VERBOS_PARAR = {"para", "pare", "parar", "paralo", "parala", "paralas", "detener", "deten",
                 "detente", "detenga", "detenlo", "detenla", "detenlas", "basta", "apaga", "apagar",
                 "apagala", "apagalo", "apagalas", "apague", "termina", "terminar", "termine",
                 "quita", "quitar", "quitala", "quitalo", "quitalas", "quite", "cancela", "cancelar",
                 "stop", "alto", "frena", "suficiente", "acaba", "acabar", "no mas", "ya no mas"}
_RELLENO_PARAR = {"oye", "cubo", "ya", "por", "favor", "puedes", "podrias", "me", "le", "la", "las",
                  "el", "los", "lo", "de", "un", "una", "eso", "esto", "esa", "esta", "todo", "todas",
                  "ahora", "mejor", "y", "no", "mas", "animacion", "animaciones", "demo", "modo",
                  "luces", "luz", "show", "ok", "listo", "gracias", "bueno"}


def es_parar(t):
    palabras = t.split()
    if not palabras or len(palabras) > 8:
        return False
    texto = " %s " % t
    if not any(" %s " % v in texto for v in _VERBOS_PARAR):
        return False
    nombres = {p for alias, _ in _ALIAS for p in alias.split()}
    return all(p in _VERBOS_PARAR or p in _RELLENO_PARAR or p in nombres for p in palabras)


def buscar_animacion(texto, aproximada=False):
    t = " %s " % normalizar(texto)
    for alias, nombre in _ALIAS:
        if " %s " % alias in t:
            return nombre
    if aproximada:          # solo cuando ya se sabe que piden algo ("pon la yuvia")
        import difflib
        mejor, cual = 0.0, None
        for palabra in t.split():
            if len(palabra) < 4:
                continue
            sonido = _como_suena(palabra)
            for forma, nombre in _PALABRAS_ANIMACION:
                parecido = difflib.SequenceMatcher(None, sonido, forma).ratio()
                if parecido > mejor:
                    mejor, cual = parecido, nombre
        if mejor >= 0.8:
            return cual
    return None


# v5.11: "pon una animacion", "pon otra", "cambia la animacion", "sorprendeme con algo"
_CUALQUIERA = re.compile(r"\b(animacion|animaciones|sorprendeme|cambiala)\b")
_OTRA = re.compile(r"(?:pon|ponme|coloca|colocame|cambia|muestrame)(?: (?:otra|otro|algo|alguna|"
                   r"cualquiera|lo que quieras))+(?: cosa)?(?: por favor)?")
_AL_AZAR = [n for n in CATALOGO if n not in ("demo", "marquesina", "reloj", "flash")]

# v5.11: se agrega a la pista de Whisper, para que reconozca los nombres de las animaciones
PISTA_ANIMACIONES = ("Pon la lluvia, coloca la nieve, los fuegos artificiales, Matrix, Pac-Man, "
                     "la hélice, el tornado, las olas, la serpiente, la carita. Para la animación.")


def nombre_hablado(nombre):
    # Como se dice una animacion: "lluvia" -> "la lluvia"
    if nombre in CATALOGO:
        return CATALOGO[nombre][0]
    return {"texto": "las letras"}.get(nombre, nombre.replace("_", " "))


def hora_hablada(t=None):
    t = time.localtime(t)
    h, m = t.tm_hour % 12 or 12, t.tm_min
    inicio = "Es la una" if h == 1 else "Son las %d" % h
    if m == 0:
        return inicio + " en punto"
    return "%s y %d" % (inicio, m)


def texto_para_cubo(texto):
    # La fuente del cubo solo tiene ASCII: sin tildes y en mayusculas
    texto = unicodedata.normalize("NFD", texto.upper())
    texto = "".join(ch for ch in texto if unicodedata.category(ch) != "Mn")
    return " ".join(re.sub(r"[^ -~]", "", texto).split())[:40]


class Accion:
    # tipo: "animacion" (nombre, mensaje opcional), "parar" o "brillo" (v3).
    # El valor del brillo es "+", "-" o un porcentaje 0..100.
    def __init__(self, tipo, nombre=None, mensaje=None, respuesta=None, valor=None):
        self.tipo = tipo
        self.nombre = nombre
        self.mensaje = mensaje
        self.valor = valor
        self.respuesta = respuesta      # lo que dice el asistente, si es un comando directo

    def etiqueta(self):
        # Como la escribiria el modelo: se guarda en el historial para que aprenda el formato
        if self.tipo == "parar":
            return "[PARAR]"
        if self.tipo == "brillo":
            return "[BRILLO: %s]" % self.valor
        if self.mensaje:
            return "[TEXTO: %s]" % self.mensaje.strip()
        return "[ANIMACION: %s]" % self.nombre

    def __repr__(self):
        if self.tipo == "brillo":
            return "Accion(brillo, %s)" % self.valor
        return "Accion(%s, %s, %r)" % (self.tipo, self.nombre, self.mensaje)


def nuevo_brillo(actual, valor):
    # Brillo 0..1 que queda despues de un pedido ("+", "-" o un porcentaje)
    if valor == "+":
        return min(1.0, actual / 0.7)
    if valor == "-":
        return max(BRILLO_MINIMO, actual * 0.7)
    return BRILLO_MINIMO + (1 - BRILLO_MINIMO) * max(0, min(100, int(valor))) / 100


def porcentaje(brillo):
    return int(round((brillo - BRILLO_MINIMO) / (1 - BRILLO_MINIMO) * 100))


def _pedido_brillo(t):
    # "baja el brillo", "brillo al maximo", "pon la luz al 40" -> Accion, o None
    if not _BRILLO.search(t):
        return None
    numero = _NUMERO.search(t)
    if _BRILLO_MAX.search(t):
        return Accion("brillo", valor=100, respuesta="Brillo al máximo.")
    if _BRILLO_MIN.search(t):
        return Accion("brillo", valor=0, respuesta="Brillo al mínimo.")
    if numero:
        valor = min(100, int(numero.group(1)))
        return Accion("brillo", valor=valor, respuesta="Pongo el brillo al %d por ciento." % valor)
    if _BRILLO_BAJA.search(t):
        return Accion("brillo", valor="-", respuesta=random.choice(["Bajo el brillo.", "Listo, más tenue."]))
    if _BRILLO_SUBE.search(t):
        return Accion("brillo", valor="+", respuesta=random.choice(["Subo el brillo.", "Listo, más brillo."]))
    return None


def interpretar(texto):
    # Comandos directos. Devuelve una Accion con su respuesta, o None si hay
    # que preguntarle al modelo.
    t = normalizar(texto)
    if _PARAR.fullmatch(t) or es_parar(t):
        return Accion("parar", respuesta=random.choice(
            ["Listo, apago la animación.", "Hecho, la detengo.", "Ok, la quito."]))
    m = _ESCRIBIR.match(t)
    if m:
        mensaje = texto_para_cubo(m.group(1))
        if mensaje:
            return Accion("animacion", "marquesina", mensaje + "   ",
                          random.choice(["Listo, lo escribo en el cubo.", "Va, lo escribo."]))
    if _HORA.search(t):
        return Accion("animacion", "reloj", respuesta=hora_hablada() + ".")
    brillo = _pedido_brillo(t)
    if brillo is not None:
        return brillo
    # Whisper a veces escribe "por la lluvia" en vez de "pon la lluvia" (v5.5)
    t = re.sub(r"^por (?=(?:el|la|los|las) )", "pon ", t)
    sin_articulo = re.sub(r"^(?:el|la|los|las|un|una|modo) ", "", t)
    pide = _VERBO.search(t)
    quiero = _QUIERO.fullmatch(t)
    nombre = None
    if pide or sin_articulo in dict(_ALIAS):     # "pon la lluvia" o solo "lluvia"
        nombre = buscar_animacion(t, aproximada=bool(pide))
        if nombre is None and pide and (_CUALQUIERA.search(t) or _OTRA.fullmatch(t)):
            nombre = random.choice(_AL_AZAR)        # "pon una animacion", "pon otra"
    elif quiero:                                 # "quiero la lluvia", "dame la yuvia"
        resto = quiero.group(1)
        nombre = dict(_ALIAS).get(resto) or (buscar_animacion(resto, aproximada=True)
                                             if " " not in resto else None)
    if nombre:
        dicho = CATALOGO[nombre][0]
        if nombre == "reloj":
            return Accion("animacion", "reloj", respuesta="Te muestro la hora. %s." % hora_hablada())
        return Accion("animacion", nombre, respuesta=random.choice(
            ["Listo, pongo %s.", "Claro, pongo %s.", "Va: %s."]) % dicho)
    return None


def de_etiqueta(tipo, valor):
    # Convierte una etiqueta del modelo en Accion (None si no es valida)
    tipo = normalizar(tipo)
    valor = (valor or "").strip()
    if tipo == "parar":
        return Accion("parar")
    if tipo == "brillo":
        if valor in ("+", "-"):
            return Accion("brillo", valor=valor)
        v = normalizar(valor)
        if v.isdigit():
            return Accion("brillo", valor=min(100, int(v)))
        return _pedido_brillo("brillo " + v)        # "maximo", "bajar"...
    if tipo == "texto" and texto_para_cubo(valor):
        return Accion("animacion", "marquesina", texto_para_cubo(valor) + "   ")
    if tipo == "animacion":
        nombre = normalizar(valor).replace(" ", "_")
        if nombre in CATALOGO:
            return Accion("animacion", nombre)
        nombre = buscar_animacion(valor)       # el modelo escribio "fuegos artificiales"
        if nombre:
            return Accion("animacion", nombre)
    return None


def instrucciones_cubo():
    # Lo que se agrega a las instrucciones del modelo. v5.4: es SIEMPRE el mismo
    # texto, y corto (solo los nombres de las animaciones). Asi Ollama reusa lo que
    # ya leyo de las instrucciones y de la conversacion, en vez de releerlo todo en
    # cada pregunta. Lo que cambia (la hora, la animacion actual) va en estado_cubo().
    nombres = ", ".join(n for n in CATALOGO if n != "demo")
    return (
        "\n\nPuedes cambiar lo que muestra el cubo, solo si te lo piden o si encaja con lo que "
        "piden (por ejemplo, algo relajante). Escribe al final de tu respuesta UNA etiqueta: "
        "[ANIMACION: nombre], [TEXTO: palabras], [PARAR] o [BRILLO: 0 a 100]. Si solo están "
        "conversando, no escribas etiquetas, y nunca las leas en voz alta. "
        "Animaciones: %s." % nombres)


def estado_cubo(actual=None, mensaje=None, brillo=None):
    # Una linea con la fecha, la hora y lo que muestra el cubo. Va junto a la
    # pregunta, no en las instrucciones (ver instrucciones_cubo).
    ahora = time.localtime()
    dias = ["lunes", "martes", "miércoles", "jueves", "viernes", "sábado", "domingo"]
    meses = ["enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto",
             "septiembre", "octubre", "noviembre", "diciembre"]
    texto = "Hoy es %s %d de %s, son las %d:%02d." % (
        dias[ahora.tm_wday], ahora.tm_mday, meses[ahora.tm_mon - 1], ahora.tm_hour, ahora.tm_min)
    if actual is None:
        texto += " El cubo no tiene animación."
    elif mensaje:
        texto += " El cubo muestra el texto %s." % mensaje.strip()
    else:
        texto += " El cubo muestra la animación %s." % actual
    if brillo is not None and brillo < 0.999:
        texto += " Brillo al %d por ciento." % porcentaje(brillo)
    return texto
