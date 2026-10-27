// programa para comprobar la multiplexiòn del cubo led, se carga en automatico toda la informaciòn de la variable matriz0

#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
#define data_pin 11// used by SPI, must be pin 11
#define clock_pin 13// used by SPI, must be 13

#define AXIS_X 1
#define AXIS_Y 2
#define AXIS_Z 3
int CUBE_SIZE = 9;

unsigned long inicio; // variable para tiempo
byte anode[18];
volatile unsigned char cube[8][8];

byte matriz0[99], matriz1[99], matriz2[99], matriz3[99];

int level=0;//keeps track of which level we are shifting data to
int anodelevel=0;//this increments through the anode levels
int BAM_Bit, BAM_Counter=0;

//..............................................

char numeros[10][8] = {
 
  { 0x00,  0x3C,  0x66,  0x6E,  0x76,  0x66,  0x66,  0x3C },    // 48 : 0
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  ** ***  |
                //              |  *** **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x18,  0x18,  0x38,  0x18,  0x18,  0x18,  0x7E },    // 49 : 1
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |   ***    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |  ******  |
 
  { 0x00,  0x3C,  0x66,  0x06,  0x0C,  0x30,  0x60,  0x7E },    // 50 : 2
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |     **   |
                //              |   **     |
                //              |  **      |
                //              |  ******  |
 
  { 0x00,  0x3C,  0x66,  0x06,  0x1C,  0x06,  0x66,  0x3C },    // 51 : 3
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |    ***   |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x0C,  0x1C,  0x2C,  0x4C,  0x7E,  0x0C,  0x0C },    // 52 : 4
                //              |          |
                //              |     **   |
                //              |    ***   |
                //              |   * **   |
                //              |  *  **   |
                //              |  ******  |
                //              |     **   |
                //              |     **   |
 
  { 0x00,  0x7E,  0x60,  0x7C,  0x06,  0x06,  0x66,  0x3C },    // 53 : 5
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  *****   |
                //              |      **  |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x3C,  0x66,  0x60,  0x7C,  0x66,  0x66,  0x3C },    // 54 : 6
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x7E,  0x66,  0x0C,  0x0C,  0x18,  0x18,  0x18 },    // 55 : 7
                //              |          |
                //              |  ******  |
                //              |  **  **  |
                //              |     **   |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x3C,  0x66,  0x66,  0x3C,  0x66,  0x66,  0x3C },    // 56 : 8
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x3C,  0x66,  0x66,  0x3E,  0x06,  0x66,  0x3C },    // 57 : 9
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |

};

// ......................................................................



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

  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up
  
  SPI.begin();//start up the SPI library
  interrupts();//let the show begin, this lets the multiplexing start
  Serial.begin(9600);
}
void loop() {
     
   rotacion();
   
}

void LED(int level, int row, int column, byte intensidad){ 
  
  if(level<0)
  level=0;
  if(level>8)
  level=8;
  if(row<0)
  row=0;
  if(row>8)
  row=8;
  if(column<0)
  column=0;
  if(column>8)
  column=8;  
  if(intensidad<0)
  intensidad=0;
  if(intensidad>15)
  intensidad=15;
  
  
  int wholebyte=(level*81)+(row*9)+column;
  int whichbyte = int((wholebyte)/8);
  
    int posReg = (wholebyte-(81*level));

    bitWrite(matriz0[int((wholebyte-level)/8)+level],  posReg-(int(posReg/8)*8), bitRead(intensidad, 0));
    bitWrite(matriz1[int((wholebyte-level)/8)+level],  posReg-(int(posReg/8)*8), bitRead(intensidad, 1));
    bitWrite(matriz2[int((wholebyte-level)/8)+level],  posReg-(int(posReg/8)*8), bitRead(intensidad, 2)); 
    bitWrite(matriz3[int((wholebyte-level)/8)+level],  posReg-(int(posReg/8)*8), bitRead(intensidad, 3)); 
   
  
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
      
         SPI.transfer(anode[anodelevel*2]+matriz0[level+10]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+9;i>=level;i--){
          SPI.transfer(matriz0[i]);  
         }

    break;
  case 1:
         SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

          SPI.transfer(anode[anodelevel*2]+matriz0[level+10]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+9;i>=level;i--){
          SPI.transfer(matriz0[i]);  
         }
 
    break;
   case 2:
         SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

          SPI.transfer(anode[anodelevel*2]+matriz0[level+10]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+9;i>=level;i--){
          SPI.transfer(matriz0[i]);  
         }
  
   break;
   case 3:

        SPI.transfer(anode[anodelevel*2+1]);                // se carga principalmente el primer byte de catodos

         SPI.transfer(anode[anodelevel*2]+matriz0[level+10]);   // se carga el segundo byte de catodos màs el led 81
         
         for(int i=level+9;i>=level;i--){
          SPI.transfer(matriz0[i]);  
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

void aleatorio(){
  inicio = millis();

  while((millis()-inicio)<10000){
      LED(random(9),random(9),random(9),random(15));
  }
  
}

void rotacion(){  // probando la matriz rotacional en 2 dimensiones.
  int xr,yr;
  double pi= 3.1415;
  int matrixA[81];
   int matrixB[81];
    int matrixC[81];
     int matrixD[81];
  int matA[81];
   int matB[81];
    int matC[81];
     int matD[81];
   

   for(int k=9;k>=0;k--){

   double angle=0;
   int cuenta=0;
    for(int i=0;i<81;i++){
      matrixA[i]=0;
      matrixB[i]=0;
      matrixC[i]=0;
      matrixD[i]=0;

      matA[i]=0;
      matB[i]=0;
      matC[i]=0;
      matD[i]=0;
    }
       for(int j = 0; j<5; j++){
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){
                        LED(8-i,8-bitR,j,bitRead(numeros[k][i], bitR));
                         if (j == 4){
                            matrixA[cuenta]=8-i;
                            matrixB[cuenta]=8-bitR;
                            matrixC[cuenta]=j;
                            matrixD[cuenta]=bitRead(numeros[k][i], bitR);
                            cuenta++;
                         }

                  } 
            }
            delay(100);
             if (j!= 4){
               apagar(); 
             }
       }   
   delay(100);
   apagar();
   
   for(int j=0;j<8;j++){ //rotacion en el eje Z
     angle=angle-pi/16;
       for(int i=0;i<cuenta;i++){
        xr=0;
        yr=0;
          if(matrixD[i]==1){
            xr=(matrixB[i]-4)*cos(angle)-(matrixC[i]-4)*sin(angle);
            yr=(matrixB[i]-4)*sin(angle)+(matrixC[i]-4)*cos(angle);
            LED(matrixA[i],xr+4,yr+4,15);
          }
          matA[i]=matrixA[i];         
          matB[i]=xr+4;   
          matC[i]=yr+4;
          matD[i]=matrixD[i];
        }
  
       delay(60);
       if (j!= 7){
          apagar();
       }
   }
   
   delay(200);
   apagar();
   
   for(int j=0;j<4;j++){
      for(int i=0;i<cuenta;i++){
     
         if(matD[i]==1){
           
            LED(matA[i],3-j,matC[i],15);
           
          }
         
      }
          delay(100);
          if (j!= 3){
           apagar();
          }
   }
  delay(300);
  apagar();
  }
}

void apagar(){

  for(int x=0;x<99;x++){
    matriz0[x]=0;
    matriz1[x]=0;
    matriz2[x]=0;
    matriz3[x]=0;
  }

  
}
