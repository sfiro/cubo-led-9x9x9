#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
#define data_pin 11// used by SPI, must be pin 11
#define clock_pin 13// used by SPI, must be 13

int shift_out;
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
  OCR1A=30; //  1/250kHz = 4us .  (30+1)x4us=124us
  
  anode[0]=B00000010;
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
  
  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up
  SPI.begin();//start up the SPI library
  interrupts();//let the show begin, this lets the multiplexing start
}

void loop() {
  // put your main code here, to run repeatedly:
matriz0[0]=B00100001;
matriz1[0]=B00100001;
matriz2[0]=B00100001;
matriz3[0]=B00100001;
}

ISR(TIMER1_COMPA_vect){

  PORTD |= 1<<blank_pin;
  
  if(BAM_Counter==8)
  BAM_Bit++;
  else
  if(BAM_Counter==16)
  BAM_Bit++;
  else
  if(BAM_Counter==32)
  BAM_Bit++;

  BAM_Counter++;//Here is where we increment the BAM counter

  SPI.transfer(anode[anodelevel+1]);//finally, send out the anode level byte
  
  switch (BAM_Bit){
  case 0:
   SPI.transfer(anode[anodelevel]+matriz0[level]);//finally, send out the anode level byte
   for(shift_out=level+1; shift_out<level+11; shift_out++)
   SPI.transfer(matriz0[shift_out]);

    break;
  case 1:
   SPI.transfer(anode[anodelevel]+matriz1[level]);//finally, send out the anode level byte
   for(shift_out=level+1; shift_out<level+11; shift_out++)
   SPI.transfer(matriz1[shift_out]);
 
    break;
   case 2:
   SPI.transfer(anode[anodelevel]+matriz2[level]);//finally, send out the anode level byte
   for(shift_out=level+1; shift_out<level+11; shift_out++)
   SPI.transfer(matriz2[shift_out]);
  
   break;
   case 3:
   SPI.transfer(anode[anodelevel]+matriz3[level]);//finally, send out the anode level byte
   for(shift_out=level+1; shift_out<level+11; shift_out++)
   SPI.transfer(matriz3[shift_out]);
   
   if(BAM_Counter==64){
    BAM_Counter=0;
    BAM_Bit=0;
    }
    break;
  }//switch_case

PORTD |= 1<<latch_pin;//Latch pin HIGH
delay(50);
PORTD &= 0<<latch_pin;//Latch pin LOW
PORTD &= 0<<blank_pin;//Blank pin LOW to turn on the LEDs with the new data

anodelevel=anodelevel+2;//inrement the anode level
level = level+11;//increment the level variable by 8, which is used to shift out data, since the next level woudl be the next 8 bytes in the arrays

if(anodelevel==18)//go back to 0 if max is reached
anodelevel=0;
if(level==99)//if you hit 64 on level, this means you just sent out all 63 bytes, so go back
level=0;
 
}
