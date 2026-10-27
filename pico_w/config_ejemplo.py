# Plantilla de configuracion. Copiala como config.py y pon tus datos ahi:
# config.py no se sube al repositorio (esta en .gitignore).

# Red WiFi de la casa. Si no se puede conectar, la Pico crea su propia red (AP).
WIFI_RED = "nombre_de_tu_red"
WIFI_CLAVE = "clave_de_tu_red"

# Red propia de la Pico (si falla la de la casa). La clave debe tener 8+ caracteres.
# Cambiala: cualquiera cerca del cubo podria conectarse.
AP_NOMBRE = "CuboLED"
AP_CLAVE = "cambiar-esta-clave"

# Pines de la Pico W hacia el conector JP7 del cubo
PIN_DATOS = 2     # GP2 -> DS (pin 14) de U1
PIN_RELOJ = 3     # GP3 -> SH_CP (pin 11) de todos los registros
PIN_LATCH = 4     # GP4 -> ST_CP (pin 12) de todos los registros

# Bajar a 1_000_000 si aparecen LEDs encendidos al azar (senal de 3.3 V debil)
FREQ_PIO = 5_000_000
