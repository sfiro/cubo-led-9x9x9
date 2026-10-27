# Modelo del sistema

Cómo funciona el cubo LED de 9×9×9, desde los LEDs hasta el software. Este documento es la
referencia para cambiar el driver o el cableado sin romper el mapa de bits.

## 1. Estructura física y eléctrica

```
            +5V
             │
   74HC595 ──┤220Ω├──► ánodo de la COLUMNA (x, y)  ── 9 LEDs en vertical, uno por piso
                                                    │
                       cátodos del PISO z  ◄────────┘  81 LEDs unidos
                             │
                       2 × NPN (base con 1 kΩ, salida del 595)
                             │
                            GND
```

- **81 columnas**: cada una une los ánodos de los 9 LEDs que están en la misma posición
  (x, y). Cada columna sale de una salida de 74HC595 a través de 220 Ω.
- **9 pisos**: cada uno une los cátodos de sus 81 LEDs. Dos transistores NPN conectan el piso
  a GND cuando su base está en alto.
- Un LED (z, y, x) se enciende cuando **su columna está en 1 y su piso está activo**.
- Solo se activa **un piso a la vez** (multiplexado). Cada LED queda encendido 1/9 del tiempo,
  y como se repite cientos de veces por segundo, el ojo ve el cubo completo.

## 2. Cadena de registros

Los 12 registros 74HC595 están en cadena: Q7' de cada uno va a DS del siguiente. CLOCK
(SH_CP) y LATCH (ST_CP) van a todos en paralelo.

```
MCU DATA ─► U1 ─► U2 ─► … ─► U10 ─► U11 ─► U12
```

Por cada piso se envían **96 bits** (12 bytes) y luego un pulso de LATCH. El primer bit que
sale termina en el último registro, así que el orden de envío es **U12 primero, U1 al final**,
cada byte con el bit más significativo primero.

| Registro | Bits | Contenido |
|---|---|---|
| U1 … U10 | 0–7 | Ánodos 0–79 (`p = y*9 + x`, registro `p//8 + 1`, bit `p%8`) |
| U11 | 0 | Ánodo 80 |
| U11 | 1–7 | Cátodos de los pisos 0–6 (piso n → bit n+1) |
| U12 | 0–1 | Cátodos de los pisos 7–8 |
| U12 | 2–7 | Sin usar |

Este mapa es el del sketch `prueba_cubo_funcionLED_4.ino`: el arreglo `anode[]` para los
cátodos y la función `LED()` para los ánodos. `pico_w/cubo.py` lo reproduce en `_TABLA`, y se
verificó bit a bit contra el algoritmo del Arduino.

## 3. Coordenadas

```
LED(z, y, x)    z = piso (0 abajo … 8 arriba)
                y = fila    (0 … 8)
                x = columna (0 … 8)
índice del vóxel = z*81 + y*9 + x        (0 … 728)
```

Las coordenadas fuera de 0..8 se **ignoran**. En el Arduino se pegaban al borde, y eso
producía artefactos en las animaciones.

## 4. Multiplexado

### Arduino (versión 2019)
- El Timer1 se configura en modo CTC con prescaler 64 y `OCR1A = 30`, así que hay una
  interrupción cada 124 µs.
- Cada interrupción envía 12 bytes por SPI (8 MHz), da el pulso de LATCH y pasa al siguiente
  piso. Un cuadro completo tarda 9 × 124 µs ≈ 1.1 ms, unos 900 Hz.
- El búfer `matriz0[99]` guarda 11 bytes por piso. Las animaciones escriben en él mientras la
  interrupción lo lee, sin doble búfer, así que puede verse un cuadro a medias.

### Pico W (versión actual)
El CPU no participa en el refresco. Todo lo hacen periféricos:

```
 Python ──dibuja──► vox (bytearray 729)
                     │ mostrar(): _empaquetar (viper)
                     ▼
            cuadro A / cuadro B  (array 'I' de 36 palabras, doble búfer)
                     ▲   │
  _dir[0] ───────────┘   │  DMA "datos" (36 palabras, pausado por DREQ del PIO)
     ▲                   ▼
  DMA "control" ◄─chain─ FIFO TX del PIO0 SM0 ──► DATA / CLOCK / LATCH
  (reescribe READ_ADDR_TRIG del canal de datos: ciclo infinito)
```

- **Formato del cuadro**: por piso hay 4 palabras de 32 bits. Las palabras 0–2 son los
  96 bits, en el orden de la sección 2 (palabra 0 bit 31 = U12 bit 7). La palabra 3 es el
  tiempo de espera del piso, en ciclos del PIO.
- **Programa PIO** (`_pio_cubo`):
  1. Saca 3 × 32 bits por DATA, con CLOCK por side-set y un bit cada 2 ciclos.
  2. Lee la palabra de espera, da el pulso de LATCH y espera esos ciclos.
- **Tiempos** (`freq_pio = 5 MHz`, `espera = 1000`):
  - CLOCK a 2.5 MHz.
  - Cada piso toma unos 1204 ciclos (≈ 241 µs).
  - Un cuadro completo tarda ≈ 2.17 ms, unos 460 Hz.
- **Doble búfer**: `mostrar()` empaqueta en el cuadro de atrás y cambia `_dir[0]`. El DMA toma
  el cuadro nuevo cuando termina el actual. Antes de reutilizar un cuadro, se espera a que el
  DMA haya salido de él (leyendo `READ_ADDR`).
- **Brillo**: el tiempo de espera no cambia el brillo, porque siempre hay un piso encendido.
  Solo cambia la frecuencia de refresco. Para tener niveles de brillo hace falta BAM: varios
  cuadros con esperas de 1×, 2×, 4×, … (pendiente).

## 5. Software de la Pico W

| Módulo | Responsabilidad |
|---|---|
| `cubo.py` | Clase `Cubo`: `led`, `leer`, `plano`, `apagar`, `encender`, `mostrar`, `detener` |
| `animaciones.py` | Corrutinas `async def x(c)` y la lista `LISTA` (nombre → función) |
| `fuente.py` | Fuente 8×8 de ASCII 32–127 (fila 0 arriba, bit 0 = columna derecha) |
| `web.py` | Servidor HTTP con asyncio: `/`, `/anim/<nombre>`, `/estado` |
| `main.py` | `Control` (una tarea de animación cancelable), conexión WiFi (STA o, si falla, AP) |
| `config.py` | Datos locales: WiFi y pines. No se sube al repo; la plantilla es `config_ejemplo.py` |
| `prueba.py` | Prueba del cableado piso por piso y LED por LED |

**Concurrencia**: un solo bucle asyncio. La animación activa y el servidor web son tareas. Al
elegir otra animación se cancela la tarea actual y se crea una nueva. Las animaciones solo
ceden el control en `await _paso(...)`, así que un cálculo largo sin `await` retrasa la
respuesta de la web, pero no el refresco del cubo.

## 6. Restricciones eléctricas

- **Niveles lógicos**: la Pico entrega 3.3 V y un 74HC595 a 5 V pide al menos 3.5 V para
  leer un "1". Funciona en la práctica, pero fuera de la especificación. Mitigaciones: pines a
  12 mA con flanco rápido (`_fuerza_pin`), un CLOCK lento y, si hace falta, un 74AHCT125.
- **Corriente**:
  - Con 220 Ω cada columna consume unos 10–13 mA.
  - Un 74HC595 admite unos 70 mA en total, así que con 8 salidas encendidas queda al límite.
  - Un piso completo encendido suma unos 0.9–1 A, repartido entre sus 2 transistores.
- **Pines usados**:
  - GP2 (DATA), GP3 (CLOCK) y GP4 (LATCH).
  - PIO0 SM0 para el cubo. El WiFi de la Pico W usa PIO1.
  - 2 canales de DMA.
