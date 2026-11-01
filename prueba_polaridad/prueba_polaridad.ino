// Prueba de polaridad del cubo 9x9x9 (Arduino MEGA o UNO).
// Averigua si los anodos y los catodos encienden con 1 o con 0.
//
// Conexion (se elige sola segun la placa):
//   MEGA: pin 51 -> DATA, pin 52 -> CLOCK, pin 2 -> LATCH  (igual que el sketch original)
//   UNO:  D2     -> DATA, D3     -> CLOCK, D4    -> LATCH
//   GND del Arduino -> GND de la fuente del cubo
//
// Abre el Monitor Serie a 9600 baudios.
// 1) Polaridad: prueba las 4 combinaciones de anodos y catodos en 1 o en 0,
//    4 s cada una. Solo una deberia encender TODO el cubo: anota la letra.
//    Con la logica del sketch original deberia ser la A.
// 2) Pisos uno por uno, con INVERTIR_ANODOS e INVERTIR_CATODOS.

#if defined(__AVR_ATmega2560__)
#define DATA_PIN  51
#define CLOCK_PIN 52
#define LATCH_PIN 2
#else
#define DATA_PIN  2
#define CLOCK_PIN 3
#define LATCH_PIN 4
#endif

// Segun la letra que encendio todo el cubo:
//   A -> 0, 0    B -> 0, 1    C -> 1, 0    D -> 1, 1
#define INVERTIR_ANODOS  0
#define INVERTIR_CATODOS 0

// Envia los 12 bytes de un piso: primero U12 y al final U1, cada uno con el
// bit mas significativo primero (igual que el sketch original con SPI).
void enviar(byte u12, byte u11, byte anodos) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, u12);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, u11);
  for (int i = 0; i < 10; i++) {                  // U10 ... U1
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, anodos);
  }
  digitalWrite(LATCH_PIN, HIGH);                  // pasa los datos a las salidas
  digitalWrite(LATCH_PIN, LOW);
}

// Envia los bits tal cual. anodos: los 81 anodos en 1 o en 0.
// catodos: 9 bits, uno por piso (bit 0 = piso de abajo).
// Catodo del piso n = bit n+1 de U11:U12 (anode[] del sketch original);
// el bit 0 de U11 es el anodo del LED 81.
void enviarCrudo(bool anodos, unsigned int catodos) {
  byte u11 = ((catodos & 0x7F) << 1) | (anodos ? 0x01 : 0x00);
  byte u12 = (catodos >> 7) & 0x03;
  enviar(u12, u11, anodos ? 0xFF : 0x00);
}

// Igual que enviarCrudo, pero con la polaridad configurada arriba.
void mostrar(bool anodos, unsigned int catodos) {
  if (INVERTIR_ANODOS) anodos = !anodos;
  if (INVERTIR_CATODOS) catodos ^= 0x1FF;
  enviarCrudo(anodos, catodos);
}

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println("--- Polaridad: cual letra enciende TODO el cubo? ---");
  Serial.println("A: anodos=1 catodos=1");
  enviarCrudo(true, 0x1FF);
  delay(4000);
  Serial.println("B: anodos=1 catodos=0");
  enviarCrudo(true, 0x000);
  delay(4000);
  Serial.println("C: anodos=0 catodos=1");
  enviarCrudo(false, 0x1FF);
  delay(4000);
  Serial.println("D: anodos=0 catodos=0");
  enviarCrudo(false, 0x000);
  delay(4000);

  Serial.println("--- Pisos uno por uno, de abajo hacia arriba ---");
  for (int n = 0; n < 9; n++) {
    Serial.print("Piso ");
    Serial.println(n);
    mostrar(true, 1u << n);
    delay(1000);
  }

  Serial.println("Apagado (2 s)");
  mostrar(false, 0);
  delay(2000);
}
