# Configuracion del cubo 10x10x10 en la Pico. Copiala como config.py si cambias algo:
# config.py no se sube al repositorio.

# Pines de la Pico hacia la cadena de 14 registros 74HC595
PIN_DATOS = 2     # GP2 -> DS (pin 14) de U1, el primer registro
PIN_RELOJ = 3     # GP3 -> SH_CP (pin 11) de los 14 registros
PIN_LATCH = 4     # GP4 -> ST_CP (pin 12) de los 14 registros

# Bajar a 1_000_000 si aparecen LEDs encendidos al azar (senal de 3.3 V debil)
FREQ_PIO = 5_000_000

# Niveles de brillo: 1 = encendido/apagado, 4 = 16 niveles (BAM)
BITS_BRILLO = 4
