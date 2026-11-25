# Modelo del sistema

Cómo funcionan los cubos LED de 9×9×9 y 10×10×10, desde los LEDs hasta el software. Este
documento es la referencia para cambiar el driver, el cableado o los protocolos sin romper
nada. El asistente de voz tiene su propio modelo, más detallado:
[asistente_voz/model.md](asistente_voz/model.md).

## 1. Reparto del trabajo

```
          ┌──────────────────────── tres formas de usar un cubo ───────────────────────┐
          │                                                                            │
 (a) Pico sola        animaciones.py en la Pico ──► cubo.py ──► cubo                   │
     9×9×9: pico_w/main.py (con página web si es Pico W)                               │
     10×10×10: cubo10/pico/main.py (demo de 41 animaciones)                            │
                                                                                       │
 (b) Raspberry Pi     la Pi genera los cuadros ──USB──► Pico (cubo.py) ──► cubo        │
     + Pico           9×9×9: asistente de voz (Pi 5)   10×10×10: página con QR (v3)    │
                                                                                       │
 (c) Simulador        animaciones.py en MicroPython WebAssembly ──► Three.js           │
          └────────────────────────────────────────────────────────────────────────────┘
```

- **La Pico** siempre hace lo mismo: refrescar el cubo con el PIO y el DMA, sin usar el CPU.
  El driver es uno solo para los dos cubos: [`pico_w/cubo.py`](pico_w/cubo.py).
- **Las animaciones** también son unas solas: [`pico_w/animaciones.py`](pico_w/animaciones.py).
  Corren en la Pico, en la Raspberry Pi (con `asistente_voz/raspberry/animaciones_pi.py`) y en
  el simulador, sin cambiar el código.
- **La Raspberry Pi**, cuando la hay, piensa y genera los cuadros; la Pico solo los muestra.

## 2. Estructura física y eléctrica

```
            +5V
             │
   74HC595 ──┤R├──► ánodo de la COLUMNA (x, y)  ── N LEDs en vertical, uno por piso
                                                 │
                       cátodos del PISO z  ◄─────┘  N×N LEDs unidos
                             │
                       transistores NPN (base desde una salida del 595)
                             │
                            GND
```

- **Columnas**: cada una une los ánodos de los N LEDs que están en la misma posición (x, y), y
  sale de una salida de 74HC595 a través de una resistencia. 81 en el 9×9×9 (220 Ω) y 100 en el
  10×10×10.
- **Pisos**: cada uno une los cátodos de sus N×N LEDs. Los transistores conectan el piso a GND
  cuando su base está en alto. En el 9×9×9 son 2 NPN por piso, con 1 kΩ en la base.
- Un LED (z, y, x) se enciende cuando **su columna está en 1 y su piso está activo**.
- Solo se activa **un piso a la vez** (multiplexado). Cada LED queda encendido 1/N del tiempo, y
  como se repite cientos de veces por segundo, el ojo ve el cubo completo.

## 3. Cadena de registros

Los 74HC595 están en cadena: Q7' de cada uno va a DS del siguiente. CLOCK (SH_CP, pin 11) y
LATCH (ST_CP, pin 12) van a todos en paralelo. OE (pin 13) va a GND y MR (pin 10) a 5 V.

```
MCU DATA ─► U1 ─► U2 ─► … ─► U(R-1) ─► U(R)        R = 12 (9×9×9) o 14 (10×10×10)
```

Por cada piso se envían los bits de toda la cadena y luego un pulso de LATCH. El primer bit
que sale termina en el último registro, así que el orden de envío es **U(R) primero, U1 al
final**, cada byte con el bit más significativo primero (como el SPI del Arduino).

**En los dos cubos**, el ánodo `p = y·N + x` está en el registro `p//8 + 1`, bit `p % 8`. Lo
que cambia es dónde están los cátodos:

**9×9×9** (12 registros, 96 bits), del sketch `prueba_cubo_funcionLED_4.ino`:

| Registro | Bits | Contenido |
|---|---|---|
| U1 … U10 | 0–7 | Ánodos 0–79 |
| U11 | 0 | Ánodo 80 |
| U11 | 1–7 | Cátodos de los pisos 0–6 (piso n → bit n+1) |
| U12 | 0–1 | Cátodos de los pisos 7–8 |
| U12 | 2–7 | Sin usar |

**10×10×10** (14 registros, 112 bits), de los sketches `cuboLED10por10` y `cuboLEDeeprom`:

| Registro | Bits | Contenido |
|---|---|---|
| U1 … U12 | 0–7 | Ánodos 0–95 |
| U13 | 0–3 | Ánodos 96–99 |
| U13 | 4–7 | Cátodos de los pisos 0–3 (piso n → bit n+4) |
| U14 | 0–5 | Cátodos de los pisos 4–9 (piso n → bit n−4) |
| U14 | 6–7 | Sin usar |

En `pico_w/cubo.py`, el diccionario `MAPAS` guarda, para cada tamaño, cuántos registros hay y
dónde está el cátodo de cada piso. `tabla_de(tam)` arma la tabla con la posición de cada bit en
el flujo que sale por DATA. Para el 9×9×9, `_TABLA` es la tabla de siempre.

## 4. Coordenadas

```
LED(z, y, x)    z = piso    (0 abajo … N-1 arriba)
                y = fila    (0 … N-1)
                x = columna (0 … N-1)
índice del vóxel = (z·N + y)·N + x      (0 … 728 en el 9×9×9, 0 … 999 en el 10×10×10)
```

Las coordenadas fuera del cubo se **ignoran**. En el Arduino se pegaban al borde, y eso
producía artefactos en las animaciones.

## 5. Multiplexado

### Arduino (2019)
- El Timer1 en modo CTC con prescaler 64 da una interrupción cada `(OCR1A + 1) × 4 µs`:
  - 9×9×9: `OCR1A = 30`, cada 124 µs por piso. Un cuadro completo tarda 1.1 ms (unos 900 Hz).
  - 10×10×10: `OCR1A = 50`, cada 204 µs por piso. Un cuadro tarda 2 ms (unos 490 Hz).
- Cada interrupción envía los bytes del piso por SPI (8 MHz), da el pulso de LATCH y pasa al
  siguiente piso.
- Las animaciones escriben en el mismo búfer que lee la interrupción, sin doble búfer, así que
  puede verse un cuadro a medias.

### Pico (versión actual, `pico_w/cubo.py`)
El CPU no participa en el refresco. Todo lo hacen periféricos:

```
 Python ──dibuja──► vox (bytearray N³, 1 byte = nivel de brillo)
                     │ mostrar(): _empaquetar (9×9×9) o _empaquetar_n (otros), en viper
                     ▼
            cuadro A / cuadro B  (array 'I', doble búfer)
                     ▲   │
  _dir[0] ───────────┘   │  DMA "datos" (pausado por DREQ del PIO)
     ▲                   ▼
  DMA "control" ◄─chain─ FIFO TX del PIO0 SM0 ──► DATA / CLOCK / LATCH
  (reescribe READ_ADDR_TRIG del canal de datos: ciclo infinito)
```

- **Palabras por piso**: el PIO saca palabras de 32 bits.
  - 9×9×9: 3 palabras = 96 bits, justo los 12 registros.
  - 10×10×10: 4 palabras = 128 bits. Los primeros 16 sobran: pasan por toda la cadena y se
    pierden al final.
  - Después de los datos va **una palabra más**: el tiempo de espera del piso, en ciclos del PIO.
- **Programa PIO**: `_pio_cubo` (3 palabras) o `_pio_cubo4` (4 palabras). Saca los bits por DATA,
  con CLOCK por side-set y un bit cada 2 ciclos; lee la espera, da el pulso de LATCH y espera.
  Cargar un piso toma `66 × palabras + 6` ciclos: 204 en el 9×9×9 y 270 en el 10×10×10.
- **Tiempos** con `freq_pio = 5 MHz`:
  - CLOCK a 2.5 MHz.
  - Con `bits=1` y `espera=1000`, cada piso del 9×9×9 toma unos 1204 ciclos (241 µs) y el
    cuadro unos 2.17 ms (460 Hz).
- **Doble búfer**: `mostrar()` empaqueta en el cuadro de atrás y cambia `_dir[0]`. El DMA toma
  el cuadro nuevo cuando termina el actual. Antes de reutilizar un cuadro, se espera a que el
  DMA haya salido de él (leyendo `READ_ADDR`).
- **Brillo**: el tiempo de espera no cambia el brillo, porque siempre hay un piso encendido.
  Solo cambia la frecuencia de refresco.
- **Niveles de brillo (BAM)**, con `Cubo(bits=2..4)`:
  - Cada piso se muestra en `bits` pasadas. La pasada `b` enciende los LEDs que tienen en 1 el
    bit `b` de su nivel, y dura `2^b` veces la primera (`base_us`, 60 µs).
  - De cada espera se descuentan los ciclos de cargar el piso siguiente, porque mientras tanto
    sigue encendida la pasada anterior. Así el tiempo encendido es exactamente
    `nivel / (2^bits − 1)` del tiempo del piso.
  - Con 4 bits a 5 MHz hay 16 niveles: unos 123 Hz en el 9×9×9 y 111 Hz en el 10×10×10.
  - Con `bits=1` (el predeterminado) el cuadro es encendido/apagado.
  - Lo usan el asistente de voz y el 10×10×10.
- **Compatibilidad**: con `tam=9`, el driver usa exactamente el mismo código de antes
  (`_TABLA`, `_empaquetar`, `_pio_cubo`). Se comprobó bit a bit que los cuadros del 9×9×9 son
  idénticos; ver la sección 10.

## 6. Software de la Pico

| Módulo | Dónde | Responsabilidad |
|---|---|---|
| `cubo.py` | `pico_w/` | Clase `Cubo(..., bits, tam)`: `led`, `brillo`, `leer`, `plano`, `apagar`, `encender`, `mostrar`, `detener` |
| `animaciones.py` | `pico_w/` | Corrutinas `async def x(c)`, `LISTA` (34, el demo del 9×9×9), `NUEVAS` (8 del Arduino Mega), `todas()`, `POR_NOMBRE` y `configurar(n)` |
| `fuente.py` | `pico_w/` | Fuente 8×8 de ASCII 32–127 (fila 0 arriba, bit 0 = columna derecha) |
| `main.py`, `web.py` | `pico_w/` | 9×9×9 solo: una tarea de animación cancelable y, en la Pico W, WiFi y página web |
| `prueba.py`, `diagnostico.py` | `pico_w/` | Pruebas del cableado del 9×9×9 |
| `main.py` | `asistente_voz/pico/` | 9×9×9 como pantalla de la Pi: recibe cuadros por USB (sección 7) |
| `main.py` | `cubo10/pico/` | 10×10×10 solo: demo de las 41 animaciones al encender |
| `diagnostico.py` | `cubo10/pico/` | Pruebas del cableado del 10×10×10, en 3 etapas |
| `config.py` | cada carpeta | Datos locales (pines, WiFi, brillo). No se sube; la plantilla es `config_ejemplo.py` |

**Animaciones para los dos tamaños.** `animaciones.py` arranca en 9×9×9. `configurar(10)`
cambia el tamaño y las variables que dependen de él: `N` (lado), `M` (último índice), `H` y `C`
(el centro como entero y exacto), `N2` (LEDs por piso), `S` (escala de las figuras), `TOPE` (la
fila de arriba de las letras de 8×8, centradas en altura) y el recorrido de las caras. Una
animación nueva debe usar esas variables, no números fijos.

**Concurrencia**: un solo bucle asyncio. La animación activa (y, en la Pico W, el servidor web)
son tareas. Al elegir otra animación se cancela la tarea actual y se crea una nueva. Las
animaciones solo ceden el control en `await _paso(...)`, así que un cálculo largo retrasa la
respuesta de la web, pero no el refresco del cubo.

## 7. Raspberry Pi + Pico por USB

La Pi manda cada cuadro como una línea de texto por el puerto serie USB de la Pico
(`asistente_voz/pico/main.py`):

| Línea | Significado |
|---|---|
| `F` + 184 hex + `\n` | 92 bytes: 729 LEDs, 1 bit cada uno |
| `G` + 730 hex + `\n` | 365 bytes: 729 LEDs, 4 bits cada uno (brillo 0–15) |
| `A\n` | Apagar todo |
| `?\n` | La Pico responde `CUBO9 <versión> <bits>` |

Se usa texto y no bytes crudos para que nunca viaje un 0x03 (Ctrl-C), y así Thonny puede
detener el programa. Si la Pi no manda nada en 3 s, la Pico muestra una esfera que respira.

Hoy el protocolo es del 9×9×9. En la v3 del 10×10×10 se amplía para 1000 LEDs, y la Pico dirá
su tamaño en la respuesta a `?`. El detalle del lado de la Pi (hilos, sincronía con la voz,
brillo, estela) está en [asistente_voz/model.md](asistente_voz/model.md).

## 8. Restricciones eléctricas

- **Niveles lógicos**: la Pico entrega 3.3 V y un 74HC595 a 5 V pide unos 3.5 V para leer un
  "1". Funciona en la práctica, pero fuera de la especificación. Mitigaciones: pines a 12 mA con
  flanco rápido (`_fuerza_pin`), un CLOCK lento (bajar `FREQ_PIO` a 1 MHz si hace falta) y, como
  solución definitiva, un 74AHCT125.
- **Corriente** (9×9×9):
  - Con 220 Ω cada columna consume unos 10–13 mA.
  - Un 74HC595 admite unos 70 mA en total, así que con 8 salidas encendidas queda al límite.
  - Un piso completo encendido suma unos 0.9–1 A, repartido entre sus transistores.
- **Pines y periféricos de la Pico**:
  - GP2 (DATA), GP3 (CLOCK) y GP4 (LATCH), y GND común con la fuente del cubo.
  - PIO0 SM0 para el cubo. El WiFi de la Pico W usa PIO1.
  - 2 canales de DMA.
- **Alimentación de la Pico sin computador**: por USB con un cargador, o con los 5 V del cubo
  en VSYS (pin 39) a través de un diodo Schottky. Nunca 5 V en un GPIO ni en 3V3.

## 9. Simulador

`simulador/` ejecuta `pico_w/animaciones.py` sin cambios, con MicroPython para WebAssembly
dentro de un Web Worker:
- **Tamaño**: con `?tam=10` en la dirección, el worker llama a `animaciones.configurar(10)` y el
  visor dibuja 1000 LEDs.
- `cubo_sim.py` reemplaza a `cubo.py` con la misma interfaz. `mostrar()` envía los vóxeles a la
  página en hexadecimal: un byte 0 cortaría el texto al pasar de Python a JavaScript.
- `sim_runner.py` cambia los módulos `time` y `asyncio` de las animaciones por un reloj virtual:
  `ticks_ms()` solo avanza con las esperas de `_paso`. Eso permite pausar, avanzar cuadro a
  cuadro y cambiar la velocidad sin que cambien las duraciones.
- La página (`app.js`, Three.js) dibuja el cubo con las coordenadas de la sección 4, y detecta
  si Python deja de responder, con un latido cada 500 ms.

Cualquier animación nueva debe usar solo la interfaz de `Cubo` y esperar con `await _paso(...)`.
Así funciona igual en el simulador, en la Pico y en la Raspberry Pi.

## 10. Cómo se verifica un cambio

El 9×9×9 está en una tienda, así que un cambio en el código común no puede alterarlo:
- **Driver**: correr `cubo.py` en CPython con la Pico simulada y comparar los cuadros empaquetados
  con los de la versión anterior, en los 4 modos de brillo. Para un cubo nuevo, simular la cadena
  de registros y comparar lo que queda en cada uno con lo que mandaba el Arduino.
- **Animaciones**: correr cada animación con la versión anterior y la nueva, con la misma semilla
  al azar y un reloj simulado, y comparar todos los cuadros. Así se comprobó la v2 del
  10×10×10: las 68 corridas del 9×9×9 dieron cuadros idénticos.
- **En el cubo real**: el diagnóstico de cada cubo, y correr las animaciones midiendo el tiempo
  de cálculo de cada cuadro (en el 10×10×10, el empaquetado tarda 2 ms).
