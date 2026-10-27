// este programa sirve para probar los datos enviados a los registros de desplazamiento. 
// la variable matriz son los datos que llenan los registros de desplazamiento para desplegar los datos 

#include <SPI.h>// SPI Library used to clock data out to the shift registers


#define latch_pin 2// pin 2 para el LATCH del registro 
#define blank_pin 4// pin 4 para el blanck del registro, recordar colocar la resistencia de pullup 1k to 5V
#define data_pin 11// pin 11
#define clock_pin 13// pin 13
byte anode[18];
byte matriz0[99], matriz1[99], matriz2[99], matriz3[99];


void setup() {

   SPI.setBitOrder(MSBFIRST);//Most Significant Bit First
  SPI.setDataMode(SPI_MODE0);// Mode 0 Rising edge of data, keep clock low
  SPI.setClockDivider(SPI_CLOCK_DIV2);//Run the data in at 16MHz/2 - 8MHz
  
  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up

//  anode[0]=B00000001;
//  anode[1]=B00000000;
//  anode[2]=B00000000;
//  anode[3]=B00000000;
//  anode[4]=B00000000;
//  anode[5]=B00000000;
//  anode[6]=B00000000;
//  anode[7]=B00100000;
//  anode[8]=B00000000;
//  anode[9]=B00000000;
//  anode[10]=B00000000;
//  anode[11]=B00000000;
//  anode[12]=B00000000;
//  anode[13]=B00000000;
//  anode[14]=B00000000;
//  anode[15]=B00000000;
//  anode[16]=B00000000;
//  anode[17]=B00000000;

   matriz0[0]=B00000010; 
   matriz0[1]=B00000000; 
   matriz0[2]=B00000000; 
   matriz0[3]=B00000000; 
   matriz0[4]=B00000000; 
   matriz0[5]=B00000000; 
   matriz0[6]=B00000000; 
   matriz0[7]=B00000000;
   matriz0[8]=B00000000; 
   matriz0[9]=B00000000; 
   matriz0[10]=B00000000; 
   matriz0[11]=B00000000;
 

  
  SPI.begin();//start up the SPI library
 
}

void loop() {
 for(int i=0;i<12;i++){
  digitalWrite(latch_pin,LOW);
  SPI.transfer(B00000000);  
  digitalWrite(latch_pin,HIGH);
  
 }

 digitalWrite(latch_pin,LOW);
 for(int i=0;i<12;i++){
  SPI.transfer(matriz0[i]);  
 }
  digitalWrite(latch_pin,HIGH);
 delay(2000);
  

}
