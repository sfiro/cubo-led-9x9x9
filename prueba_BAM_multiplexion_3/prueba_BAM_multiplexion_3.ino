// programa para comprobar la multiplexiòn del cubo led, se carga en automatico toda la informaciòn de la variable matriz0

#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
#define data_pin 11// used by SPI, must be pin 11
#define clock_pin 13// used by SPI, must be 13


byte anode[18];

byte matriz0[99], matriz1[99], matriz2[99], matriz3[99];

int level=0;//keeps track of which level we are shifting data to
int anodelevel=0;//this increments through the anode levels
int BAM_Bit, BAM_Counter=0;

void setup() {

  SPI.setBitOrder(MSBFIRST);//Most Significant Bit First
  SPI.setDataMode(SPI_MODE0);// Mode 0 Rising edge of data, keep clock low
  SPI.setClockDivider(SPI_CLOCK_DIV2);//Run the data in at 16MHz/2 - 8MHz
  
  
  noInterrupts();
  //Configuraciòn de timer 1 para la multiplexaciòn del cubo led 
  TCCR1A = B00000000;//Register A all 0's since we're not toggling any pins
  TCCR1B = B00001011;//, so 16MHz/64=250kHz
  TIMSK1 = B00000010;//bit 1 set to call the interrupt on an OCR1A match
  OCR1A=35; //  1/250kHz = 4us .  (30+1)x4us=124us
  
  anode[0]=B00000010;    // matriz de activaciòn de los catodos, niveles del cubo 
  anode[1]=B00000000;
  anode[2]=B00000100;
  anode[3]=B00000000;
  anode[4]=B00001000;
  anode[5]=B00000000;
  anode[6]=B00010000;
  anode[7]=B00000000;
  anode[8]=B00100000;
  anode[9]=B00000000;
  anode[10]=B01000000;
  anode[11]=B00000000;
  anode[12]=B10000000;
  anode[13]=B00000000;
  anode[14]=B00000000;
  anode[15]=B00000001;
  anode[16]=B00000000;
  anode[17]=B00000010;

   matriz0[0]=B00000000;    // llenado manual de la matriz a desplegar en el cubo led 
   matriz0[1]=B00000000; 
   matriz0[2]=B00000000; 
   matriz0[3]=B00000000; 
   matriz0[4]=B00000000; 
   matriz0[5]=B00000000; 
   matriz0[6]=B00000000; 
   matriz0[7]=B00000000;
   matriz0[8]=B00000000; 
   matriz0[9]=B00000000; 
  matriz0[10]=B00000001; 

   matriz1[0]=B00000000;    // llenado manual de la matriz a desplegar en el cubo led 
   matriz1[1]=B00000000; 
   matriz1[2]=B00000000; 
   matriz1[3]=B00000000; 
   matriz1[4]=B00000000; 
   matriz1[5]=B00000000; 
   matriz1[6]=B00000000; 
   matriz1[7]=B00000000;
   matriz1[8]=B00000000; 
   matriz1[9]=B00000000; 
  matriz1[10]=B00000001; 

   matriz2[0]=B00000000;    // llenado manual de la matriz a desplegar en el cubo led 
   matriz2[1]=B00000000; 
   matriz2[2]=B00000000; 
   matriz2[3]=B00000000; 
   matriz2[4]=B00000000; 
   matriz2[5]=B00000000; 
   matriz2[6]=B00000000; 
   matriz2[7]=B00000000;
   matriz2[8]=B00000000; 
   matriz2[9]=B00000000; 
  matriz2[10]=B00000001; 


   matriz3[0]=B00000000;    // llenado manual de la matriz a desplegar en el cubo led 
   matriz3[1]=B00000000; 
   matriz3[2]=B00000000; 
   matriz3[3]=B00000000; 
   matriz3[4]=B00000000; 
   matriz3[5]=B00000000; 
   matriz3[6]=B00000000; 
   matriz3[7]=B00000000;
   matriz3[8]=B00000000; 
   matriz3[9]=B00000000; 
  matriz3[10]=B00000001; 
//

//   matriz0[0]=B00000000;    // llenado manual de la matriz a desplegar en el cubo led 
//   matriz0[1]=B00010000; 
//   matriz0[2]=B00010000; 
//   matriz0[3]=B00010000; 
//   matriz0[4]=B00010000; 
//   matriz0[5]=B00010000; 
//   matriz0[6]=B00010000; 
//   matriz0[7]=B00010000;
//   matriz0[8]=B00010000; 
//   matriz0[9]=B00010000; 
//   matriz0[10]=B00010000; 
//
//   matriz0[11]=B00000001; 
//   matriz0[12]=B00000000; 
//   matriz0[13]=B00000000; 
//   matriz0[14]=B00000000; 
//   matriz0[15]=B00000000; 
//   matriz0[16]=B00000000; 
//   matriz0[17]=B10000000; 
//   matriz0[18]=B10000000;
//   matriz0[19]=B10000000; 
//   matriz0[20]=B00000000; 
//   matriz0[21]=B10000000; 
//
//   
//   matriz0[22]=B00000000; 
//   matriz0[23]=B00100000; 
//   matriz0[24]=B00100000; 
//   matriz0[25]=B00100000; 
//   matriz0[26]=B00000000; 
//   matriz0[27]=B00000100; 
//   matriz0[28]=B00000100; 
//   matriz0[29]=B00000100;
//   matriz0[30]=B00000100; 
//   matriz0[31]=B00000100; 
//   matriz0[32]=B00000100; 
//
//   matriz0[33]=B00000000; 
//   matriz0[34]=B10000000; 
//   matriz0[35]=B10000000; 
//   matriz0[36]=B10000000; 
//   matriz0[37]=B00000000; 
//   matriz0[38]=B00001000; 
//   matriz0[39]=B00001000; 
//   matriz0[40]=B00001000;
//   matriz0[41]=B00000000; 
//   matriz0[42]=B00000000; 
//   matriz0[43]=B00000000; 
//
//   matriz0[44]=B00000000; 
//   matriz0[45]=B00000000; 
//   matriz0[46]=B00000000; 
//   matriz0[47]=B00000000; 
//   matriz0[48]=B01000000; 
//   matriz0[49]=B01000000; 
//   matriz0[50]=B01000000; 
//   matriz0[51]=B01000000;
//   matriz0[52]=B01000000; 
//   matriz0[53]=B01000000; 
//   matriz0[54]=B01000000; 
//
//   matriz0[55]=B00000001; 
//   matriz0[56]=B00000001; 
//   matriz0[57]=B00000001; 
//   matriz0[58]=B00000001; 
//   matriz0[59]=B00000001; 
//   matriz0[60]=B00000001; 
//   matriz0[61]=B00000001; 
//   matriz0[62]=B00000001;
//   matriz0[63]=B00000001; 
//   matriz0[64]=B00000001; 
//   matriz0[65]=B00000001; 
//
//   matriz0[66]=B00000000; 
//   matriz0[67]=B00010000; 
//   matriz0[68]=B00010000; 
//   matriz0[69]=B00010000; 
//   matriz0[70]=B00010000; 
//   matriz0[71]=B00010000; 
//   matriz0[72]=B00010000; 
//   matriz0[73]=B00010000;
//   matriz0[74]=B00010000; 
//   matriz0[75]=B00010000; 
//   matriz0[76]=B00010000; 
//
//   matriz0[77]=B00000000; 
//   matriz0[78]=B01000000; 
//   matriz0[79]=B01000000; 
//   matriz0[80]=B01000000; 
//   matriz0[81]=B01000000; 
//   matriz0[82]=B01000000; 
//   matriz0[83]=B01000000; 
//   matriz0[84]=B01000000;
//   matriz0[85]=B01000000; 
//   matriz0[86]=B01000000; 
//   matriz0[87]=B01000000; 
//
//   matriz0[88]=B00000001; 
//   matriz0[89]=B00000001; 
//   matriz0[90]=B00000000; 
//   matriz0[91]=B00000000; 
//   matriz0[92]=B00000000; 
//   matriz0[93]=B00000000; 
//   matriz0[94]=B00000000; 
//   matriz0[95]=B00000000;
//   matriz0[96]=B00000000; 
//   matriz0[97]=B00000001; 
//   matriz0[98]=B00000001; 
 
  
  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up
  
  SPI.begin();//start up the SPI library
  interrupts();//let the show begin, this lets the multiplexing start
  Serial.begin(9600);
}

void loop() {

}

void clareado(){
    for(int i=0;i<12;i++){
      digitalWrite(latch_pin,LOW);
      SPI.transfer(B00000000);  
      digitalWrite(latch_pin,HIGH);
     }
 }

ISR(TIMER1_COMPA_vect){
  
 digitalWrite(latch_pin,LOW);

  if(BAM_Counter==8)
  BAM_Bit++;
  else
  if(BAM_Counter==24)
  BAM_Bit++;
  else
  if(BAM_Counter==56)
  BAM_Bit++;

  BAM_Counter++;//Here is where we increment the BAM counter

  switch (BAM_Bit){
  case 0:
         SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos
      
         SPI.transfer(anode[anodelevel*2]+matriz0[level]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+1;i<level+11;i++){
          SPI.transfer(matriz0[i]);  
         }

    break;
  case 1:
         SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

         SPI.transfer(anode[anodelevel*2]+matriz1[level]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+1;i<level+11;i++){
          SPI.transfer(matriz1[i]);  
         }
 
    break;
   case 2:
         SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

         SPI.transfer(anode[anodelevel*2]+matriz2[level]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+1;i<level+11;i++){
          SPI.transfer(matriz2[i]);  
         }
  
   break;
   case 3:

        SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

        SPI.transfer(anode[anodelevel*2]+matriz3[level]);   // se carga el segundo byte de catodos màs el led 81
         
        for(int i=level+1;i<level+11;i++){
          SPI.transfer(matriz3[i]);  
        }

   
   if(BAM_Counter==128){
    BAM_Counter=0;
    BAM_Bit=0;
    }
    break;
  }//switch_case

 

 
 digitalWrite(latch_pin,HIGH);


 level = level + 11;
 anodelevel++;


 if(level == 99)
 level=0;
 
 if(anodelevel == 9)
 anodelevel = 0;
 
  
}
