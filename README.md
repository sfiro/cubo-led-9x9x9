# Cubo LED 9×9×9

Cubo de 729 LEDs (9 pisos de 9×9) controlado con registros de desplazamiento 74HC595,
construido según el Instructable
[Cubo LED 9x9x9 con Arduino](https://www.instructables.com/Cubo-LED-9x9x9-Con-Arduino).

El proyecto empezó en 2019 con un Arduino. Ahora se está migrando a una **Raspberry Pi Pico W**
con MicroPython, sin cambiar el hardware del cubo.

- **[pico_w/](pico_w/)**: versión actual (Pico W + MicroPython + control por WiFi).
  Las instrucciones de conexión e instalación están en [pico_w/LEEME.md](pico_w/LEEME.md).
- **[simulador/](simulador/)**: simulador 3D en el navegador que ejecuta las mismas animaciones
  de la Pico, con editor para probar animaciones nuevas. Ver [simulador/LEEME.md](simulador/LEEME.md).
- **[asistente_voz/](asistente_voz/)**: proyecto aparte. Un asistente de voz local en una
  Raspberry Pi 5, con el cubo como interfaz: se ilumina al ritmo de la voz. Ver
  [asistente_voz/README.md](asistente_voz/README.md).
- **[model.md](model.md)**: cómo funciona el sistema completo (hardware, cadena de bits,
  coordenadas, multiplexado y software). Conviene leerlo antes de modificar el driver.

## Hardware

| Parte | Cantidad | Función |
|---|---|---|
| LEDs | 729 | 9 pisos × 81 columnas. Ánodos unidos por columna, cátodos unidos por piso |
| 74HC595 | 12 | U1–U11: los 81 ánodos. U11–U12: los 9 cátodos |
| Resistencias 220 Ω | 81 | Limitan la corriente de cada columna |
| Transistores NPN (TO-92) | 18 | 2 por piso, conectan el piso a GND |
| Resistencias 1 kΩ | 18 | Base de los transistores |
| Fuente 5 V | 1 | Alimenta registros y LEDs |

El microcontrolador solo usa 3 señales: **DATA**, **CLOCK** y **LATCH** (conector JP7).

Esquemáticos (Fritzing):
- [CircuitoCuboLED.fzz](CircuitoCuboLED.fzz) (archivo editable)
- [CircuitoCuboLED_esquemático.jpg](CircuitoCuboLED_esquemático.jpg)
- [CircuitoCuboLED.jpg](CircuitoCuboLED.jpg)
- [cubo Esquematico.jpg](cubo%20Esquematico.jpg)

[Archivos cubo led.ai](Archivos%20cubo%20led.ai) contiene los diseños en Illustrator.

## Estructura del repositorio

| Carpeta | Plataforma | Contenido |
|---|---|---|
| `pico_w/` | Pico W, MicroPython | **Versión actual**: driver con PIO+DMA, 34 animaciones, página web |
| `asistente_voz/` | Raspberry Pi 5 + Pico | Asistente de voz local: la Pi escucha, piensa y habla, y la Pico muestra la voz en el cubo |
| `simulador/` | Navegador | Simulador 3D (Three.js + MicroPython WebAssembly) y editor de animaciones |
| `prueba_cubo_funcionLED_4/` | Arduino Mega | Última versión de Arduino para el 9×9×9, con unas 30 animaciones. Es la referencia del port |
| `cuboLEDeeprom/` | Arduino Mega | Variante para un cubo de 10×10×10, que cambia de animación en cada reinicio usando la EEPROM |
| `cuboLED10por10/` | Arduino | Primera versión para el 10×10×10 |
| `cuboLedFunciones2/`, `cuboLedFunciones3/` | Arduino | Versiones intermedias de las animaciones |
| `prueba_BAM_multiplexion_3/` | Arduino | Prueba de 4 niveles de brillo (Bit Angle Modulation) |
| `Prueba_multiplexion_2/`, `CuboLed1/`, `timer/` | Arduino | Pruebas del multiplexado y del Timer1 |
| `PruebaDeRegistros/`, `prueba_registros_num1/` | Arduino | Pruebas de los 74HC595 |
| `prueba_chips/` | Arduino Mega/UNO | Diagnóstico chip por chip desde el Monitor Serie: sonda en Q7', salidas una por una, envío lento y salida fija para medir |
| `prueba_arduino_uno/`, `prueba_polaridad/`, `prueba_mapa/` | Arduino Mega/UNO | Pruebas de pisos, de polaridad y del mapa de bits, imprimiendo los bytes enviados |

Si el cubo no enciende, revisa primero la fuente (5 V firmes con todo encendido, 2 A o más) y que
**CLOCK vaya al pin 11 y LATCH al pin 12** de los 74HC595: si están cruzados, los datos avanzan
un solo lugar por envío y nada funciona.

Los sketches de Arduino se conservan tal como estaban en 2019, como historia y referencia.

## Inicio rápido (Pico W)

1. Instala MicroPython 1.22 o más nuevo en la Pico W.
2. Conecta GP2 → DATA, GP3 → CLOCK, GP4 → LATCH y GND con GND.
3. Copia `pico_w/config_ejemplo.py` como `pico_w/config.py` y pon los datos de tu WiFi.
4. Copia los `.py` de `pico_w/` a la Pico, corre `prueba.py` y luego reinicia.
5. Abre en el celular la IP que imprime la consola.

Los detalles están en [pico_w/LEEME.md](pico_w/LEEME.md).

## Probar animaciones sin el cubo

```bash
python3 -m http.server 8765
```

Luego abre <http://localhost:8765/simulador/>. También se puede publicar con GitHub Pages
(los pasos están en [simulador/LEEME.md](simulador/LEEME.md)).

## Ideas pendientes

- Usar los niveles de brillo (BAM, ya en `pico_w/cubo.py` con `Cubo(bits=4)`) también en las
  animaciones de la Pico sola. Hoy los usa el asistente de voz.
- Portar las animaciones del Arduino que faltan: `folder`, `sinwaveTwo`, `rotacion` y
  `wipe_out`, entre otras.
- Juegos interactivos desde el celular (por ejemplo, la serpiente 3D).
- Agregar un adaptador de nivel 74AHCT125, si la señal de 3.3 V da problemas.
