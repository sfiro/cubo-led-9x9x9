# Muestra el volumen del microfono para elegir UMBRAL_VOZ en config.py:
# quedate callado unos segundos, luego habla normal, y elige un valor entre
# los dos (por ejemplo, el doble del silencio).

import time

try:
    import config
except ImportError:
    import config_ejemplo as config
from audio import Microfono

mic = Microfono(config.MICROFONO)
print("Microfono a %d Hz. Umbral actual: %.3f (Ctrl-C para salir)" % (mic.sr, config.UMBRAL_VOZ))
maximo = 0.0
try:
    while True:
        time.sleep(0.1)
        maximo = max(maximo * 0.97, mic.rms)
        marca = "HABLANDO" if mic.rms > config.UMBRAL_VOZ else ""
        print("\rRMS %.3f  max %.3f  [%-40s] %-8s"
              % (mic.rms, maximo, "#" * min(40, int(mic.rms * 400)), marca), end="", flush=True)
except KeyboardInterrupt:
    print()
finally:
    mic.cerrar()
