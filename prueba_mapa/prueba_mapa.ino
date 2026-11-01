// Mapa del cubo 9x9x9 (Arduino MEGA o UNO): averigua que enciende cada salida
// de los 74HC595. Abre el Monitor Serie a 9600 baudios y anota que se ve en
// cada paso.
//
// Conexion (se elige sola segun la placa):
//   MEGA: pin 51 -> DATA, pin 52 -> CLOCK, pin 2 -> LATCH  (igual que el sketch original)
//   UNO:  D2     -> DATA, D3     -> CLOCK, D4    -> LATCH
//   GND del Arduino -> GND de la fuente del cubo
//
// Parte 1: todos los anodos encendidos y UN solo bit de U11/U12 en 1 por vez
//          (2 s cada uno). Segun el esquema, U11 bits 1-7 y U12 bits 0-1 son
//          los pisos 0..8: cada paso deberia encender un piso completo.
// Parte 2: todos los pisos encendidos y UN solo chip de anodos (U1..U10) en
//          0xFF por vez (2 s cada uno). Cada paso deberia encender 8 columnas.

#if defined(__AVR_ATmega2560__)
#define DATA_PIN  51
#define CLOCK_PIN 52
#define LATCH_PIN 2
#else
#define DATA_PIN  2
#define CLOCK_PIN 3
#define LATCH_PIN 4
#endif

// u[0] = U1 ... u[11] = U12. Se envia primero U12 y al final U1, cada byte con
// el bit mas significativo primero (igual que el sketch original con SPI).
byte u[12];

void enviar() {
  digitalWrite(LATCH_PIN, LOW);
  for (int i = 11; i >= 0; i--) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, u[i]);
  }
  digitalWrite(LATCH_PIN, HIGH);                  // pasa los datos a las salidas
  digitalWrite(LATCH_PIN, LOW);
}

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println();
  Serial.println("=== PARTE 1: anodos encendidos, un bit de U11/U12 por vez ===");
  for (int k = 1; k < 16; k++) {                  // el bit 0 de U11 es el anodo 81
    for (int i = 0; i < 10; i++) u[i] = 0xFF;
    unsigned int bits = 1u << k;
    u[10] = (bits & 0xFF) | 0x01;                 // U11 (+ anodo 81)
    u[11] = bits >> 8;                            // U12
    enviar();
    Serial.print(k < 8 ? "U11 bit " : "U12 bit ");
    Serial.print(k < 8 ? k : k - 8);
    if (k <= 9) {
      Serial.print("  -> deberia ser el PISO ");
      Serial.println(k - 1);
    } else {
      Serial.println("  -> sin uso, deberia estar APAGADO");
    }
    delay(2000);
  }

  Serial.println();
  Serial.println("=== PARTE 2: todos los pisos encendidos, un chip de anodos por vez ===");
  for (int c = 0; c <= 10; c++) {
    for (int i = 0; i < 10; i++) u[i] = (i == c) ? 0xFF : 0x00;
    u[10] = 0xFE | (c == 10 ? 0x01 : 0x00);       // pisos 0-6 (+ anodo 81 al final)
    u[11] = 0x03;                                 // pisos 7-8
    enviar();
    if (c < 10) {
      Serial.print("U");
      Serial.print(c + 1);
      Serial.print(" (columnas ");
      Serial.print(c * 8);
      Serial.print("-");
      Serial.print(c * 8 + 7);
      Serial.println(")  -> deberian encender 8 columnas completas");
    } else {
      Serial.println("U11 bit 0 (columna 80)  -> deberia encender 1 columna, la ultima");
    }
    delay(2000);
  }

  for (int i = 0; i < 12; i++) u[i] = 0;
  enviar();
  Serial.println("Apagado (3 s)");
  delay(3000);
}
