# Cubo LED 10×10×10

Segundo cubo del proyecto: 1000 LEDs azules, 10 pisos de 10×10. Va a estar en una tienda, sin
voz: la gente escanea un QR, controla el cubo desde el celular y graba el video.

```
Celular ──QR──► página web ──► Raspberry Pi 3B+ ──USB──► Pico ──► 14 registros 74HC595 ──► cubo
```

- **La Pico** maneja el cubo: multiplexa los 10 pisos con el PIO y el DMA, con 16 niveles de
  brillo y sin parpadeo. Usa el **mismo driver que el 9×9×9** ([`pico_w/cubo.py`](../pico_w/cubo.py)),
  con `Cubo(tam=10)`.
- **La Raspberry Pi 3B+** (desde la v3) genera las animaciones, sirve la página web y le manda
  los cuadros a la Pico por USB.

## Hardware

Es el mismo diseño del 9×9×9, más grande. El cableado sale de los sketches
[`cuboLED10por10`](../cuboLED10por10/cuboLED10por10.ino) (Arduino Uno) y
[`cuboLEDeeprom`](../cuboLEDeeprom/cuboLEDeeprom.ino) (Arduino Mega), que manejaban este cubo.

| Parte | Cantidad | Función |
|---|---|---|
| LEDs azules | 1000 | 10 pisos × 100 columnas. Ánodos unidos por columna, cátodos unidos por piso |
| 74HC595 | 14 | U1–U13: los 100 ánodos. U13–U14: los 10 cátodos |
| Transistores | por piso | Conectan cada piso a GND (como en el 9×9×9) |
| Fuente 5 V | 1 | Alimenta registros y LEDs. 2 A o más |

### Mapa de bits
Cada piso se manda como una cadena de 14 bytes. El primer registro (U1) es el que recibe DATA.

| Registro | Bits | Qué maneja |
|---|---|---|
| U1 … U12 | 0–7 | Ánodos 0–95 del piso: el ánodo `p = fila × 10 + columna` está en U`p//8 + 1`, bit `p % 8` |
| U13 | 0–3 | Ánodos 96–99 |
| U13 | 4–7 | Cátodos de los pisos 0–3 |
| U14 | 0–5 | Cátodos de los pisos 4–9 |
| U14 | 6–7 | Sin usar |

El PIO de la Pico manda palabras de 32 bits, así que cada piso son 128 bits. Los primeros 16
sobran: pasan por toda la cadena y se pierden al final.

### Conexión con la Pico

| Pico | Pin físico | Cubo |
|---|---|---|
| GP2 | 4 | DATA → DS (pin 14) de U1 |
| GP3 | 5 | CLOCK → SH_CP (pin 11) de los 14 registros |
| GP4 | 6 | LATCH → ST_CP (pin 12) de los 14 registros |
| GND | 3 o 38 | GND de la fuente del cubo (**obligatorio**) |

- En los 74HC595, **OE (pin 13) a GND** y **MR (pin 10) a 5 V**.
- **CLOCK va al pin 11 y LATCH al pin 12.** Si están cruzados, nada funciona: es lo que le pasó
  al 9×9×9.
- **Alimentación:** mientras programas, la Pico va por USB y comparte solo el GND con la fuente
  del cubo. Nunca conectes 5 V a un pin GPIO ni al pin 3V3.
- **3.3 V contra 5 V:** la Pico da 3.3 V y un 74HC595 a 5 V espera unos 3.5 V. Casi siempre
  funciona, como en el 9×9×9. Si aparecen LEDs locos, baja `FREQ_PIO` a `1_000_000` en
  `config.py`, usa cables cortos o pon un 74AHCT125 entre la Pico y el cubo.

## Instalación en la Pico
1. Instala MicroPython 1.22 o más nuevo: mantén apretado BOOTSEL, conecta el USB y copia el
   `.uf2` de <https://micropython.org/download/RPI_PICO/> a la unidad `RPI-RP2`.
2. Copia a la Pico, con [Thonny](https://thonny.org) o con `mpremote`. **Ojo:** de `pico_w/` solo
   va `cubo.py`. El `diagnostico.py`, el `main.py` y el `config_ejemplo.py` de `pico_w/` son del
   9×9×9; los del 10×10×10 están en `cubo10/pico/`.
   - [`pico_w/cubo.py`](../pico_w/cubo.py): el driver, el mismo del 9×9×9;
   - [`pico/diagnostico.py`](pico/diagnostico.py);
   - [`pico/main.py`](pico/main.py);
   - [`pico/config_ejemplo.py`](pico/config_ejemplo.py). Si cambias pines o brillo, cópialo como
     `config.py`.

   Con `mpremote`, desde la carpeta del repo:
   ```bash
   mpremote fs cp pico_w/cubo.py :cubo.py + fs cp cubo10/pico/diagnostico.py :diagnostico.py + fs cp cubo10/pico/config_ejemplo.py :config_ejemplo.py
   ```
3. Abre `diagnostico.py` en Thonny y dale **Run**. Responde s/n a cada prueba:
   - **Etapa 1**, con señales lentas: todo el cubo, piso por piso, y una fila y una columna del
     piso 0.
   - **Etapa 2**, el PIO: todo el cubo multiplexado.
   - **Etapa 3**, el driver completo: planos en los 3 ejes, un LED recorriendo un piso y los 16
     niveles de brillo.
4. Si todo pasa, copia `main.py` y reinicia la Pico: el cubo arranca solo.
   ```bash
   mpremote fs cp cubo10/pico/main.py :main.py + reset
   ```

## Plan

| Versión | Qué trae |
|---|---|
| **v1** | El driver de la Pico para el 10×10×10, el diagnóstico y 5 animaciones para probarlo |
| v2 | Las animaciones de `cuboLEDeeprom` y del 9×9×9 en 10×10×10, y el simulador |
| v3 | La Raspberry Pi 3B+ por USB y la página web: botones de animaciones, "tu nombre en el cubo", "graba tu video" y turnos |
| v4 | El QR por internet: `cubo.ingenioabstracto.co` con un túnel de Cloudflare, y el código que muestra el cubo para jugar solo en la tienda |
| v5 | Mover el cubo con el giroscopio del celular, dibujar, y descuentos para la tienda |

Cada versión queda en git con una etiqueta `cubo10-vN.0` y un documento en
[`versiones/`](versiones/). Lo que trae cada versión y cómo probarla está en
[`versiones/README.md`](versiones/README.md).

## El 9×9×9 no cambia
El driver es uno solo para los dos cubos. Con `tam=9` (lo predeterminado) usa exactamente el
mismo código de empaquetado y el mismo programa del PIO de antes. Se comprobó bit por bit que
los cuadros del 9×9×9 salen idénticos. El cubo de la tienda con el asistente de voz no necesita
ninguna actualización.
