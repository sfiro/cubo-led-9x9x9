# Revisa cada parte del asistente por separado e imprime un informe.
# No necesita que nadie hable: Whisper se prueba con la voz que genera Piper.
# Al final el parlante dice la respuesta de Ollama y el cubo la sigue.
#
#   python diagnostico.py                 todo
#   python diagnostico.py --sin-sonido    sin usar el parlante ni el microfono

import argparse
import sys
import time

import numpy as np

try:
    import config
except ImportError:
    import config_ejemplo as config

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
    c.apagar()
    return c


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

probar("Paquetes de Python", paquetes)
c = probar("Cubo por USB (Pico)", cubo)
c = None if c is True else c
probar("Dispositivos de audio", dispositivos)
if not args.sin_sonido:
    probar("Microfono", microfono)
piper_ok = probar("Texto a voz (Piper)", voz_piper)
if piper_ok:
    probar("Voz a texto (Whisper)", whisper, *piper_ok)
respuesta = probar("Modelo de lenguaje (Ollama)", ollama)
if piper_ok and not args.sin_sonido:
    probar("Hablar con el cubo", hablar, c, piper_ok[0],
           respuesta if isinstance(respuesta, str) else FRASE)
if c is not None:
    c.cerrar()

print("\n== Resumen ==")
for nombre, ok, detalle in resultados:
    print("  %-30s %s  %s" % (nombre, "OK   " if ok else "FALLA", detalle))
sys.exit(0 if all(ok for _, ok, _ in resultados) else 1)
