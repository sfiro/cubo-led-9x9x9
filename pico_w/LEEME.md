# Cubo LED 9×9×9 con Raspberry Pi Pico W (MicroPython)

Mismo hardware del Instructable: 12 registros 74HC595 y 18 transistores. Solo cambia el
Arduino por una Pico W.

| Archivo | Qué hace |
|---|---|
| `cubo.py` | Driver: el PIO y el DMA multiplexan los 9 pisos solos, sin usar el CPU. Incluye doble búfer, así que no se ven cuadros a medias |
| `animaciones.py` | 22 animaciones: las portadas de `prueba_cubo_funcionLED_4.ino`, más otras nuevas con física y geometría |
| `fuente.py` | La fuente de letras 8×8 del sketch original |
| `web.py` | Página para elegir la animación desde el celular |
| `main.py` | Arranca en modo demo, se conecta al WiFi y levanta la página |
| `config_ejemplo.py` | Plantilla de configuración. Cópiala como `config.py`, que queda fuera del repo |
| `prueba.py` | Prueba de pisos y de LED por LED, para revisar el cableado |

## Conexión

| Pico W | Pin físico | Cubo (conector JP7) |
|---|---|---|
| GP2 | 4 | DATA → DS (pin 14) de U1 |
| GP3 | 5 | CLOCK → SH_CP (pin 11) de todos los 595 |
| GP4 | 6 | LATCH → ST_CP (pin 12) de todos los 595 |
| GND | 3 o 38 | GND de la fuente del cubo (**obligatorio**) |

- Revisa que en los 74HC595 **OE (pin 13) esté a GND** y **MR (pin 10) a 5 V**. El sketch viejo
  definía un `blank_pin` en el pin 4 del Arduino: si OE llega al conector, conéctalo a GND.
- **Alimentación:**
  - Mientras programas, alimenta la Pico por USB y comparte solo el GND con la fuente del cubo.
  - Para que funcione sola, lleva los 5 V del cubo a **VSYS (pin 39) a través de un diodo
    Schottky** (1N5817 o similar). Así no hay problema si después conectas el USB.
  - **Nunca** conectes 5 V a un pin GPIO ni al pin 3V3.

### Sobre conectar directo a 3.3 V
Un 74HC595 a 5 V garantiza leer un "1" solo desde unos 3.5 V, y la Pico entrega 3.3 V. Casi
siempre funciona, pero queda al límite. El driver ya ayuda: pone los pines a 12 mA con flancos
rápidos, y el reloj es lento (2.5 MHz). Si ves LEDs encendidos al azar, parpadeos o pisos
corridos, prueba en este orden:
1. Bajar `FREQ_PIO` en `config.py` a `1_000_000`.
2. Usar cables cortos y unir los GND con un cable grueso.
3. Poner un 74AHCT125 (unos 0.50 USD) entre la Pico y el cubo. Es la solución definitiva.

## Instalación
1. Instala MicroPython para **Pico W**: mantén apretado BOOTSEL, conecta el USB y copia el
   `.uf2` de <https://micropython.org/download/RPI_PICO_W/> a la unidad `RPI-RP2`. Usa la
   versión 1.22 o más nueva, que trae `rp2.DMA`.
2. Copia `config_ejemplo.py` como `config.py`, pon tu red WiFi y cambia `AP_CLAVE`.
3. Copia todos los `.py` a la Pico con [Thonny](https://thonny.org) (Archivo → Guardar como →
   Raspberry Pi Pico) o con `mpremote`:
   ```bash
   mpremote cp cubo.py animaciones.py fuente.py web.py config.py main.py prueba.py :
   ```
4. **Primero prueba el cableado:** abre `prueba.py` en Thonny y dale Run. Debe encender cada
   piso y después recorrer los 729 LEDs uno por uno, imprimiendo en qué registro está cada uno.
5. Reinicia la Pico. `main.py` arranca solo e imprime la dirección de la página, por ejemplo
   `http://192.168.1.50`. Ábrela en el celular, que tiene que estar en la misma red.
   - Si no logra conectarse al WiFi, la Pico crea la red `CuboLED`. Conéctate a ella y abre la
     dirección que imprime (normalmente `http://192.168.4.1`).

## Agregar animaciones
En `animaciones.py`:
```python
async def mi_animacion(c):
    c.apagar()
    for z in range(9):
        c.led(z, 4, 4, 1)          # z = piso, y = fila, x = columna (0..8)
        await _paso(c, 100)        # muestra el cuadro y espera 100 ms
```
Después agrégala a `LISTA`, al final del archivo, y aparece sola en la página.

## Animaciones

| Nombre | Qué hace |
|---|---|
| `marquesina` | Texto que corre alrededor de las 4 caras |
| `cubo_giratorio` | Cubo de alambre que gira en dos ejes y cambia de tamaño |
| `lluvia` | Gotas a distinta velocidad, con viento, estela y salpicaduras en el piso |
| `fuegos` | Cohete que sube y explota con gravedad |
| `helice` | Doble hélice tipo ADN girando |
| `planos` | Planos que se abren desde el centro, plano diagonal y plano que gira |
| `rebotes` | Tres pelotas con gravedad que rebotan |
| `disparos` | Proyectiles desde las 6 caras con onda expansiva al chocar |
| `olas` | Interferencia de dos fuentes de ondas en movimiento |
| `tornado` | Partículas en espiral; el embudo se pasea por el piso |
| `fuente` | Chorro de agua en parábola |
| `vida` | Juego de la vida en 3D, con los bordes del cubo conectados |
| `espectro` | Ecualizador en cascada que se aleja hacia el fondo |
| `texto`, `bordes`, `flash`, `llenar_vaciar`, `onda`, `aleatorio`, `serpiente`, `esfera`, `ascensores` | Portadas del sketch de Arduino |

Antes de cargarlas en el cubo se pueden ver en el [simulador](../simulador/).

## Bugs del código original que ya quedaron corregidos
- `LED()` pegaba a los bordes las coordenadas fuera del cubo, y por eso los fuegos artificiales
  dejaban LEDs pegados en las caras. Ahora se ignoran.
- `lluvia()` escribía fuera de `vector[1][6]`.
- `effect_axis_updown_randsuspend` usaba `x*8+y` y `7` (restos del cubo de 8×8×8).
- `letras()` leía una 9.ª fila de la fuente, que no existe.
