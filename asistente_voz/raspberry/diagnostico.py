# Revisa cada parte del asistente por separado e imprime un informe.
# No necesita que nadie hable: Whisper se prueba con la voz que genera Piper.
# Al final el parlante dice la respuesta de Ollama y el cubo la sigue.
#
#   python diagnostico.py                 todo
#   python diagnostico.py --sin-sonido    sin usar el parlante ni el microfono

import argparse
import os
import sys
import time

import numpy as np

try:
    import config
except ImportError:
    import config_ejemplo as config

from asistente import VERSION

FRASE = "Hola, soy el cubo de luz. Esta es una prueba del asistente de voz."
resultados = []


def probar(nombre, funcion, *args):
    print("\n== %s ==" % nombre)
    t = time.monotonic()
    try:
        valor = funcion(*args)
    except Exception as e:
        print("  ERROR: %s: %s" % (type(e).__name__, e))
        resultados.append((nombre, False, "%s: %s" % (type(e).__name__, e)))
        return None
    resultados.append((nombre, True, "%.1f s" % (time.monotonic() - t)))
    return valor if valor is not None else True


def paquetes():
    import faster_whisper
    import piper  # noqa: F401
    import serial
    import sounddevice
    print("  Python %s | numpy %s | sounddevice %s | pyserial %s | faster-whisper %s"
          % (sys.version.split()[0], np.__version__, sounddevice.__version__,
             serial.__version__, faster_whisper.__version__))


def cubo():
    from cubo_usb import CuboUSB
    c = CuboUSB(config.PUERTO_CUBO)
    print("  Pico en %s: enciendo piso por piso y luego todo" % c.puerto)
    for z in range(9):
        vox = np.zeros((9, 9, 9), bool)
        vox[z] = True
        c.mostrar(vox)
        time.sleep(0.15)
    c.mostrar(np.ones((9, 9, 9), bool))
    time.sleep(0.5)
    if c.bits > 1:
        # v3: cada piso con un brillo distinto, del 1 (abajo) al 15 (arriba)
        print("  La Pico tiene %d niveles de brillo: ahora cada piso brilla distinto, "
              "mas tenue abajo y mas fuerte arriba" % (1 << c.bits))
        niveles = np.zeros((9, 9, 9), np.uint8)
        for z in range(9):
            niveles[z] = round(1 + 14 * z / 8)
        c.mostrar(niveles)
        time.sleep(2)
    else:
        print("  AVISO: la Pico no tiene niveles de brillo (firmware de la v1 o v2). "
              "Cargale el nuevo con ./cargar_pico.sh")
    c.apagar()
    return c


def animaciones(c):
    # v2: las 34 animaciones de la Pico corriendo en la Pi
    from animaciones_pi import NOMBRES, MotorAnimaciones
    motor = MotorAnimaciones()
    sin_luz = []
    for nombre in NOMBRES:
        motor.iniciar(nombre)
        maximo = 0
        inicio = time.monotonic()
        minimo = 2.0 if nombre == "lluvia" else 0.3
        # algunas (disparos) tardan un poco en encender el primer LED: hasta 2 s
        while time.monotonic() - inicio < (minimo if maximo else max(minimo, 2.0)):
            cuadro = motor.cuadro()
            maximo = max(maximo, int(cuadro.sum()))
            if c is not None:
                c.mostrar(cuadro)
            time.sleep(0.025)
        if maximo == 0:
            sin_luz.append(nombre)
    motor.detener()
    if c is not None:
        c.apagar()
    print("  %d animaciones probadas (en el cubo se ve cada una un momento, la lluvia 2 s)"
          % len(NOMBRES))
    if sin_luz:
        raise RuntimeError("no encendieron ningun LED: %s" % ", ".join(sin_luz))


def etiquetas():
    # v2: el modelo tiene que saber pedir animaciones con [ANIMACION: nombre]
    import comandos
    from asistente import preguntar
    from voz import Frases
    frases = Frases()
    respuesta = ""
    pedido = [{"role": "user", "content": "Pon en el cubo algo de Navidad, por favor."}]
    for pedazo in preguntar(pedido, comandos.instrucciones_cubo()):
        respuesta += pedazo
        frases.agregar(pedazo)
    frases.terminar()
    acciones = [comandos.de_etiqueta(t, v) for t, v in frases.etiquetas]
    print("  Pregunta: %s\n  Respuesta: %s\n  Acciones: %s"
          % (pedido[0]["content"], respuesta.strip(), [a for a in acciones if a] or "ninguna"))
    if not any(acciones):
        print("  AVISO: el modelo no pidio ninguna animacion. Los comandos directos (\"pon el "
              "arbol\") funcionan igual; para pedidos libres conviene un modelo mas grande.")


def dispositivos():
    import sounddevice as sd
    print(sd.query_devices())
    if len(sd.query_devices()) == 0:
        raise RuntimeError("PortAudio no ve ningun dispositivo. Si la Pi tiene escritorio "
                           "(PipeWire), instala pipewire-alsa y revisa XDG_RUNTIME_DIR "
                           "(ver 'Audio con PipeWire' en el README)")
    entrada = sd.query_devices(config.MICROFONO, "input")
    salida = sd.query_devices(config.PARLANTE, "output")
    print("  Microfono elegido: %s (%d Hz)" % (entrada["name"], entrada["default_samplerate"]))
    print("  Parlante elegido:  %s (%d Hz)" % (salida["name"], salida["default_samplerate"]))


def microfono():
    from audio import Microfono
    m = Microfono(config.MICROFONO)
    valores = []
    for _ in range(30):
        time.sleep(0.1)
        valores.append(m.rms)
    m.cerrar()
    print("  Ruido del ambiente en 3 s: promedio %.4f, maximo %.4f (UMBRAL_VOZ = %.3f)"
          % (np.mean(valores), max(valores), config.UMBRAL_VOZ))
    if max(valores) == 0:
        raise RuntimeError("el microfono solo entrega ceros: esta silenciado o no es el "
                           "dispositivo correcto (revisa MICROFONO en config.py)")
    if np.mean(valores) > config.UMBRAL_VOZ:
        print("  AVISO: el ruido supera UMBRAL_VOZ; el asistente creeria que siempre "
              "le hablan. Sube UMBRAL_VOZ en config.py")


def voz_piper():
    from voz import Voz
    v = Voz(config.VOZ_PIPER)
    t = time.monotonic()
    audio = v.sintetizar(FRASE)
    print("  %.1f s de audio a %d Hz, generado en %.2f s"
          % (len(audio) / v.sr, v.sr, time.monotonic() - t))
    return v, audio


def whisper(voz, audio):
    from faster_whisper import WhisperModel

    from audio import remuestrear
    modelo = WhisperModel(config.WHISPER_MODELO, device="cpu", compute_type="int8")
    t = time.monotonic()
    segmentos, _ = modelo.transcribe(remuestrear(audio, voz.sr, 16000),
                                     language=config.IDIOMA, beam_size=1)
    texto = " ".join(s.text.strip() for s in segmentos).strip()
    print("  Dije:     %s\n  Entendio: %s  (%.1f s)" % (FRASE, texto, time.monotonic() - t))
    if not texto:
        raise RuntimeError("no transcribio nada")
    return modelo


def activacion(voz, modelo):
    # v4: Whisper tiene que reconocer la frase de activacion, y no confundirla
    from activacion import buscar_frase

    from audio import remuestrear
    frase = getattr(config, "FRASE_ACTIVACION", "hola cubo")
    pruebas = [("%s, pon la lluvia." % frase.capitalize(), True),
               ("%s." % frase.capitalize(), True),
               ("Hola, ¿cómo estás?", False)]
    errores = []
    for dicho, esperado in pruebas:
        audio = remuestrear(voz.sintetizar(dicho), voz.sr, 16000)
        segmentos, _ = modelo.transcribe(audio, language=config.IDIOMA, beam_size=1,
                                         vad_filter=True, initial_prompt=frase.title() + ".")
        texto = " ".join(s.text.strip() for s in segmentos).strip()
        encontrada, resto = buscar_frase(texto, frase)
        print("  Dije: %-28s Entendio: %-30s -> %s%s" % (
            dicho, texto, "despierta" if encontrada else "sigue dormido",
            " y hace: %s" % resto if resto else ""))
        if encontrada != esperado:
            errores.append(dicho)
    if config.ACTIVACION != "frase":
        print('  AVISO: en config.py ACTIVACION = "%s"; para usar "%s" pon ACTIVACION = "frase"'
              % (config.ACTIVACION, frase))
    if errores:
        raise RuntimeError("se equivoco con: %s" % "; ".join(errores))


def saber():
    # v5: los documentos de ../conocimiento/ y la busqueda en ellos
    import conocimiento
    carpeta = conocimiento.carpeta_conocimiento(getattr(config, "CARPETA_CONOCIMIENTO", None))
    if carpeta is None:
        raise RuntimeError("no esta la carpeta ../conocimiento ni ../conocimiento_ejemplo")
    s = conocimiento.Conocimiento(carpeta, config.OLLAMA_URL,
                                  getattr(config, "MODELO_EMBEDDINGS", "bge-m3"),
                                  getattr(config, "FRAGMENTOS", 2),
                                  getattr(config, "UMBRAL_FRAGMENTO", 0.35),
                                  getattr(config, "UMBRAL_PREGUNTA_EXACTA", 0.88))
    print("  Carpeta: %s\n  %d secciones, %d preguntas frecuentes, %d parrafos sin completar"
          % (carpeta, len(s.fragmentos), len(s.preguntas), s.pendientes))
    if os.path.basename(carpeta) == "conocimiento_ejemplo":
        print("  AVISO: se esta usando la plantilla. Corre ./instalar.sh para crear ../conocimiento "
              "y llena negocio.md con los datos de la tienda")
    if s.aviso:
        print("  AVISO: " + s.aviso)
    for pregunta in ["¿A qué hora abren?", "¿Cómo te llamas?", "¿Qué es el champús?",
                     "¿Dónde puedo bailar salsa?", "¿Qué paseos puedo hacer cerca de Cali?",
                     "¿Cuánto cuesta una mochila wayúu?"]:
        exacta = s.pregunta_exacta(pregunta)
        if exacta:
            print("  %-42s -> pregunta frecuente '%s' (%.2f)" % (pregunta, exacta[1], exacta[2]))
        else:
            print("  %-42s -> %s" % (pregunta, ", ".join(
                "%s %.2f" % (f.nombre(), p) for p, f in s.buscar(pregunta)) or "nada"))
    if s.aviso:
        raise RuntimeError("sin el modelo de embeddings (la busqueda por palabras funciona)")


def base_productos():
    # v5.1: la base de productos importada y la busqueda de precios
    import conocimiento
    import productos
    carpeta = conocimiento.carpeta_conocimiento(getattr(config, "CARPETA_CONOCIMIENTO", None))
    mensaje = productos.preparar(carpeta)
    if mensaje:
        print("  " + mensaje)
    if not os.path.exists(os.path.join(carpeta, "productos.json")):
        print("  No hay base de productos: copia el Excel como %s/productos.xlsx" % carpeta)
        return
    catalogo = productos.Catalogo(carpeta)
    print("  %d productos en la base" % len(catalogo.productos))
    for pregunta in ["¿Cuánto vale un imán?", "¿Tienen llaveros del gato de Tejada?",
                     "¿Qué tienen de la Ermita?", "¿Tienen camisetas?"]:
        _, resumen = catalogo.buscar(pregunta)
        print("  %-38s -> %s" % (pregunta, resumen or "nada"))


def ollama():
    from asistente import preguntar
    t = time.monotonic()
    primera = None
    respuesta = ""
    for pedazo in preguntar([{"role": "user", "content": "Saluda en una sola frase corta."}]):
        primera = primera or time.monotonic()
        respuesta += pedazo
    print("  %s: %s\n  (primera palabra en %.1f s, total %.1f s)"
          % (config.MODELO, respuesta.strip(), (primera or t) - t, time.monotonic() - t))
    if not respuesta.strip():
        raise RuntimeError("respuesta vacia")
    return respuesta.strip()


def hablar(c, voz, texto):
    from audio import Reproductor
    from cubo_usb import CuboConsola
    from visual import Animador
    from voz import limpiar
    rep = Reproductor(config.PARLANTE, config.AJUSTE_SINCRONIA_S)
    anim = Animador(c or CuboConsola(), rep, modo=config.ANIMACION_HABLANDO,
                    fps=config.CUADROS_POR_SEGUNDO)
    anim.start()
    print("  Diciendo por el parlante: %s" % texto)
    rep.agregar(voz.sintetizar(limpiar(texto)), voz.sr)
    rep.esperar()
    time.sleep(0.5)
    anim.detener()
    rep.cerrar()


p = argparse.ArgumentParser(description="Diagnostico del asistente de voz")
p.add_argument("--sin-sonido", action="store_true", help="no usar el parlante ni el microfono")
args = p.parse_args()
print("Diagnostico del asistente de voz v%s" % VERSION)

probar("Paquetes de Python", paquetes)
c = probar("Cubo por USB (Pico)", cubo)
c = None if c is True else c
probar("Animaciones en la Pi (v2)", animaciones, c)
probar("Dispositivos de audio", dispositivos)
if not args.sin_sonido:
    probar("Microfono", microfono)
piper_ok = probar("Texto a voz (Piper)", voz_piper)
if piper_ok:
    modelo_whisper = probar("Voz a texto (Whisper)", whisper, *piper_ok)
    if modelo_whisper:
        probar("Frase de activacion (v4)", activacion, piper_ok[0], modelo_whisper)
probar("Base de productos (v5.1)", base_productos)
probar("Conocimiento (v5)", saber)
respuesta = probar("Modelo de lenguaje (Ollama)", ollama)
if respuesta:
    probar("El modelo elige animaciones (v2)", etiquetas)
if piper_ok and not args.sin_sonido:
    probar("Hablar con el cubo", hablar, c, piper_ok[0],
           respuesta if isinstance(respuesta, str) else FRASE)
if c is not None:
    c.cerrar()

print("\n== Resumen ==")
for nombre, ok, detalle in resultados:
    print("  %-30s %s  %s" % (nombre, "OK   " if ok else "FALLA", detalle))
sys.exit(0 if all(ok for _, ok, _ in resultados) else 1)
