// programa para comprobar la multiplexiòn del cubo led, se carga en automatico toda la informaciòn de la variable matriz0

#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
// en arduino uno
//#define data_pin 11// used by SPI, must be pin 11
//#define clock_pin 13// used by SPI, must be 13

// en arduino mega
#define data_pin 51// used by SPI, must be pin 11
#define clock_pin 52// used by SPI, must be 13

#define AXIS_X 1
#define AXIS_Y 2
#define AXIS_Z 3
int CUBE_SIZE = 9;

unsigned long inicio; // variable para tiempo
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
  
  cubo();

  fireworks();

  rainVersionTwo();

  flash(); 

  disparos();

  aleatorioEncender();

  aleatorioApagar();

  lluvia();

  folder();
  
  apagar();
  bouncyvTwo();

  disparosDobles();

  aleatorio();

  letras();

  sinwaveTwo();

  planos();
  
  disparosTriples();

  wipe_out();

  sinelines (1000, 5);

  effect_boxside_randsend_parallel (AXIS_X, 0, 20, 1);
  effect_boxside_randsend_parallel (AXIS_X, 1, 20, 1);
  effect_boxside_randsend_parallel (AXIS_Y, 0, 20, 1);
  effect_boxside_randsend_parallel (AXIS_Y, 1, 20, 1);
  effect_boxside_randsend_parallel (AXIS_Z, 0, 20, 1);
  effect_boxside_randsend_parallel (AXIS_Z, 1, 20, 1);
    
  effect_axis_updown_randsuspend(AXIS_Z, 50,1000,0);
  effect_axis_updown_randsuspend(AXIS_Z, 50,1000,1);
  effect_axis_updown_randsuspend(AXIS_Z, 50,1000,0);
  effect_axis_updown_randsuspend(AXIS_Z, 50,1000,1);
  effect_axis_updown_randsuspend(AXIS_X, 50,1000,0);
  effect_axis_updown_randsuspend(AXIS_X, 50,1000,1);
  effect_axis_updown_randsuspend(AXIS_Y, 50,1000,0);
  effect_axis_updown_randsuspend(AXIS_Y, 50,1000,1);
    
  linespin(1000,5);

    //lluvia2();

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

void aleatorioEncender(){
  inicio = millis();

  while((millis()-inicio)<5000){
      LED(random(9),random(9),random(9),15);
      delay(5);
  }
  
}

void aleatorioApagar(){
  inicio = millis();

  while((millis()-inicio)<10000){
      LED(random(9),random(9),random(9),0);
      delay(5);
  }
  
}


void encender(){

  for(int x=0;x<9;x++){
    for(int y=0;y<9;y++){
      for(int z=0;z<9;z++){
        LED(z,y,x,15);
      }
    }
  }
  
}

void disparos(){
  apagar();
  inicio = millis();
  while(millis()-inicio< 3000){
      int y =random(9);
      int z =random(9);
      for(int x=0;x<9;x++){
        LED(z,y,x,15);
        delay(30);
      }
      for(int x=0;x<9;x++){
        LED(z,y,x,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int z =random(9);
      for(int y=0;y<9;y++){
        LED(z,y,x,15);
        delay(30);
      }
      for(int y=0;y<9;y++){
        LED(z,y,x,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int y =random(9);
      for(int z=0;z<9;z++){
        LED(z,y,x,15);
        delay(30);
      }
      for(int z=0;z<9;z++){
        LED(z,y,x,0);
        delay(30);
      }
  }
  
}

void disparosDobles(){
  apagar();
  inicio = millis();
  while(millis()-inicio< 3000){
      int y =random(9);
      int z =random(9);
      for(int x=0;x<9;x++){
        LED(z,y,x,15);
        LED(x,y,z,15);
        delay(30);
      }
      for(int x=0;x<9;x++){
        LED(z,y,x,0);
        LED(x,y,z,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int z =random(9);
      for(int y=0;y<9;y++){
        LED(z,y,x,15);
        LED(z,x,y,15);
        delay(30);
      }
      for(int y=0;y<9;y++){
        LED(z,y,x,0);
        LED(z,x,y,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int y =random(9);
      for(int z=0;z<9;z++){
        LED(z,y,x,15);
        LED(y,z,x,15);
        delay(30);
      }
      for(int z=0;z<9;z++){
        LED(z,y,x,0);
        LED(y,z,x,0);
        delay(30);
      }
  }
  
}

void disparosTriples(){
  apagar();
  inicio = millis();
  while(millis()-inicio< 3000){
      int y =random(9);
      int z =random(9);
      for(int x=0;x<9;x++){
        LED(z,y,x,15);
        LED(x,y,z,15);
        LED(y,x,z,15);
        delay(30);
      }
      for(int x=0;x<9;x++){
        LED(z,y,x,0);
        LED(x,y,z,0);
        LED(y,x,z,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int z =random(9);
      for(int y=0;y<9;y++){
        LED(z,y,x,15);
        LED(z,x,y,15);
        LED(y,x,z,15);
        delay(30);
      }
      for(int y=0;y<9;y++){
        LED(z,y,x,0);
        LED(z,x,y,0);
        LED(y,x,z,0);
        delay(30);
      }
  }
  inicio = millis();
  while(millis()-inicio< 3000){
      int x =random(9);
      int y =random(9);
      for(int z=0;z<9;z++){
        LED(z,y,x,15);
        LED(y,z,x,15);
        LED(y,x,z,15);
        delay(30);
      }
      for(int z=0;z<9;z++){
        LED(z,y,x,0);
        LED(y,z,x,0);
        LED(y,x,z,0);
        delay(30);
      }
  }
  
}

void planos(){
  apagar();
  inicio = millis();
  while(millis()-inicio< 5000){
     for(int z=0;z<9;z++){
        for(int y=0;y<9;y++){
          for(int x=0;x<9;x++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int y=0;y<9;y++){
          for(int x=0;x<9;x++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int z=8;z>=0;z--){
        for(int y=0;y<9;y++){
          for(int x=0;x<9;x++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int y=0;y<9;y++){
          for(int x=0;x<9;x++){
            LED(z,y,x,0);
          }
        }
     }
  }
  inicio = millis();
  while(millis()-inicio< 5000){
     for(int y=0;y<9;y++){
        for(int z=0;z<9;z++){
          for(int x=0;x<9;x++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int z=0;z<9;z++){
          for(int x=0;x<9;x++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int y=8;y>=0;y--){
        for(int z=0;z<9;z++){
          for(int x=0;x<9;x++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int z=0;z<9;z++){
          for(int x=0;x<9;x++){
            LED(z,y,x,0);
          }
        }
     }
  }
  inicio = millis();
  while(millis()-inicio< 5000){
     for(int x=0;x<9;x++){
        for(int y=0;y<9;y++){
          for(int z=0;z<9;z++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int y=0;y<9;y++){
          for(int z=0;z<9;z++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int x=8;x>=0;x--){
        for(int y=0;y<9;y++){
          for(int z=0;z<9;z++){
            LED(z,y,x,15);
          }
        }
        delay(50);
        for(int y=0;y<9;y++){
          for(int z=0;z<9;z++){
            LED(z,y,x,0);
          }
        }
     }
  }
  
}

void flash(){

  apagar();
  delay(100);
  encender();
  delay(100);
  apagar();
  delay(100);
  encender();
  delay(100);
  apagar();
  delay(100);
  encender();
  delay(100);

}

void lluvia(){
  apagar();
 inicio = millis();
 while(millis()-inicio< 5000){
     int vector[1][6];
      for(int i=0;i<6;i++){
          int x = random(9);
          int y = random(9);
          vector[0][i]=x;
          vector[1][i]=y;
       }
    
      for(int z=8;z>=0;z--){
       for(int i=0;i<6;i++){
          LED(z,vector[0][i],vector[1][i],15);
       }
       delay(30);
       for(int i=0;i<6;i++){
          LED(z,vector[0][i],vector[1][i],0);
       }
       
      }
 }
}

void lluvia2(){
  apagar();
 int vector[1][18];
  for(int i=0;i<3;i++){
      int x = random(9);
      int y = random(9);
      vector[0][i]=x;
      vector[1][i]=y;
   }

  for(int z=8;z>=0;z--){
     for(int i=0;i<3;i++){
        LED(z,vector[0][i],vector[1][i],15);
     }
     delay(70);
     for(int i=0;i<3;i++){
        LED(z,vector[0][i],vector[1][i],0);
     }
   
  }
  
}

void cubo(){
  apagar();
  int tiempo = 100;
  for(int x=0;x<9;x++){
    LED(0,0,x,15);
    LED(0,x,0,15);
    LED(x,0,0,15);
    delay(tiempo);
  }
  for(int x=0;x<9;x++){
    LED(x,0,8,15);
    LED(0,x,8,15);
    LED(0,8,x,15);
    LED(x,8,0,15);
    LED(8,x,0,15);
    LED(8,0,x,15);
    delay(tiempo);
  }
  for(int x=0;x<9;x++){
    LED(8,8,x,15);
    LED(x,8,8,15);
    LED(8,x,8,15);
    delay(tiempo);
  }
  delay(500);
  for(int x=0;x<9;x++){
    LED(0,0,x,0);
    LED(0,x,0,0);
    LED(x,0,0,0);
    delay(tiempo);
  }
  for(int x=0;x<9;x++){
    LED(x,0,8,0);
    LED(0,x,8,0);
    LED(0,8,x,0);
    LED(x,8,0,0);
    LED(8,x,0,0);
    LED(8,0,x,0);
    delay(tiempo);
  }
  for(int x=0;x<9;x++){
    LED(8,8,x,0);
    LED(x,8,8,0);
    LED(8,x,8,0);
    delay(tiempo);
  }
}

void sinwaveTwo(){//*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo
    int sinewavearray[9], addr, sinemult[9], colselect, intensidad=15, addrt;
    int sinewavearrayOLD[9], select, subZ=-8, subT=8, multi=1;//random(-1, 2);
  
   sinewavearray[0]=0;
   sinewavearray[1]=1;
   sinewavearray[2]=2;
   sinewavearray[3]=3;
   sinewavearray[4]=4;
   sinewavearray[5]=5;
   sinewavearray[6]=6;
   sinewavearray[7]=7;
   sinewavearray[8]=8;
   
   sinemult[0]=1;
   sinemult[1]=1; 
   sinemult[2]=1;
   sinemult[3]=1;
   sinemult[4]=1;
   sinemult[5]=1;
   sinemult[6]=1;
   sinemult[7]=1;
   sinemult[8]=1;
  
      inicio=millis();
      
  while(millis()-inicio<15000){
    
  for(addr=0; addr<9; addr++){
      if(sinewavearray[addr]==8){
        sinemult[addr]=-1;
      }
      if(sinewavearray[addr]==0){
        sinemult[addr]=1;     
      }
      sinewavearray[addr] = sinewavearray[addr] + sinemult[addr];
   }


  for(addr=0; addr<9; addr++){
    
    LED(sinewavearrayOLD[addr], addr, 0, 0);
    LED(sinewavearrayOLD[addr], 0, addr, 0);
    LED(sinewavearrayOLD[addr], subT-addr, 8, 0);
    LED(sinewavearrayOLD[addr], 8, subT-addr, 0);
         
    LED(sinewavearray[addr], addr, 0, intensidad);
    LED(sinewavearray[addr], 0, addr, intensidad);
    LED(sinewavearray[addr], subT-addr,8, intensidad);
    LED(sinewavearray[addr], 8, subT-addr, intensidad);
  }
    
  for(addr=1; addr<8; addr++){   
     LED(sinewavearrayOLD[addr+multi*1], addr, 1, 0);
     LED(sinewavearrayOLD[addr+multi*1], 1, addr, 0);
     LED(sinewavearrayOLD[addr+multi*1], subT-addr, 7, 0);
     LED(sinewavearrayOLD[addr+multi*1], 7, subT-addr, 0);  
      
     LED(sinewavearray[addr+multi*1], addr, 1, intensidad);
     LED(sinewavearray[addr+multi*1], 1, addr, intensidad);
     LED(sinewavearray[addr+multi*1], subT-addr,7, intensidad);
     LED(sinewavearray[addr+multi*1], 7, subT-addr, intensidad);
  }
 
  for(addr=2; addr<7; addr++){   
      LED(sinewavearrayOLD[addr+multi*2], addr, 2, 0);
      LED(sinewavearrayOLD[addr+multi*2], 2, addr, 0);
      LED(sinewavearrayOLD[addr+multi*2], subT-addr, 6, 0);
      LED(sinewavearrayOLD[addr+multi*2], 6, subT-addr, 0);  
      
      LED(sinewavearray[addr+multi*2], addr, 2, intensidad);
      LED(sinewavearray[addr+multi*2], 2, addr, intensidad);
      LED(sinewavearray[addr+multi*2], subT-addr,6, intensidad);
      LED(sinewavearray[addr+multi*2], 6, subT-addr, intensidad);
   }  
   
  for(addr=3; addr<6; addr++){   
      LED(sinewavearrayOLD[addr+multi*3], addr, 3, 0);
      LED(sinewavearrayOLD[addr+multi*3], 3, addr, 0);
      LED(sinewavearrayOLD[addr+multi*3], subT-addr, 5, 0);
      LED(sinewavearrayOLD[addr+multi*3], 5, subT-addr, 0);  
      
      LED(sinewavearray[addr+multi*3], addr, 3,intensidad);
      LED(sinewavearray[addr+multi*3], 3, addr, intensidad);
      LED(sinewavearray[addr+multi*3], subT-addr,5, intensidad);
      LED(sinewavearray[addr+multi*3], 5, subT-addr, intensidad);
   }      

   for(addr=4; addr<5; addr++){   
      LED(sinewavearrayOLD[addr+multi*3], addr, 4, 0);
      LED(sinewavearrayOLD[addr+multi*3], 4, addr, 0);
      LED(sinewavearrayOLD[addr+multi*3], subT-addr, 4, 0);
      LED(sinewavearrayOLD[addr+multi*3], 4, subT-addr, 0);  
      
      LED(sinewavearray[addr+multi*3], addr, 4,intensidad);
      LED(sinewavearray[addr+multi*3], 4, addr, intensidad);
      LED(sinewavearray[addr+multi*3], subT-addr,4, intensidad);
      LED(sinewavearray[addr+multi*3], 4, subT-addr, intensidad);
   }    
   
     for(addr=0; addr<9; addr++)
         sinewavearrayOLD[addr]=sinewavearray[addr];
         
    delay(30);

    
    
  }//while
  
  
}//SinewaveTwo


void bouncyvTwo(){//****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo
  int  select;
  int oldx[50], oldy[50], oldz[50];
  int x[50], y[50], z[50], addr, ledcount=20, direct, direcTwo;
  int xbit=1, ybit=1, zbit=1;
   for(addr=0; addr<ledcount+1; addr++){
     oldx[addr]=0;
     oldy[addr]=0;
     oldz[addr]=0;
     x[addr]=0;
     y[addr]=0;
     z[addr]=0;
   }
  apagar();
      inicio=millis();
      
  while(millis()-inicio<20000){
    
      direct = random(3);

      for(addr=1; addr<ledcount+1; addr++){
        LED(oldx[addr], oldy[addr],oldz[addr], 0);
        LED(x[addr], y[addr], z[addr], 15);
      }
      
      for(addr=1; addr<ledcount+1; addr++){
        oldx[addr]=x[addr];
        oldy[addr]=y[addr];
        oldz[addr]=z[addr];
      }

      delay(20);


      if(direct==0)
      x[0]= x[0]+xbit;
      if(direct==1)
      y[0]= y[0]+ybit;
      if(direct==2)
      z[0]= z[0]+zbit;
      
      if(x[0]>8){
            xbit=-1;
            x[0]=8;
      }
      if(x[0]<0){
          xbit=1;
          x[0]=0;
      }
      if(y[0]>8){
          ybit=-1;
          y[0]=8;
      }
      if(y[0]<0){
          ybit=1;
          y[0]=0;
      }
      if(z[0]>8){
          zbit=-1;
          z[0]=8;
      }
      if(z[0]<0){
          zbit=1;
          z[0]=0;
      }

    for(addr=ledcount; addr>0; addr--){
        x[addr]=x[addr-1];
        y[addr]=y[addr-1];
        z[addr]=z[addr-1];
    }


  }//while
  
}//bouncyv2

void rainVersionTwo(){//****rainVersionTwo****rainVersionTwo****rainVersionTwo****rainVersionTwo****rainVersionTwo
  int x[64], y[64], z[64], addr, leds=64;
  int  xold[64], yold[64], zold[64];
  
//  for(addr=0; addr<64; addr++){
//    x[addr]=random(9);
//    y[addr]=random(9);
//    z[addr]=random(9);  
//  }
  for(addr=0; addr<64; addr++){  // inicia cayendo la lluvia
    x[addr]=0;
    y[addr]=0;
    z[addr]=0;  
  }
  inicio=millis();
  while(millis()-inicio<20000){

   for(int addr=0; addr<leds; addr++){
      LED(zold[addr], xold[addr], yold[addr], 0);
      if(z[addr]<0){
          LED(z[addr], x[addr], y[addr], 0);  
      } else{
          LED(z[addr], x[addr], y[addr], 15);
      }
      
      
   }
  
   for(addr=0; addr<leds; addr++){
      xold[addr]=x[addr];
      yold[addr]=y[addr];
      zold[addr]=z[addr];
   } 
  
  
  delay(15);

//  for(int yy=0; yy<9; yy++){ 
//     for(int xx=0; xx<9; xx++){
//        LED(0, xx, yy, 0);
//      }
//  }

   for(addr=0; addr<leds; addr++){
      
      z[addr] = z[addr]-1;
      
     //x[addr] = x[addr]+1;
     // y[addr] = y[addr]+1;
      if(z[addr]< random(-100,0) ){
            x[addr]=random(9);
            y[addr]=random(9);    
            z[addr]=8; 
       
      }//-check
  }//add

  
}//while
 
}//rainv2

void folder(){//****folder****folder****folder****folder****folder****folder****folder****folder****folder
  int xx, yy, zz, pullback[16], state=0, backorfront=7;//backorfront 7 for back 0 for front
  
  int folderaddr[16], LED_Old[16], oldpullback[16], ranx=15, ranselect;
  int bot=0, top=1, right=0, left=0, back=0, front=0, side=0, side_select;
  
  folderaddr[0]=-8;
  folderaddr[1]=-7;
  folderaddr[2]=-6;
  folderaddr[3]=-5;
  folderaddr[4]=-4;
  folderaddr[5]=-3;
  folderaddr[6]=-2;
  folderaddr[7]=-1;
  folderaddr[8]=0;
  
  for(xx=0; xx<8; xx++){
  oldpullback[xx]=0;
  pullback[xx]=0;
  }
  
  
  
  inicio=millis();
  while(millis()-inicio<10000){ 
    if(top==1){
          if(side==0){
               //top to left-side
              for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(8-LED_Old[yy], yy-oldpullback[yy],xx , 0);
                    LED(8-folderaddr[yy], yy-pullback[yy],xx , ranx);
                }
              }
           }
        if(side==2){
          //top to back-side
              for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(8-LED_Old[yy], xx, yy-oldpullback[yy], 0);
                    LED(8-folderaddr[yy], xx, yy-pullback[yy], ranx);
                  }
              }
         }
        if(side==3){
            //top-side to front-side
              for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(8-LED_Old[7-yy], xx, yy+oldpullback[yy], 0);
                    LED(8-folderaddr[7-yy], xx, yy+pullback[yy], ranx);
                }
              }
         }
         
        if(side==1){
        //top-side to right
             for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(8-LED_Old[8-yy], yy+oldpullback[yy],xx , 0);
                    LED(8-folderaddr[8-yy], yy+pullback[yy],xx , ranx);
                }
              }
        }
    }//top
    
    if(right==1){
            if(side==4){
             //right-side to top
                 for(yy=0; yy<9; yy++){
                    for(xx=0; xx<9; xx++){
                    LED(yy+oldpullback[8-yy],8-LED_Old[8-yy],xx , 0);
                    LED( yy+pullback[8-yy],8-folderaddr[8-yy],xx , ranx);
                    }
                 }
            }
        if(side==3){
          //right-side to front-side
           for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(xx, 8-LED_Old[8-yy],yy+oldpullback[yy], 0);
                    LED(xx,8-folderaddr[8-yy], yy+pullback[yy], ranx);
                }  
           }
        }
        if(side==2){
           //right-side to back-side
           for(yy=0; yy<9; yy++){
              for(xx=0; xx<9; xx++){
                  LED(xx, 8-LED_Old[yy],yy-oldpullback[yy], 0);
                  LED(xx,8-folderaddr[yy], yy-pullback[yy], ranx);
              }
            }
        }
        if(side==5){
        //right-side to bottom
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(yy-oldpullback[yy],8-LED_Old[yy],xx , 0);
                    LED( yy-pullback[yy],8-folderaddr[yy],xx , ranx);
                }
            }
        }
    }//right
    
     if(left==1){
             if(side==4){
                 //left-side to top
                for(yy=0; yy<9; yy++){
                    for(xx=0; xx<9; xx++){
                        LED(yy+oldpullback[yy],LED_Old[8-yy],xx , 0);
                        LED( yy+pullback[yy],folderaddr[8-yy],xx , ranx);
                    }
                }
             }
            if(side==3){
               //left-side to front-side
                 for(yy=0; yy<9; yy++){
                    for(xx=0; xx<9; xx++){
                        LED(xx, LED_Old[8-yy],yy+oldpullback[yy], 0);
                        LED(xx,folderaddr[8-yy], yy+pullback[yy], ranx);
                    }
                 }
            }
            if(side==2){
              //left-side to back-side
                for(yy=0; yy<9; yy++){
                    for(xx=0; xx<9; xx++){
                        LED(xx, LED_Old[yy],yy-oldpullback[yy], 0);
                        LED(xx,folderaddr[yy], yy-pullback[yy], ranx);
                    }
                }
            }
            if(side==5){
                //left-side to bottom
                 for(yy=0; yy<9; yy++){
                      for(xx=0; xx<9; xx++){
                          LED(yy-oldpullback[yy],LED_Old[yy],xx , 0);
                          LED( yy-pullback[yy],folderaddr[yy],xx , ranx);
                      }
                 }
            }
      }//left
  
  
   if(back==1){
      if(side==1){
          //back-side to right-side
          for(yy=0; yy<9; yy++){
              for(xx=0; xx<9; xx++){
                  LED(xx, yy+oldpullback[yy],LED_Old[8-yy], 0);
                  LED(xx, yy+pullback[yy],folderaddr[8-yy], ranx);
              }
          }
      }
      if(side==4){
          // back-side to top-side
          for(yy=0; yy<9; yy++){
              for(xx=0; xx<9; xx++){
                  LED(yy+oldpullback[yy],xx,LED_Old[8-yy] , 0);
                  LED( yy+pullback[yy],xx,folderaddr[8-yy] , ranx);
              }
          }
      }
      if(side==5){
          // back-side to bottom
          for(yy=0; yy<9; yy++){
              for(xx=0; xx<9; xx++){
                  LED(yy-oldpullback[yy],xx,LED_Old[yy] , 0);
                  LED( yy-pullback[yy],xx,folderaddr[yy] , ranx);
              }
          }
      }//state1
     if(side==0){
        //back-side to left-side
          for(yy=0; yy<8; yy++){
              for(xx=0; xx<8; xx++){
                  LED(xx, yy-oldpullback[yy],LED_Old[yy], 0);
                  LED(xx, yy-pullback[yy],folderaddr[yy], ranx);
              }
          }
     }
   }//back

   
  if(bot==1){
        if(side==1){
          // bottom-side to right-side
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(LED_Old[8-yy], yy+oldpullback[yy],xx , 0);
                    LED(folderaddr[8-yy], yy+pullback[yy],xx , ranx);
                }
            }
        }
        if(side==3){
              //bottom to front-side
              for(yy=0; yy<9; yy++){
                  for(xx=0; xx<9; xx++){
                      LED(LED_Old[8-yy], xx, yy+oldpullback[yy], 0);
                      LED(folderaddr[8-yy], xx, yy+pullback[yy], ranx);
                  }
               }
         }
      if(side==2){
        //bottom to back-side
        for(yy=0; yy<9; yy++){
            for(xx=0; xx<9; xx++){
                LED(LED_Old[yy], xx, yy-oldpullback[yy], 0);
                LED(folderaddr[yy], xx, yy-pullback[yy], ranx);
            }
        }
      }
      if(side==0){
        //bottom to left-side
        for(yy=0; yy<9; yy++){
            for(xx=0; xx<9; xx++){
                LED(LED_Old[yy], yy-oldpullback[yy],xx , 0);
                LED(folderaddr[yy], yy-pullback[yy],xx , ranx);
            }
        }
     }
   }//bot
   
  if(front==1){
         if(side==0){
              //front-side to left-side
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(xx, yy-oldpullback[yy],8-LED_Old[yy], 0);
                    LED(xx, yy-pullback[yy],8-folderaddr[yy], ranx);
                }
             }
        }
        if(side==5){
            // front-side to bottom
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(yy-oldpullback[yy],xx,8-LED_Old[yy] , 0);
                    LED( yy-pullback[yy],xx,8-folderaddr[yy] , ranx);
                }
            }
        }
        if(side==4){
            // front-side to top-side
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(yy+oldpullback[yy],xx,8-LED_Old[8-yy] , 0);
                    LED( yy+pullback[yy],xx,8-folderaddr[8-yy] , ranx);
                }
            }
        }
        
        if(side==1){
            //front-side to right-side
            for(yy=0; yy<9; yy++){
                for(xx=0; xx<9; xx++){
                    LED(xx, yy+oldpullback[yy],8-LED_Old[8-yy], 0);
                    LED(xx, yy+pullback[yy],8-folderaddr[8-yy], ranx);
                }
            }
        }
    }//front

  


  delay(5);//               DELAY   DELAY  DELAY
  for(xx=0; xx<9; xx++){
      LED_Old[xx]=folderaddr[xx];
      oldpullback[xx]=pullback[xx];
  }
  

  if(folderaddr[8]==8){
   // pullback=8;
    for(zz=0; zz<9; zz++)
       pullback[zz] = pullback[zz]+1;
 
   if(pullback[8]==9){//finished with fold
   delay(10);
     //state++;
     //if(state==4)
     //state=0;    
     
     side_select=random(3);
     
     if(top==1){//                 TOP
        top=0; 
     if(side==0){//top to left
        left=1;
        if(side_select==0) side=2;
        if(side_select==1) side=3;
        //if(side_select==2) side=4;
        if(side_select==2) side=5;
     } else    
     if(side==1){//top to right
        right=1;
        if(side_select==0) side=5;
        if(side_select==1) side=2;
        if(side_select==2) side=3;
        //if(side_select==3) side=4;
     } else  
     if(side==2){//top to back
        back=1;
        if(side_select==0) side=0;
        if(side_select==1) side=1;
        if(side_select==2) side=5;
        //if(side_select==3) side=4;
     } else      
     if(side==3){//top to front
        front=1;
        if(side_select==0) side=0;
        if(side_select==1) side=1;
        if(side_select==2) side=5;
        //if(side_select==3) side=4;
     }   
     } else//top
     if(bot==1){//                 BOTTOM
         bot=0; 
         if(side==0){//bot to left
            left=1;
            if(side_select==0) side=2;
            if(side_select==1) side=3;
            if(side_select==2) side=4;
            //if(side_select==3) side=5;
         } else    
         if(side==1){//bot to right
            right=1;
            //if(side_select==0) side=5;
            if(side_select==0) side=2;
            if(side_select==1) side=3;
            if(side_select==2) side=4;
         } else  
         if(side==2){//bot to back
            back=1;
            if(side_select==0) side=0;
            if(side_select==1) side=1;
            //if(side_select==2) side=5;
            if(side_select==2) side=4;
          } else      
         if(side==3){//bot to front
            front=1;
            if(side_select==0) side=0;
            if(side_select==1) side=1;
            //if(side_select==2) side=5;
            if(side_select==2) side=4;
          }   
     } else//bot
     if(right==1){//                 RIGHT
         right=0; 
         if(side==4){//right to top
              top=1;
              if(side_select==0) side=2;
              if(side_select==1) side=3;
              if(side_select==2) side=0;
              //if(side_select==3) side=1;
         } else    
         if(side==5){//right to bot
              bot=1;
              if(side_select==0) side=0;
              if(side_select==1) side=2;
              if(side_select==2) side=3;
              //if(side_select==3) side=1;
          } 
          else  
         if(side==2){//right to back
          back=1;
              if(side_select==0) side=0;
              //if(side_select==1) side=1;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          } else      
          if(side==3){//right to front
              front=1;
              if(side_select==0) side=0;
              //if(side_select==1) side=1;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          }   
     } else//bot
     if(left==1){//                 LEFT
         left=0; 
         if(side==4){//left to top
              top=1;
              //if(side_select==0) side=2;
              if(side_select==0) side=3;
              if(side_select==1) side=2;
              if(side_select==2) side=1;
         } else    
         if(side==5){//left to bot
              bot=1;
              //if(side_select==0) side=0;
              if(side_select==0) side=2;
              if(side_select==1) side=3;
              if(side_select==2) side=1;
         } else  
         if(side==2){//left to back
              back=1;
              //if(side_select==0) side=0;
              if(side_select==0) side=1;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          } else      
          if(side==3){//left to front
              front=1;
              //if(side_select==0) side=0;
              if(side_select==0) side=1;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          }   
     } else//bot
     if(front==1){//                 front
         front=0; 
         if(side==4){//front to top
              top=1;
              if(side_select==0) side=2;
              //if(side_select==1) side=3;
              if(side_select==1) side=0;
              if(side_select==2) side=1;
          } else    
         if(side==5){//front to bot
              bot=1;
              if(side_select==0) side=0;
              if(side_select==1) side=2;
              //if(side_select==2) side=3;
              if(side_select==2) side=1;
          } else  
         if(side==0){//front to left
              left=1;
              if(side_select==0) side=2;
             // if(side_select==1) side=3;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          } else      
          if(side==1){//front to right
              right=1;
              if(side_select==0) side=2;
             // if(side_select==1) side=3;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          }   
     } else//bot
     if(back==1){//                 back
         back=0; 
         if(side==4){//back to top
              top=1;
              //if(side_select==0) side=2;
              if(side_select==0) side=3;
              if(side_select==1) side=0;
              if(side_select==2) side=1;
          } else    
         if(side==5){//back to bot
              bot=1;
              if(side_select==0) side=0;
              //if(side_select==1) side=2;
              if(side_select==1) side=3;
              if(side_select==2) side=1;
          } else  
         if(side==0){//back to left
              left=1;
              //if(side_select==0) side=2;
              if(side_select==0) side=3;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          } else      
          if(side==1){//back to right
              right=1;
              //if(side_select==0) side=2;
              if(side_select==0) side=3;
              if(side_select==1) side=5;
              if(side_select==2) side=4;
          }   
     } //bot

     // for(yy=0; yy<8; yy++)
  //for(xx=0; xx<8; xx++)
  //LED(LED_Old[yy], xx, yy-oldpullback[yy], 0, 0, 0);
      for(xx=0; xx<9; xx++){
          oldpullback[xx]=0;
          pullback[xx]=0;
      }
     
      folderaddr[0]=-9;
      folderaddr[1]=-8;
      folderaddr[2]=-7;
      folderaddr[3]=-6;
      folderaddr[4]=-5;
      folderaddr[5]=-4;
      folderaddr[6]=-3;
      folderaddr[7]=-2;
      folderaddr[8]=-1;
  }//pullback==7
  }//folderaddr==7    

  if(folderaddr[8]!=8)
  for(zz=0; zz<9; zz++)
  folderaddr[zz] = folderaddr[zz]+1;
  
  }//while
  
}//folder

void wipe_out(){//*****wipe_out*****wipe_out*****wipe_out*****wipe_out*****wipe_out*****wipe_out*****wipe_out*****wipe_out
  int xxx=0, yyy=0, zzz=0;
  int fx=random(9), fy=random(9), fz=random(9), direct, fxm=1, fym=1, fzm=1, fxo=0, fyo=0, fzo=0;
  int  ftx=random(9), fty=random(9), ftz=random(9), ftxm=1, ftym=1, ftzm=1, ftxo=0, ftyo=0, ftzo=0;
  int select, intensidad1, intensidad2;
  for(xxx=0; xxx<9; xxx++){
  for(yyy=0; yyy<9; yyy++){
  for(zzz=0; zzz<9; zzz++){
  LED(xxx, yyy, zzz, 0);
  }}}
  
     
     intensidad1=15;
     intensidad2=15;
     
        inicio=millis();
      
  while(millis()-inicio<10000){
  
    //fx=random(8); fy=random(8); fz=random(8);

    LED(fxo, fyo, fzo, 0);
    LED(fxo, fyo, fzo+1, 0);
    LED(fxo, fyo, fzo-1, 0);
    LED(fxo+1, fyo, fzo, 0);
    LED(fxo-1, fyo, fzo, 0);
    LED(fxo, fyo+1, fzo, 0);
    LED(fxo, fyo-1, fzo, 0);
    
    LED(ftxo, ftyo, ftzo, 0);
    LED(ftxo, ftyo, ftzo+1, 0);
    LED(ftxo, ftyo, ftzo-1, 0);
    LED(ftxo+1, ftyo, ftzo, 0);
    LED(ftxo-1, ftyo, ftzo, 0);
    LED(ftxo, ftyo+1, ftzo, 0);
    LED(ftxo, ftyo-1, ftzo, 0);

    LED(ftx, fty, ftz, intensidad1);
    LED(ftx, fty, ftz+1, intensidad1);
    LED(ftx, fty, ftz-1,  intensidad1);
    LED(ftx+1, fty, ftz, intensidad1);
    LED(ftx-1, fty, ftz, intensidad1);
    LED(ftx, fty+1, ftz, intensidad1);
    LED(ftx, fty-1, ftz, intensidad1);     
    
    LED(fx, fy, fz, intensidad2);
    LED(fx, fy, fz+1, intensidad2);
    LED(fx, fy, fz-1, intensidad2);
    LED(fx+1, fy, fz, intensidad2);
    LED(fx-1, fy, fz, intensidad2);
    LED(fx, fy+1, fz, intensidad2);
    LED(fx, fy-1, fz, intensidad2);  
    
    
     delay(20);
     
    fxo=fx;
    fyo=fy;
    fzo=fz; 
    
    ftxo=ftx;
    ftyo=fty;
    ftzo=ftz; 
 
    direct=random(3);
    if(direct==0)
    fx= fx+fxm;
    if(direct==1)
    fy= fy+fym;  
    if(direct==2)
    fz= fz+fzm;  
  if(fx<0){
    fx=0; fxm=1;}
  if(fx>8){
    fx=8; fxm=-1;}  
  if(fy<0){
    fy=0; fym=1;}
  if(fy>8){
    fy=8; fym=-1;}    
  if(fz<0){
    fz=0; fzm=1;}
  if(fz>8){
    fz=8; fzm=-1;}  
  
      direct=random(3);
    if(direct==0)
    ftx= ftx+ftxm;
    if(direct==1)
    fty= fty+ftym;  
    if(direct==2)
    ftz= ftz+ftzm;  
  if(ftx<0){
    ftx=0; ftxm=1;}
  if(ftx>8){
    ftx=8; ftxm=-1;}  
  if(fty<0){
    fty=0; ftym=1;}
  if(fty>8){
    fty=8; ftym=-1;}    
  if(ftz<0){
    ftz=0; ftzm=1;}
  if(ftz>8){
    ftz=8; ftzm=-1;} 
  }//while
    for(xxx=0; xxx<9; xxx++){
  for(yyy=0; yyy<9; yyy++){
  for(zzz=0; zzz<9; zzz++){
  LED(xxx, yyy, zzz, 0);
  }}}
  
}//wipeout

//..............................................

char font_data[128][8] = {
{ 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00 },    // 0 :
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x3E,  0x41,  0x55,  0x41,  0x55,  0x49,  0x3E },    // 1 : 
                //              |          |
                //              |   *****  |
                //              |  *     * |
                //              |  * * * * |
                //              |  *     * |
                //              |  * * * * |
                //              |  *  *  * |
                //              |   *****  |
 
  { 0x00,  0x3E,  0x7F,  0x6B,  0x7F,  0x6B,  0x77,  0x3E },    // 2 : 
                //              |          |
                //              |   *****  |
                //              |  ******* |
                //              |  ** * ** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  *** *** |
                //              |   *****  |
 
  { 0x00,  0x22,  0x77,  0x7F,  0x7F,  0x3E,  0x1C,  0x08 },    // 3 : 
                //              |          |
                //              |   *   *  |
                //              |  *** *** |
                //              |  ******* |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |
 
  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x3E,  0x1C,  0x08 },    // 4 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |
 
  { 0x00,  0x08,  0x1C,  0x2A,  0x7F,  0x2A,  0x08,  0x1C },    // 5 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |  ******* |
                //              |   * * *  |
                //              |     *    |
                //              |    ***   |
 
  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x3E,  0x08,  0x1C },    // 6 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |   *****  |
                //              |     *    |
                //              |    ***   |
 
  { 0x00,  0x00,  0x1C,  0x3E,  0x3E,  0x3E,  0x1C,  0x00 },    // 7 :  
                //              |          |
                //              |          |
                //              |    ***   |
                //              |   *****  |
                //              |   *****  |
                //              |   *****  |
                //              |    ***   |
                //              |          |
 
  { 0xFF,  0xFF,  0xE3,  0xC1,  0xC1,  0xC1,  0xE3,  0xFF },    // 8 : 
                //              | ******** |
                //              | ******** |
                //              | ***   ** |
                //              | **     * |
                //              | **     * |
                //              | **     * |
                //              | ***   ** |
                //              | ******** |
 
  { 0x00,  0x00,  0x1C,  0x22,  0x22,  0x22,  0x1C,  0x00 },    // 9 :  
                //              |          |
                //              |          |
                //              |    ***   |
                //              |   *   *  |
                //              |   *   *  |
                //              |   *   *  |
                //              |    ***   |
                //              |          |
 
  { 0xFF,  0xFF,  0xE3,  0xDD,  0xDD,  0xDD,  0xE3,  0xFF },    // 10 :
 
                //              | ******** |
                //              | ******** |
                //              | ***   ** |
                //              | ** *** * |
                //              | ** *** * |
                //              | ** *** * |
                //              | ***   ** |
                //              | ******** |
 
  { 0x00,  0x0F,  0x03,  0x05,  0x39,  0x48,  0x48,  0x30 },    // 11 : 

                //              |          |
                //              |     **** |
                //              |       ** |
                //              |      * * |
                //              |   ***  * |
                //              |  *  *    |
                //              |  *  *    |
                //              |   **     |
 
  { 0x00,  0x08,  0x3E,  0x08,  0x1C,  0x22,  0x22,  0x1C },    // 12 : 
                //              |          |
                //              |     *    |
                //              |   *****  |
                //              |     *    |
                //              |    ***   |
                //              |   *   *  |
                //              |   *   *  |
                //              |    ***   |
 
  { 0x00,  0x18,  0x14,  0x10,  0x10,  0x30,  0x70,  0x60 },    // 13 :
 
                //              |          |
                //              |    **    |
                //              |    * *   |
                //              |    *     |
                //              |    *     |
                //              |   **     |
                //              |  ***     |
                //              |  **      |
 
  { 0x00,  0x0F,  0x19,  0x11,  0x13,  0x37,  0x76,  0x60 },    // 14 : 
                //              |          |
                //              |     **** |
                //              |    **  * |
                //              |    *   * |
                //              |    *  ** |
                //              |   ** *** |
                //              |  *** **  |
                //              |  **      |
 
  { 0x00,  0x08,  0x2A,  0x1C,  0x77,  0x1C,  0x2A,  0x08 },    // 15 : 
                //              |          |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |  *** *** |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |
 
  { 0x00,  0x60,  0x78,  0x7E,  0x7F,  0x7E,  0x78,  0x60 },    // 16 : 
                //              |          |
                //              |  **      |
                //              |  ****    |
                //              |  ******  |
                //              |  ******* |
                //              |  ******  |
                //              |  ****    |
                //              |  **      |
 
  { 0x00,  0x03,  0x0F,  0x3F,  0x7F,  0x3F,  0x0F,  0x03 },    // 17 : 
                //              |          |
                //              |       ** |
                //              |     **** |
                //              |   ****** |
                //              |  ******* |
                //              |   ****** |
                //              |     **** |
                //              |       ** |
 
  { 0x00,  0x08,  0x1C,  0x2A,  0x08,  0x2A,  0x1C,  0x08 },    // 18 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |     *    |
 
  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x00,  0x66,  0x66 },    // 19 : 
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
 
  { 0x00,  0x3F,  0x65,  0x65,  0x3D,  0x05,  0x05,  0x05 },    // 20 : 
                //              |          |
                //              |   ****** |
                //              |  **  * * |
                //              |  **  * * |
                //              |   **** * |
                //              |      * * |
                //              |      * * |
                //              |      * * |
 
  { 0x00,  0x0C,  0x32,  0x48,  0x24,  0x12,  0x4C,  0x30 },    // 21 : 
                //              |          |
                //              |     **   |
                //              |   **  *  |
                //              |  *  *    |
                //              |   *  *   |
                //              |    *  *  |
                //              |  *  **   |
                //              |   **     |
 
  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x7F,  0x7F,  0x7F },    // 22 : 
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  ******* |
                //              |  ******* |
                //              |  ******* |
 
  { 0x00,  0x08,  0x1C,  0x2A,  0x08,  0x2A,  0x1C,  0x3E },    // 23 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |   *****  |
 
  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x1C,  0x1C,  0x1C },    // 24 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |    ***   |
                //              |    ***   |
                //              |    ***   |
 
  { 0x00,  0x1C,  0x1C,  0x1C,  0x7F,  0x3E,  0x1C,  0x08 },    // 25 : 
                //              |          |
                //              |    ***   |
                //              |    ***   |
                //              |    ***   |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |
 
  { 0x00,  0x08,  0x0C,  0x7E,  0x7F,  0x7E,  0x0C,  0x08 },    // 26 : 
                //              |          |
                //              |     *    |
                //              |     **   |
                //              |  ******  |
                //              |  ******* |
                //              |  ******  |
                //              |     **   |
                //              |     *    |
 
  { 0x00,  0x08,  0x18,  0x3F,  0x7F,  0x3F,  0x18,  0x08 },    // 27 : 
                //              |          |
                //              |     *    |
                //              |    **    |
                //              |   ****** |
                //              |  ******* |
                //              |   ****** |
                //              |    **    |
                //              |     *    |
 
  { 0x00,  0x00,  0x00,  0x70,  0x70,  0x70,  0x7F,  0x7F },    // 28 : 
                //              |          |
                //              |          |
                //              |          |
                //              |  ***     |
                //              |  ***     |
                //              |  ***     |
                //              |  ******* |
                //              |  ******* |
 
  { 0x00,  0x00,  0x14,  0x22,  0x7F,  0x22,  0x14,  0x00 },    // 29 : 
                //              |          |
                //              |          |
                //              |    * *   |
                //              |   *   *  |
                //              |  ******* |
                //              |   *   *  |
                //              |    * *   |
                //              |          |
 
  { 0x00,  0x08,  0x1C,  0x1C,  0x3E,  0x3E,  0x7F,  0x7F },    // 30 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |    ***   |
                //              |   *****  |
                //              |   *****  |
                //              |  ******* |
                //              |  ******* |
 
  { 0x00,  0x7F,  0x7F,  0x3E,  0x3E,  0x1C,  0x1C,  0x08 },    // 31 : 
                //              |          |
                //              |  ******* |
                //              |  ******* |
                //              |   *****  |
                //              |   *****  |
                //              |    ***   |
                //              |    ***   |
                //              |     *    |
 
  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00 },    // 32 :  
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x18,  0x18,  0x18,  0x18,  0x18,  0x00,  0x18 },    // 33 : !
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
 
  { 0x00,  0x36,  0x36,  0x14,  0x00,  0x00,  0x00,  0x00 },    // 34 : "
                //              |          |
                //              |   ** **  |
                //              |   ** **  |
                //              |    * *   |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x36,  0x36,  0x7F,  0x36,  0x7F,  0x36,  0x36 },    // 35 : #
                //              |          |
                //              |   ** **  |
                //              |   ** **  |
                //              |  ******* |
                //              |   ** **  |
                //              |  ******* |
                //              |   ** **  |
                //              |   ** **  |
 
  { 0x00,  0x08,  0x1E,  0x20,  0x1C,  0x02,  0x3C,  0x08 },    // 36 : $
                //              |          |
                //              |     *    |
                //              |    ****  |
                //              |   *      |
                //              |    ***   |
                //              |       *  |
                //              |   ****   |
                //              |     *    |
 
  { 0x00,  0x60,  0x66,  0x0C,  0x18,  0x30,  0x66,  0x06 },    // 37 : %
                //              |          |
                //              |  **      |
                //              |  **  **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **  **  |
                //              |      **  |
 
  { 0x00,  0x3C,  0x66,  0x3C,  0x28,  0x65,  0x66,  0x3F },    // 38 : &
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |   ****   |
                //              |   * *    |
                //              |  **  * * |
                //              |  **  **  |
                //              |   ****** |
 
  { 0x00,  0x18,  0x18,  0x18,  0x30,  0x00,  0x00,  0x00 },    // 39 : '
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x60,  0x30,  0x18,  0x18,  0x18,  0x30,  0x60 },    // 40 : (
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |  **      |
 
  { 0x00,  0x06,  0x0C,  0x18,  0x18,  0x18,  0x0C,  0x06 },    // 41 : )
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |     **   |
                //              |      **  |
 
  { 0x00,  0x00,  0x36,  0x1C,  0x7F,  0x1C,  0x36,  0x00 },    // 42 : *
                //              |          |
                //              |          |
                //              |   ** **  |
                //              |    ***   |
                //              |  ******* |
                //              |    ***   |
                //              |   ** **  |
                //              |          |
 
  { 0x00,  0x00,  0x08,  0x08,  0x3E,  0x08,  0x08,  0x00 },    // 43 : +
                //              |          |
                //              |          |
                //              |     *    |
                //              |     *    |
                //              |   *****  |
                //              |     *    |
                //              |     *    |
                //              |          |
 
  { 0x00,  0x00,  0x00,  0x00,  0x30,  0x30,  0x30,  0x60 },    // 44 : ,
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |   **     |
                //              |   **     |
                //              |   **     |
                //              |  **      |
 
  { 0x00,  0x00,  0x00,  0x00,  0x3C,  0x00,  0x00,  0x00 },    // 45 : -
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x60,  0x60 },    // 46 : .
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  **      |
                //              |  **      |
 
  { 0x00,  0x00,  0x06,  0x0C,  0x18,  0x30,  0x60,  0x00 },    // 47 : /
                //              |          |
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |
                //              |          |
 
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
 
  { 0x00,  0x00,  0x18,  0x18,  0x00,  0x18,  0x18,  0x00 },    // 58 : :
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |
 
  { 0x00,  0x00,  0x18,  0x18,  0x00,  0x18,  0x18,  0x30 },    // 59 : ;
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |   **     |
 
  { 0x00,  0x06,  0x0C,  0x18,  0x30,  0x18,  0x0C,  0x06 },    // 60 : <
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |      **  |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x00,  0x3C,  0x00,  0x00 },    // 61 : =
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |          |
 
  { 0x00,  0x60,  0x30,  0x18,  0x0C,  0x18,  0x30,  0x60 },    // 62 : >
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |
 
  { 0x00,  0x3C,  0x66,  0x06,  0x1C,  0x18,  0x00,  0x18 },    // 63 : ?
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |    ***   |
                //              |    **    |
                //              |          |
                //              |    **    |
 
  { 0x00,  0x38,  0x44,  0x5C,  0x58,  0x42,  0x3C,  0x00 },    // 64 : @
                //              |          |
                //              |   ***    |
                //              |  *   *   |
                //              |  * ***   |
                //              |  * **    |
                //              |  *    *  |
                //              |   ****   |
                //              |          |
 
  { 0x00,  0x3C,  0x66,  0x66,  0x7E,  0x66,  0x66,  0x66 },    // 65 : A
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
 
  { 0x00,  0x7C,  0x66,  0x66,  0x7C,  0x66,  0x66,  0x7C },    // 66 : B
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
 
  { 0x00,  0x3C,  0x66,  0x60,  0x60,  0x60,  0x66,  0x3C },    // 67 : C
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x7C,  0x66,  0x66,  0x66,  0x66,  0x66,  0x7C },    // 68 : D
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
 
  { 0x00,  0x7E,  0x60,  0x60,  0x7C,  0x60,  0x60,  0x7E },    // 69 : E
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
                //              |  ******  |
 
  { 0x00,  0x7E,  0x60,  0x60,  0x7C,  0x60,  0x60,  0x60 },    // 70 : F
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
                //              |  **      |
 
  { 0x00,  0x3C,  0x66,  0x60,  0x60,  0x6E,  0x66,  0x3C },    // 71 : G
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |
                //              |  ** ***  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x66,  0x66,  0x66,  0x7E,  0x66,  0x66,  0x66 },    // 72 : H
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
 
  { 0x00,  0x3C,  0x18,  0x18,  0x18,  0x18,  0x18,  0x3C },    // 73 : I
                //              |          |
                //              |   ****   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   ****   |
 
  { 0x00,  0x1E,  0x0C,  0x0C,  0x0C,  0x6C,  0x6C,  0x38 },    // 74 : J
                //              |          |
                //              |    ****  |
                //              |     **   |
                //              |     **   |
                //              |     **   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ***    |
 
  { 0x00,  0x66,  0x6C,  0x78,  0x70,  0x78,  0x6C,  0x66 },    // 75 : K
                //              |          |
                //              |  **  **  |
                //              |  ** **   |
                //              |  ****    |
                //              |  ***     |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |
 
  { 0x00,  0x60,  0x60,  0x60,  0x60,  0x60,  0x60,  0x7E },    // 76 : L
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  ******  |
 
  { 0x00,  0x63,  0x77,  0x7F,  0x6B,  0x63,  0x63,  0x63 },    // 77 : M
                //              |          |
                //              |  **   ** |
                //              |  *** *** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  **   ** |
                //              |  **   ** |
                //              |  **   ** |
 
  { 0x00,  0x63,  0x73,  0x7B,  0x6F,  0x67,  0x63,  0x63 },    // 78 : N
                //              |          |
                //              |  **   ** |
                //              |  ***  ** |
                //              |  **** ** |
                //              |  ** **** |
                //              |  **  *** |
                //              |  **   ** |
                //              |  **   ** |
 
  { 0x00,  0x3C,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3C },    // 79 : O
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x7C,  0x66,  0x66,  0x66,  0x7C,  0x60,  0x60 },    // 80 : P
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
 
  { 0x00,  0x3C,  0x66,  0x66,  0x66,  0x6E,  0x3C,  0x06 },    // 81 : Q
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ** ***  |
                //              |   ****   |
                //              |      **  |
 
  { 0x00,  0x7C,  0x66,  0x66,  0x7C,  0x78,  0x6C,  0x66 },    // 82 : R
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |
 
  { 0x00,  0x3C,  0x66,  0x60,  0x3C,  0x06,  0x66,  0x3C },    // 83 : S
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |   ****   |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x7E,  0x5A,  0x18,  0x18,  0x18,  0x18,  0x18 },    // 84 : T
                //              |          |
                //              |  ******  |
                //              |  * ** *  |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3E },    // 85 : U
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
 
  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3C,  0x18 },    // 86 : V
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
 
  { 0x00,  0x63,  0x63,  0x63,  0x6B,  0x7F,  0x77,  0x63 },    // 87 : W
                //              |          |
                //              |  **   ** |
                //              |  **   ** |
                //              |  **   ** |
                //              |  ** * ** |
                //              |  ******* |
                //              |  *** *** |
                //              |  **   ** |
 
  { 0x00,  0x63,  0x63,  0x36,  0x1C,  0x36,  0x63,  0x63 },    // 88 : X
                //              |          |
                //              |  **   ** |
                //              |  **   ** |
                //              |   ** **  |
                //              |    ***   |
                //              |   ** **  |
                //              |  **   ** |
                //              |  **   ** |
 
  { 0x00,  0x66,  0x66,  0x66,  0x3C,  0x18,  0x18,  0x18 },    // 89 : Y
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x7E,  0x06,  0x0C,  0x18,  0x30,  0x60,  0x7E },    // 90 : Z
                //              |          |
                //              |  ******  |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |
                //              |  ******  |
 
  { 0x00,  0x1E,  0x18,  0x18,  0x18,  0x18,  0x18,  0x1E },    // 91 : [
                //              |          |
                //              |    ****  |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    ****  |
 
  { 0x00,  0x00,  0x60,  0x30,  0x18,  0x0C,  0x06,  0x00 },    // 92 : \
                //              |          |
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |      **  |
                //              |          |
 
  { 0x00,  0x78,  0x18,  0x18,  0x18,  0x18,  0x18,  0x78 },    // 93 : ]
                //              |          |
                //              |  ****    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |  ****    |
 
  { 0x00,  0x08,  0x14,  0x22,  0x41,  0x00,  0x00,  0x00 },    // 94 : ^
                //              |          |
                //              |     *    |
                //              |    * *   |
                //              |   *   *  |
                //              |  *     * |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x7F },    // 95 : _
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  ******* |
 
  { 0x00,  0x0C,  0x0C,  0x06,  0x00,  0x00,  0x00,  0x00 },    // 96 : `
                //              |          |
                //              |     **   |
                //              |     **   |
                //              |      **  |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x06,  0x3E,  0x66,  0x3E },    // 97 : a
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |      **  |
                //              |   *****  |
                //              |  **  **  |
                //              |   *****  |
 
  { 0x00,  0x60,  0x60,  0x60,  0x7C,  0x66,  0x66,  0x7C },    // 98 : b
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x60,  0x66,  0x3C },    // 99 : c
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x06,  0x06,  0x06,  0x3E,  0x66,  0x66,  0x3E },    // 100 : d
                //              |          |
                //              |      **  |
                //              |      **  |
                //              |      **  |
                //              |   *****  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x7E,  0x60,  0x3C },    // 101 : e
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  ******  |
                //              |  **      |
                //              |   ****   |
 
  { 0x00,  0x1C,  0x36,  0x30,  0x30,  0x7C,  0x30,  0x30 },    // 102 : f
                //              |          |
                //              |    ***   |
                //              |   ** **  |
                //              |   **     |
                //              |   **     |
                //              |  *****   |
                //              |   **     |
                //              |   **     |
 
  { 0x00,  0x00,  0x3E,  0x66,  0x66,  0x3E,  0x06,  0x3C },    // 103 : g
                //              |          |
                //              |          |
                //              |   *****  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |   ****   |
 
  { 0x00,  0x60,  0x60,  0x60,  0x7C,  0x66,  0x66,  0x66 },    // 104 : h
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
 
  { 0x00,  0x00,  0x18,  0x00,  0x18,  0x18,  0x18,  0x3C },    // 105 : i
                //              |          |
                //              |          |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   ****   |
 
  { 0x00,  0x0C,  0x00,  0x0C,  0x0C,  0x6C,  0x6C,  0x38 },    // 106 : j
                //              |          |
                //              |     **   |
                //              |          |
                //              |     **   |
                //              |     **   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ***    |
 
  { 0x00,  0x60,  0x60,  0x66,  0x6C,  0x78,  0x6C,  0x66 },    // 107 : k
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **  **  |
                //              |  ** **   |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |
 
  { 0x00,  0x18,  0x18,  0x18,  0x18,  0x18,  0x18,  0x18 },    // 108 : l
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x00,  0x00,  0x63,  0x77,  0x7F,  0x6B,  0x6B },    // 109 : m
                //              |          |
                //              |          |
                //              |          |
                //              |  **   ** |
                //              |  *** *** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  ** * ** |
 
  { 0x00,  0x00,  0x00,  0x7C,  0x7E,  0x66,  0x66,  0x66 },    // 110 : n
                //              |          |
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x66,  0x66,  0x3C },    // 111 : o
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
 
  { 0x00,  0x00,  0x7C,  0x66,  0x66,  0x7C,  0x60,  0x60 },    // 112 : p
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
 
  { 0x00,  0x00,  0x3C,  0x6C,  0x6C,  0x3C,  0x0D,  0x0F },    // 113 : q
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ****   |
                //              |     ** * |
                //              |     **** |
 
  { 0x00,  0x00,  0x00,  0x7C,  0x66,  0x66,  0x60,  0x60 },    // 114 : r
                //              |          |
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |
 
  { 0x00,  0x00,  0x00,  0x3E,  0x40,  0x3C,  0x02,  0x7C },    // 115 : s
                //              |          |
                //              |          |
                //              |          |
                //              |   *****  |
                //              |  *       |
                //              |   ****   |
                //              |       *  |
                //              |  *****   |
 
  { 0x00,  0x00,  0x18,  0x18,  0x7E,  0x18,  0x18,  0x18 },    // 116 : t
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |  ******  |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x00,  0x00,  0x66,  0x66,  0x66,  0x66,  0x3E },    // 117 : u
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
 
  { 0x00,  0x00,  0x00,  0x00,  0x66,  0x66,  0x3C,  0x18 },    // 118 : v
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
 
  { 0x00,  0x00,  0x00,  0x63,  0x6B,  0x6B,  0x6B,  0x3E },    // 119 : w
                //              |          |
                //              |          |
                //              |          |
                //              |  **   ** |
                //              |  ** * ** |
                //              |  ** * ** |
                //              |  ** * ** |
                //              |   *****  |
 
  { 0x00,  0x00,  0x00,  0x66,  0x3C,  0x18,  0x3C,  0x66 },    // 120 : x
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
                //              |   ****   |
                //              |  **  **  |
 
  { 0x00,  0x00,  0x00,  0x66,  0x66,  0x3E,  0x06,  0x3C },    // 121 : y
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |   ****   |
 
  { 0x00,  0x00,  0x00,  0x3C,  0x0C,  0x18,  0x30,  0x3C },    // 122 : z
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |   ****   |
 
  { 0x00,  0x0E,  0x18,  0x18,  0x30,  0x18,  0x18,  0x0E },    // 123 : {
                //              |          |
                //              |     ***  |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |    **    |
                //              |    **    |
                //              |     ***  |
 
  { 0x00,  0x18,  0x18,  0x18,  0x00,  0x18,  0x18,  0x18 },    // 124 : |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
 
  { 0x00,  0x70,  0x18,  0x18,  0x0C,  0x18,  0x18,  0x70 },    // 125 : }
                //              |          |
                //              |  ***     |
                //              |    **    |
                //              |    **    |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |  ***     |
 
  { 0x00,  0x00,  0x00,  0x3A,  0x6C,  0x00,  0x00,  0x00 },    // 126 : ~
                //              |          |
                //              |          |
                //              |          |
                //              |   *** *  |
                //              |  ** **   |
                //              |          |
                //              |          |
                //              |          |
 
  { 0x00,  0x08,  0x1C,  0x36,  0x63,  0x41,  0x41,  0x7F }    // 127 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   ** **  |
                //              |  **   ** |
                //              |  *     * |
                //              |  *     * |
                //              |  ******* |
 
 
};

// ........................................


void letras(){ // cuenta regresiva
  char string[4] = {'H','O','L','A'};
    for(int cuenta=0;cuenta<4;cuenta++){
       
       for(int j = 0; j<9; j++){
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){
                        LED(8-i,8-bitR,j,bitRead(font_data[string[cuenta]][i], bitR));
                  } 
            }   
            delay(50);
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){        
                        LED(8-i,8-bitR,j,0);
                  }     
            }  
       }
    }
   for(int cuenta=0;cuenta<4;cuenta++){
       
       for(int j = 0; j<9; j++){
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){
                        LED(8-i,j,bitR,bitRead(font_data[string[cuenta]][i], bitR));
                  } 
            }   
            delay(50);
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){        
                        LED(8-i,j,8-bitR,0);
                  }     
            }  
       }
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
   

   for(int k=57;k>47;k--){

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
                        LED(8-i,8-bitR,j,bitRead(font_data[k][i], bitR));
                         if (j == 4){
                            matrixA[cuenta]=8-i;
                            matrixB[cuenta]=8-bitR;
                            matrixC[cuenta]=j;
                            matrixD[cuenta]=bitRead(font_data[k][i], bitR);
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

void fireworks (){
  int i,f,e;
  float origin_x = 3;
  float origin_y = 3;
  float origin_z = 3;
  int n=30;
  int delay_ms = 40;
  int rand_y, rand_x, rand_z;

  float slowrate, gravity;

  // Particles and their position, x,y,z and their movement, dx, dy, dz
  float particles[n][6];
  int iterations = 10;
  
  for (i=0; i<iterations; i++)
  {
     apagar();

    origin_x = rand()%4;
    origin_y = rand()%4;
    origin_z = rand()%2;
    origin_z +=5;
    origin_x +=2;
    origin_y +=2;

    // shoot a particle up in the air
    int x_old=0,y_old=0,z_old=0;
    for (e=0;e<origin_z;e++)
    {
      LED(z_old,x_old,y_old,0);
      LED(e,origin_x,origin_y,15);
      x_old=origin_x;
      y_old=origin_y;
      z_old=e;
      delay(30+20*e);
    }

    // Fill particle array
    for (f=0; f<n; f++)
    {
      // Position
      particles[f][0] = origin_x;
      particles[f][1] = origin_y;
      particles[f][2] = origin_z;
      
      rand_x = rand()%200;
      rand_y = rand()%200;
      rand_z = rand()%200;

      // Movement
      particles[f][3] = 1-(float)rand_x/100; // dx
      particles[f][4] = 1-(float)rand_y/100; // dy
      particles[f][5] = 1-(float)rand_z/100; // dz
    }

    // explode
    for (e=0; e<25; e++)
    {
      slowrate = 1+tan((e+0.1)/20)*10;
      
      gravity = tan((e+0.1)/20)/2;

      for (f=0; f<n; f++)
      {
        particles[f][0] += particles[f][3]/slowrate;
        particles[f][1] += particles[f][4]/slowrate;
        particles[f][2] += particles[f][5]/slowrate;
        particles[f][2] -= gravity;

        LED(particles[f][2],particles[f][0],particles[f][1],15);

      }

      delay(delay_ms);
      
      for (f=0; f<n; f++)
      {

        LED(particles[f][2],particles[f][0],particles[f][1],0);

      }
      
    }

  }

}

void sinelines (int iterations, int delay_ms)
{
  int i,x;

  float left, right, sine_base, x_dividor,ripple_height;

  for (i=0; i<iterations; i++)
  {
    for (x=0; x<9 ;x++)
    {
      x_dividor = 2 + sin((float)i/100)+1;
      ripple_height = 3 + (sin((float)i/200)+1)*6;

      sine_base = (float) i/40 + (float) x/x_dividor;

      left = 4 + sin(sine_base)*ripple_height;
      right = 4 + cos(sine_base)*ripple_height;
      right = 7-left;

      //printf("%i %i \n", (int) left, (int) right);

      line_3d(0-3, x, (int) left, 7+3, x, (int) right);
      //line_3d((int) right, 7, x);
    }
  
   delay(delay_ms);
  apagar();
  }
}

void line_3d (int x1, int y1, int z1, int x2, int y2, int z2)
{
  int i, dx, dy, dz, l, m, n, x_inc, y_inc, z_inc,
  err_1, err_2, dx2, dy2, dz2;
  int pixel[3];
  pixel[0] = x1;
  pixel[1] = y1;
  pixel[2] = z1;
  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;
  x_inc = (dx < 0) ? -1 : 1;
  l = abs(dx);
  y_inc = (dy < 0) ? -1 : 1;
  m = abs(dy);
  z_inc = (dz < 0) ? -1 : 1;
  n = abs(dz);
  dx2 = l << 1;
  dy2 = m << 1;
  dz2 = n << 1;
  if ((l >= m) && (l >= n)) {
  err_1 = dy2 - l;
  err_2 = dz2 - l;
  for (i = 0; i < l; i++) {
      //PUT_PIXEL(pixel);
      LED(pixel[2],pixel[0],pixel[1],15);
      //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
      if (err_1 > 0) {
          pixel[1] += y_inc;
          err_1 -= dx2;
      }
      if (err_2 > 0) {
          pixel[2] += z_inc;
          err_2 -= dx2;
      }
      err_1 += dy2;
      err_2 += dz2;
      pixel[0] += x_inc;
      }
  } else if ((m >= l) && (m >= n)) {
      err_1 = dx2 - m;
      err_2 = dz2 - m;
      for (i = 0; i < m; i++) {
          //PUT_PIXEL(pixel);
          LED(pixel[2],pixel[0],pixel[1],15);
          //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
          if (err_1 > 0) {
              pixel[0] += x_inc;
              err_1 -= dy2;
          }
          if (err_2 > 0) {
              pixel[2] += z_inc;
              err_2 -= dy2;
          }
          err_1 += dx2;
          err_2 += dz2;
          pixel[1] += y_inc;
     }
  } else {
      err_1 = dy2 - n;
      err_2 = dx2 - n;
      for (i = 0; i < n; i++) {
          LED(pixel[2],pixel[0],pixel[1],15);
          //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
          //PUT_PIXEL(pixel);
          if (err_1 > 0) {
              pixel[1] += y_inc;
              err_1 -= dz2;
          }
          if (err_2 > 0) {
              pixel[0] += x_inc;
              err_2 -= dz2;
          }
          err_1 += dy2;
          err_2 += dx2;
          pixel[2] += z_inc;
      }
  }
  LED(pixel[2],pixel[0],pixel[1],15);
  //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
  //PUT_PIXEL(pixel);
}

void effect_boxside_randsend_parallel (char axis, int origin, int delay_ms, int mode)
{
  int i;
  int done;
  unsigned char cubepos[81];
  unsigned char pos[81];
  int notdone = 1;
  int notdone2 = 1;
  int sent = 0;
  
  for (i=0;i<81;i++)
  {
    pos[i] = 0;
  }
  
  while (notdone)
  {
    if (mode == 1)
    {
      notdone2 = 1;
      while (notdone2 && sent<81)
      {
        i = rand()%81;
        if (pos[i] == 0)
        {
          sent++;
          pos[i] += 1;
          notdone2 = 0;
        }
      }
    } else if (mode == 2)
    {
      if (sent<81)
      {
        pos[sent] += 1;
        sent++;
      }
    }
    
    done = 0;
    for (i=0;i<81;i++)
    {
      if (pos[i] > 0 && pos[i] <8)
      {
        pos[i] += 1;
      }
        
      if (pos[i] == 8)
        done++;
    }
    
    if (done == 81)
      notdone = 0;
    
    for (i=0;i<81;i++)
    {
      if (origin == 0)
      {
        cubepos[i] = pos[i];
      } else
      {
        cubepos[i] = (8-pos[i]);
      }
    }
    delay(delay_ms);
    draw_positions_axis(axis,cubepos,0);
  } 
}

void draw_positions_axis (char axis, unsigned char positions[81], int invert)
{
  int x, y, p;
  
  apagar();
  
  for (x=0; x<9; x++)
  {
    for (y=0; y<9; y++)
    {
      if (invert)
      {
        p = (7-positions[(x*8)+y]);
      } else
      {
        p = positions[(x*8)+y];
      }
    
      if (axis == AXIS_Z)
        LED(p,x,y,15);
        
      if (axis == AXIS_Y)
        LED(y,x,p,15);
        
      if (axis == AXIS_X)
        LED(x,p,y,15);
    }
  }
  
}

void effect_axis_updown_randsuspend (char axis, int delay_ms, int sleep_m, int invert)
{
  unsigned char positions[81];
  unsigned char destinations[81];

  int i,px;
  
    // Set 64 random positions
  for (i=0; i<81; i++)
  {
    positions[i] = 0; // Set all starting positions to 0
    destinations[i] = rand()%9;
  }

    // Loop 8 times to allow destination 7 to reach all the way
  for (i=0; i<9; i++)
  {
        // For every iteration, move all position one step closer to their destination
    for (px=0; px<81; px++)
    {
      if (positions[px]<destinations[px])
      {
        positions[px]++;
      }
    }
        // Draw the positions and take a nap
    draw_positions_axis (axis, positions,invert);
    delay(delay_ms);
  }
  
    // Set all destinations to 7 (opposite from the side they started out)
  for (i=0; i<81; i++)
  {
    destinations[i] = 8;
  }
  
    // Suspend the positions in mid-air for a while
  delay(sleep_m);
  
    // Then do the same thing one more time
  for (i=0; i<9; i++)
  {
    for (px=0; px<81; px++)
    {
      if (positions[px]<destinations[px])
      {
        positions[px]++;
      }
      if (positions[px]>destinations[px])
      {
        positions[px]--;
      }
    }
    draw_positions_axis (axis, positions,invert);
    delay(delay_ms);
  }
}

void init_LUT(unsigned char LUT[82])
{
  unsigned char i;
  float sin_of,sine;
  for (i=0;i<82;i++)
  {
    sin_of=i*PI/81; // Just need half a sin wave
    sine=sin(sin_of);
    // Use 181.0 as this squared is <32767, so we can multiply two sin or cos without overflowing an int.
    LUT[i]=sine*181.0;
  }
}

int totty_sin(unsigned char LUT[82],int sin_of)
{
  unsigned char inv=0;
  if (sin_of<0)
  {
    sin_of=-sin_of;
    inv=1;
  }
  sin_of&=0x7f; //127
  if (sin_of>81)
  {
    sin_of-=81;
    inv=1-inv;
  }
  if (inv)
    return -LUT[sin_of];
  else
    return LUT[sin_of];
}

int totty_cos(unsigned char LUT[82],int cos_of)
{
  unsigned char inv=0;
  cos_of+=32;// Simply rotate by 90 degrees for COS
  cos_of&=0x7f;//127
  if (cos_of>81)
  {
    cos_of-=81;
    inv=1;
  }
  if (inv)
    return -LUT[cos_of];
  else
    return LUT[cos_of];
}

void linespin (int iterations, int delay_ms)
{
  float top_x, top_y, top_z, bot_x, bot_y, bot_z, sin_base;
  float center_x, center_y;

  center_x = 5;
  center_y = 5;

  int i, z;
  for (i=0;i<iterations;i++)
  {

    //printf("Sin base %f \n",sin_base);

    for (z = 0; z < 9; z++)
    {

        sin_base = (float)i/50 + (float)z/(10+(7*sin((float)i/200)));
    
        top_x = center_x + sin(sin_base)*5;
        top_y = center_x + cos(sin_base)*5;
        //top_z = center_x + cos(sin_base/100)*2.5;
    
        bot_x = center_x + sin(sin_base+3.14)*10;
        bot_y = center_x + cos(sin_base+3.14)*10;
        //bot_z = 7-top_z;
        
        bot_z = z;
        top_z = z;
    
        // setvoxel((int) top_x, (int) top_y, 7);
        // setvoxel((int) bot_x, (int) bot_y, 0);
    
        //printf("P1: %i %i %i P2: %i %i %i \n", (int) top_x, (int) top_y, 7, (int) bot_x, (int) bot_y, 0);
    
        //line_3d((int) top_x, (int) top_y, (int) top_z, (int) bot_x, (int) bot_y, (int) bot_z);
        linea_3d((int) top_z, (int) top_x, (int) top_y, (int) bot_z, (int) bot_x, (int) bot_y);
    }

    // delay_ms(delay);
    apagar();
  }

}

void linea_3d(int x1, int y1, int z1, int x2, int y2, int z2){
  int i, dx, dy, dz, l, m, n, x_inc, y_inc, z_inc, err_1, err_2, dx2, dy2, dz2;
  int pixel[3];
  pixel[0] = x1;
  pixel[1] = y1;
  pixel[2] = z1;
  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;
  x_inc = (dx < 0) ? -1 : 1;
  l = abs(dx);
  y_inc = (dy < 0) ? -1 : 1;
  m = abs(dy);
  z_inc = (dz < 0) ? -1 : 1;
  n = abs(dz);
  dx2 = l << 1;
  dy2 = m << 1;
  dz2 = n << 1;
  if ((l >= m) && (l >= n)) {
      err_1 = dy2 - l;
      err_2 = dz2 - l;
      for (i = 0; i < l; i++) {
          //PUT_PIXEL(pixel);
          LED(pixel[2],pixel[0],pixel[1],15);
          //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
          if (err_1 > 0) {
              pixel[1] += y_inc;
              err_1 -= dx2;
          }
          if (err_2 > 0) {
              pixel[2] += z_inc;
              err_2 -= dx2;
          }
          err_1 += dy2;
          err_2 += dz2;
          pixel[0] += x_inc;
      }
  } else if ((m >= l) && (m >= n)) {
      err_1 = dx2 - m;
      err_2 = dz2 - m;
      for (i = 0; i < m; i++) {
          //PUT_PIXEL(pixel);
          LED(pixel[2],pixel[0],pixel[1],15);
          //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
          if (err_1 > 0) {
              pixel[0] += x_inc;
              err_1 -= dy2;
          }
          if (err_2 > 0) {
              pixel[2] += z_inc;
              err_2 -= dy2;
          }
          err_1 += dx2;
          err_2 += dz2;
          pixel[1] += y_inc;
      }
  } else {
      err_1 = dy2 - n;
      err_2 = dx2 - n;
      for (i = 0; i < n; i++) {
          LED(pixel[2],pixel[0],pixel[1],15);
          //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
          //PUT_PIXEL(pixel);
          if (err_1 > 0) {
              pixel[1] += y_inc;
              err_1 -= dz2;
          }
          if (err_2 > 0) {
              pixel[0] += x_inc;
              err_2 -= dz2;
          }
          err_1 += dy2;
          err_2 += dx2;
          pixel[2] += z_inc;
      }
  }
  LED(pixel[2],pixel[0],pixel[1],15);
  //printf("Setting %i %i %i \n", pixel[0],pixel[1],pixel[2]);
  //PUT_PIXEL(pixel);
}
