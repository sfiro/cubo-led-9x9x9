# Cubos LED 9×9×9 y 10×10×10

Dos cubos de LEDs hechos a mano, manejados con registros de desplazamiento 74HC595:

- **9×9×9** (729 LEDs), construido según el Instructable
  [Cubo LED 9x9x9 con Arduino](https://www.instructables.com/Cubo-LED-9x9x9-Con-Arduino).
- **10×10×10** (1000 LEDs azules), con el mismo diseño, más grande.

El proyecto empezó en 2019 con Arduino. Hoy los dos cubos funcionan con una **Raspberry Pi Pico**
(MicroPython), que refresca el cubo sin parpadeo y con 16 niveles de brillo. Encima de la Pico
puede ir una **Raspberry Pi** que piensa: el asistente de voz en el 9×9×9 y, más adelante, la
página web con QR en el 10×10×10.

Cómo funciona por dentro (cableado, cadena de bits, multiplexado, software y protocolos):
**[model.md](model.md)**. Conviene leerlo antes de cambiar el driver o el cableado.

## Estado actual

| Proyecto | Cubo | Hardware | Versión | Estado |
|---|---|---|---|---|
| **[Asistente de voz](asistente_voz/README.md)** | 9×9×9 | Raspberry Pi 5 + Pico | `asistente-v5.12` | En la tienda **Cali Abstracto**. Arranca solo al encender la Pi. Conversa sobre la tienda y sobre Cali, y pone las animaciones con la voz |
| **[Cubo 10×10×10](cubo10/README.md)** | 10×10×10 | Pico (luego + Raspberry Pi 3B+) | `cubo10-v2.0` | Funciona solo con la Pico: al encenderla recorre 41 animaciones. Sigue la v3: página web para el celular con QR |
| **[Pico W sola](pico_w/LEEME.md)** | 9×9×9 | Pico W | — | 34 animaciones y una página web por WiFi para elegirlas |
| **[Simulador](simulador/LEEME.md)** | los dos | Navegador | — | Corre las mismas animaciones en 3D: [9×9×9](https://sfiro.github.io/cubo-led-9x9x9/simulador/) y [10×10×10](https://sfiro.github.io/cubo-led-9x9x9/simulador/?tam=10) |

```
                         ┌───────────────────────────── Raspberry Pi (opcional) ─────┐
 micrófono, parlante ───►│ asistente de voz (Pi 5) · página web con QR (Pi 3B+, v3)  │
                         └──────────────── cuadros por USB ───────────────────────────┘
                                                   │
                                                   ▼
          Raspberry Pi Pico ── PIO + DMA ── DATA / CLOCK / LATCH ──► cadena de 74HC595 ──► cubo
          (pico_w/cubo.py: el mismo driver para los dos cubos)
```

## Los dos cubos

| | 9×9×9 | 10×10×10 |
|---|---|---|
| LEDs | 729 | 1000, azules |
| Registros 74HC595 | 12: U1–U11 ánodos, U11–U12 cátodos | 14: U1–U13 ánodos, U13–U14 cátodos |
| Bits por piso | 96 | 112 (el PIO manda 128) |
| Driver | `Cubo()` | `Cubo(tam=10)` |
| Animaciones | `animaciones.py` tal cual | `animaciones.configurar(10)` |
| Arduino original | Mega: `prueba_cubo_funcionLED_4/` | Uno: `cuboLED10por10/`; Mega: `cuboLEDeeprom/` |

Los dos usan solo 3 señales: **DATA**, **CLOCK** y **LATCH**. En la Pico van a **GP2, GP3 y
GP4** (pines físicos 4, 5 y 6), más el **GND común** con la fuente del cubo.

### Hardware del 9×9×9

| Parte | Cantidad | Función |
|---|---|---|
| LEDs | 729 | 9 pisos × 81 columnas. Ánodos unidos por columna, cátodos unidos por piso |
| 74HC595 | 12 | U1–U11: los 81 ánodos. U11–U12: los 9 cátodos |
| Resistencias 220 Ω | 81 | Limitan la corriente de cada columna |
| Transistores NPN (TO-92) | 18 | 2 por piso, conectan el piso a GND |
| Resistencias 1 kΩ | 18 | Base de los transistores |
| Fuente 5 V | 1 | Alimenta registros y LEDs, 2 A o más |

Esquemáticos (Fritzing): [CircuitoCuboLED.fzz](CircuitoCuboLED.fzz) (editable),
[CircuitoCuboLED_esquemático.jpg](CircuitoCuboLED_esquemático.jpg),
[CircuitoCuboLED.jpg](CircuitoCuboLED.jpg) y [cubo Esquematico.jpg](cubo%20Esquematico.jpg).
[Archivos cubo led.ai](Archivos%20cubo%20led.ai) tiene los diseños en Illustrator.

El hardware del 10×10×10 y su mapa de bits están en [cubo10/README.md](cubo10/README.md).

**Si un cubo no enciende**, revisa primero:
1. La fuente: 5 V firmes con todo encendido, 2 A o más.
2. El GND común entre la Pico y la fuente.
3. Que **CLOCK vaya al pin 11 y LATCH al pin 12** de los 74HC595. Si están cruzados, los datos
   avanzan un solo lugar por envío y nada funciona.

## Estructura del repositorio

| Carpeta | Plataforma | Contenido |
|---|---|---|
| `pico_w/` | Pico / Pico W, MicroPython | **El driver común** (`cubo.py`), las animaciones (`animaciones.py`, `fuente.py`) y el programa del 9×9×9 solo, con página web |
| `asistente_voz/` | Raspberry Pi 5 + Pico | Asistente de voz local con el 9×9×9 como pantalla. Documentos de la tienda en `conocimiento/` |
| `cubo10/` | Pico (+ Raspberry Pi 3B+) | El 10×10×10: programa de la Pico, diagnóstico, documentación y plan de versiones |
| `simulador/` | Navegador | Simulador 3D (Three.js + MicroPython WebAssembly) y editor de animaciones |
| `prueba_cubo_funcionLED_4/` | Arduino Mega | Última versión de Arduino para el 9×9×9, con unas 30 animaciones |
| `cuboLEDeeprom/` | Arduino Mega | El 10×10×10: cambia de animación en cada reinicio usando la EEPROM |
| `cuboLED10por10/` | Arduino Uno | Primera versión para el 10×10×10 |
| `cuboLedFunciones2/`, `cuboLedFunciones3/` | Arduino | Versiones intermedias de las animaciones |
| `prueba_BAM_multiplexion_3/` | Arduino | Prueba de 4 niveles de brillo (Bit Angle Modulation) |
| `Prueba_multiplexion_2/`, `CuboLed1/`, `timer/` | Arduino | Pruebas del multiplexado y del Timer1 |
| `PruebaDeRegistros/`, `prueba_registros_num1/` | Arduino | Pruebas de los 74HC595 |
| `prueba_chips/` | Arduino Mega/Uno | Diagnóstico chip por chip desde el Monitor Serie |
| `prueba_arduino_uno/`, `prueba_polaridad/`, `prueba_mapa/` | Arduino Mega/Uno | Pruebas de pisos, de polaridad y del mapa de bits |

Los sketches de Arduino se conservan tal como estaban, como historia y como referencia del
cableado.

## Versiones

Cada proyecto guarda sus versiones como **etiquetas de git** y documenta cada una aparte:

| Proyecto | Etiquetas | Documentos |
|---|---|---|
| Asistente de voz | `asistente-v1.0` … `asistente-v5.12` | [asistente_voz/versiones/](asistente_voz/versiones/README.md) |
| Cubo 10×10×10 | `cubo10-v1.0`, `cubo10-v2.0` | [cubo10/versiones/](cubo10/versiones/README.md) |

Todas las etiquetas: <https://github.com/sfiro/cubo-led-9x9x9/tags>.

## Inicio rápido

**9×9×9 con la Pico W sola:**
1. Instala MicroPython 1.22 o más nuevo en la Pico W.
2. Conecta GP2 → DATA, GP3 → CLOCK, GP4 → LATCH y GND con GND.
3. Copia `pico_w/config_ejemplo.py` como `config.py` y pon los datos de tu WiFi.
4. Copia los `.py` de `pico_w/` a la Pico, corre `prueba.py` y luego reinicia.
5. Abre en el celular la IP que imprime la consola.

Detalles en [pico_w/LEEME.md](pico_w/LEEME.md).

**10×10×10:** de `pico_w/` van `cubo.py`, `animaciones.py` y `fuente.py`, y de `cubo10/pico/`
van `main.py`, `diagnostico.py` y `config_ejemplo.py`. Ver [cubo10/README.md](cubo10/README.md).

**Asistente de voz:** se instala en la Raspberry Pi 5 con `instalar.sh`. Ver
[asistente_voz/README.md](asistente_voz/README.md).

## Probar animaciones sin el cubo

En línea: [simulador 9×9×9](https://sfiro.github.io/cubo-led-9x9x9/simulador/) y
[simulador 10×10×10](https://sfiro.github.io/cubo-led-9x9x9/simulador/?tam=10).

En el computador, desde la carpeta del repositorio:
```bash
python3 -m http.server 8765
```
Luego abre <http://localhost:8765/simulador/> (o `?tam=10`).

## Ideas pendientes

- **10×10×10 v3:** Raspberry Pi 3B+ por USB con la página para el celular: botones de
  animaciones, "tu nombre en el cubo", "graba tu video" y turnos.
- **10×10×10 v4:** el QR por internet (`cubo.ingenioabstracto.co`, con un túnel de Cloudflare)
  y un código en el cubo para jugar solo en la tienda.
- **10×10×10 v5:** mover el cubo con el giroscopio del celular, dibujar, y descuentos.
- **Asistente:** medios de pago y envíos de la tienda, una frase por categoría de producto, y
  aclarar los nombres raros del catálogo.
- Usar los 16 niveles de brillo en la Pico W sola del 9×9×9, que hoy enciende y apaga.
- Agregar las 8 animaciones del Arduino Mega (`NUEVAS`) al demo del 9×9×9. Ya funcionan en ese
  tamaño, pero no están en su demo, para no cambiar el cubo de la tienda.
- Un adaptador de nivel 74AHCT125, si la señal de 3.3 V da problemas.

## Mantener la documentación al día

Cada cambio actualiza, en el mismo commit:
- el README del proyecto que cambió y, si cambia la versión, su documento en `versiones/`;
- **este README**: la tabla "Estado actual", las versiones y las ideas pendientes;
- **[model.md](model.md)**, si cambia el cableado, el driver, un protocolo o cómo se reparte el
  trabajo.
