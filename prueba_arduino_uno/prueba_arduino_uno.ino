// Prueba de pisos del cubo 9x9x9 con Arduino MEGA o UNO.
// Envia los datos EXACTAMENTE como la ISR de prueba_cubo_funcionLED_4.ino
// (mismo arreglo anode[], SPI y mismo orden de bytes) y por el Monitor Serie
// imprime la secuencia de 12 bytes enviada en cada paso.
//
// Conexion (igual que el sketch original, usa el SPI por hardware):
//   MEGA: pin 51 -> DATA, pin 52 -> CLOCK, pin 2 -> LATCH
//   UNO:  pin 11 -> DATA, pin 13 -> CLOCK, pin 2 -> LATCH
//   GND del Arduino -> GND de la fuente del cubo
//
// Monitor Serie a 9600 baudios.
// Orden de envio (igual que la ISR):
//   1) anode[piso*2+1]                 -> llega al ultimo registro (catodos)
//   2) anode[piso*2] + bit del LED 81  -> penultimo registro (catodos + LED 81)
//   3) 10 bytes de LEDs, del byte 9 al byte 0 -> el byte 0 queda en el registro
//      que recibe DATA

#include <SPI.h>

#define latch_pin 2

// Catodos de cada piso, copiados del sketch original
byte anode[18] = {
  B00000010, B00000000,   // piso 0
  B00000100, B00000000,   // piso 1
  B00001000, B00000000,   // piso 2
  B00010000, B00000000,   // piso 3
  B00100000, B00000000,   // piso 4
  B01000000, B00000000,   // piso 5
  B10000000, B00000000,   // piso 6
  B00000000, B00000001,   // piso 7
  B00000000, B00000010    // piso 8
};

byte secuencia[12];       // los 12 bytes en el orden en que se envian

void imprimirByte(byte b) {
  for (int i = 7; i >= 0; i--) Serial.print(bitRead(b, i));
}

// Envia un piso como la ISR: catodos de 'piso' y los 81 LEDs en 'leds'
// (1 = todos encendidos, 0 = todos apagados). piso = -1 apaga todos los pisos.
void enviarPiso(int piso, bool leds) {
  byte cat_alto = (piso >= 0) ? anode[piso * 2 + 1] : 0;
  byte cat_bajo = (piso >= 0) ? anode[piso * 2] : 0;
  byte led81 = leds ? 0x01 : 0x00;
  byte resto = leds ? 0xFF : 0x00;

  secuencia[0] = cat_alto;
  secuencia[1] = cat_bajo + led81;
  for (int i = 2; i < 12; i++) secuencia[i] = resto;

  digitalWrite(latch_pin, LOW);
  for (int i = 0; i < 12; i++) SPI.transfer(secuencia[i]);
  digitalWrite(latch_pin, HIGH);

  Serial.print("  enviado (1ro -> ultimo): ");
  for (int i = 0; i < 12; i++) {
    imprimirByte(secuencia[i]);
    Serial.print(i == 1 ? " | " : " ");
  }
  Serial.println();
}

void setup() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV16);   // 1 MHz, mas lento que el original
  pinMode(latch_pin, OUTPUT);
  SPI.begin();
  Serial.begin(9600);
  Serial.println("Formato: byte1 byte2 | 10 bytes de LEDs  (byte1 llega al ultimo registro)");
}

void loop() {
  for (int n = 0; n < 9; n++) {
    Serial.print("Piso ");
    Serial.print(n);
    Serial.println(" encendido completo");
    enviarPiso(n, true);
    delay(2000);
  }

  Serial.println("Apagado");
  enviarPiso(-1, false);
  delay(2000);
}
