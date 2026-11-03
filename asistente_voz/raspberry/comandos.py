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
             ["modo demo", "todas las animaciones", "todas", "sorprendeme"]),
}

_VERBOS = (r"pon|ponme|ponle|ponga|muestra|muestrame|mostrar|activa|cambia|enciende|prende|"
           r"haz|hazme|quiero ver|dejame ver|ensename|reproduce|inicia|arranca|lanza|"
           r"vuelve a|regresa a")
_PARAR = re.compile(
    r"(?:oye cubo )?(?:por favor )?(para|deten|detente|basta|quita|apaga|termina|stop)"
    r"(?: (?:ya|eso|esto|todo|la animacion|las animaciones|las luces|el cubo|la luz))*"
    r"(?: por favor)?")
_ESCRIBIR = re.compile(
    r".*?\b(?:escribe|escribeme|escribir|pon el texto|muestra el texto|pon el mensaje|"
    r"muestra el mensaje)\s+(?:la palabra |el texto |el mensaje |que diga |esto )?(.+)")
_HORA = re.compile(r"\b(que hora es|que horas son|dime la hora|me dices la hora|sabes la hora)\b")
_VERBO = re.compile(r"\b(%s)\b" % _VERBOS)
_ETIQUETA = re.compile(r"\[\s*(ANIMACI[OÓ]N|TEXTO|PARAR)\s*(?::\s*([^\]]*))?\]", re.I)

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


def buscar_animacion(texto):
    t = " %s " % normalizar(texto)
    for alias, nombre in _ALIAS:
        if " %s " % alias in t:
            return nombre
    return None


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
    # tipo: "animacion" (nombre, mensaje opcional) o "parar"
    def __init__(self, tipo, nombre=None, mensaje=None, respuesta=None):
        self.tipo = tipo
        self.nombre = nombre
        self.mensaje = mensaje
        self.respuesta = respuesta      # lo que dice el asistente, si es un comando directo

    def etiqueta(self):
        # Como la escribiria el modelo: se guarda en el historial para que aprenda el formato
        if self.tipo == "parar":
            return "[PARAR]"
        if self.mensaje:
            return "[TEXTO: %s]" % self.mensaje.strip()
        return "[ANIMACION: %s]" % self.nombre

    def __repr__(self):
        return "Accion(%s, %s, %r)" % (self.tipo, self.nombre, self.mensaje)


def interpretar(texto):
    # Comandos directos. Devuelve una Accion con su respuesta, o None si hay
    # que preguntarle al modelo.
    t = normalizar(texto)
    if _PARAR.fullmatch(t):
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
    sin_articulo = re.sub(r"^(?:el|la|los|las|un|una|modo) ", "", t)
    if _VERBO.search(t) or sin_articulo in dict(_ALIAS):     # "pon la lluvia" o solo "lluvia"
        nombre = buscar_animacion(t)
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


def instrucciones_cubo(actual=None, mensaje=None):
    # Lo que se agrega a las instrucciones del modelo en cada pregunta
    lista = "\n".join("- %s: %s" % (n, d) for n, (_, d, _) in CATALOGO.items())
    if actual is None:
        estado = "Ahora no hay ninguna animación."
    elif mensaje:
        estado = "Ahora el cubo muestra el texto %s." % mensaje.strip()
    else:
        estado = "Ahora el cubo muestra la animación %s." % actual
    ahora = time.localtime()
    dias = ["lunes", "martes", "miércoles", "jueves", "viernes", "sábado", "domingo"]
    meses = ["enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto",
             "septiembre", "octubre", "noviembre", "diciembre"]
    fecha = "Hoy es %s %d de %s de %d, y son las %02d:%02d." % (
        dias[ahora.tm_wday], ahora.tm_mday, meses[ahora.tm_mon - 1], ahora.tm_year,
        ahora.tm_hour, ahora.tm_min)
    return (
        "\n\nAdemás de conversar, puedes cambiar lo que muestra el cubo. Solo si la persona "
        "lo pide, o si pide algo que encaje con una animación (por ejemplo, algo relajante "
        "o algo de Navidad), responde con una frase corta y escribe al final UNA etiqueta:\n"
        "[ANIMACION: nombre] para poner una animación de la lista,\n"
        "[TEXTO: palabras] para escribir un texto corto en el cubo,\n"
        "[PARAR] para apagar la animación.\n"
        "Si solo están conversando, no escribas ninguna etiqueta. Nunca leas ni expliques "
        "las etiquetas. Animaciones disponibles:\n%s\n%s %s" % (lista, estado, fecha))
