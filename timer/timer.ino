#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
#define data_pin 11// used by SPI, must be pin 11
#define clock_pin 13// used by SPI, must be 13

int shift_out;
byte anode[8];

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
  
  anode[0]=B00000001;
  anode[1]=B00000010;
  anode[2]=B00000100;
  anode[3]=B00001000;
  anode[4]=B00010000;
  anode[5]=B00100000;
  anode[6]=B01000000;
  anode[7]=B10000000;
  
  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up
  SPI.begin();//start up the SPI library
  interrupts();//let the show begin, this lets the multiplexing start
}

void loop() {
  // put your main code here, to run repeatedly:

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

  switch (BAM_Bit){
  case 0:
   for(shift_out=level; shift_out<level+8; shift_out++)
   SPI.transfer(matriz0[shift_out]);
  
    break;
  case 1:
   for(shift_out=level; shift_out<level+8; shift_out++)
   SPI.transfer(matriz1[shift_out]);
 
    break;
   case 2:
   for(shift_out=level; shift_out<level+8; shift_out++)
   SPI.transfer(matriz2[shift_out]);
  
   break;
   case 3:
   for(shift_out=level; shift_out<level+8; shift_out++)
   SPI.transfer(matriz3[shift_out]);
   
   if(BAM_Counter==64){
    BAM_Counter=0;
    BAM_Bit=0;
    }
    break;
  }//switch_case

 
}
