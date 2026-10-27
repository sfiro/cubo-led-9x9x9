// Programa para probar el funcionamiento adecuado de los registros de desplazamiento 74HC595
//------------------------------------------------------------------------------------------------

#include <SPI.h>                                        // Librerìa para manejo de registro de desplazamiento

#define latch_pin 2                                     // Pin conocido como Latch, en el integrado es el pin 12
#define data_pin 11                                     // Pin conocido como data, en el integrado es el pin 14
#define clock_pin 13                                    // Pin conocido como Clock, en el integrado es el pin 13


void setup() {
  
  SPI.setBitOrder(MSBFIRST);                            //bit mas significante primero
  SPI.setDataMode(SPI_MODE0);                           // modo 0 en la libreria del SPI
  SPI.setClockDivider(SPI_CLOCK_DIV2);                  //corre a una velocidad de 16MHz/2 - 8MHz

  pinMode(latch_pin, OUTPUT);                           //definiciòn del Pin latch como salida
  pinMode(data_pin, OUTPUT);                            //definiciòn del Pin data como salida
  pinMode(clock_pin, OUTPUT);                           //definiciòn del Pin clock como salida
  SPI.begin();                                          //inicia la librerìa SPI

}

void loop() {
  
 SPI.transfer(B01010101);                             // transferencia de la serie de datos
 digitalWrite(latch_pin,HIGH);                        // habilitar  los datos en el puerto de salida del chip hc595
 delay(1000);                                         // esperar 1 segundo
 digitalWrite(latch_pin,LOW);                         // deshabilitar  los datos en el puerto de salida del chip hc595
 
 SPI.transfer(B10101010);                            // transferencia de la serie de datos
 digitalWrite(latch_pin,HIGH);                        // habilitar  los datos en el puerto de salida del chip hc595
 delay(1000);                                         // esperar 1 segundo
 digitalWrite(latch_pin,LOW);                         // deshabilitar  los datos en el puerto de salida del chip hc595

 

}
