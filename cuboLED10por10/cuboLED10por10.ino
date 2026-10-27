// programa para cubo LED 10x10x10

#include <SPI.h>                                          // SPI Library used to clock data out to the shift registers

#define tamanoCubo 10                                             // tamaño del cubo led
#define cantidadRegistros 13 //int((tamanoCubo*tamanoCubo)/8)+1   //cantidad de registros de 74HC595 (solo los registros que manejan los anodos), este valor se consigue encontrando el numero de lEDs en un piso,
                                                                  // en nuestro caso son (10 por 10 = 100 LEDs), 100 LEDs dividido 8 salidas por registro = 12.5 registros, entonces son 13 registros
                                                                  // estos son la cantidad de registros (bytes) necesarios para encender un solo piso independientemente
#define totalBitesPlanos 130 //cantidadRegistros*tamanoCubo       // cantidad de registros (bytes) necesarios para encender todo el cubo LED
                                                  

#define latch_pin 2                                     // Pin conocido como Latch, en el integrado es el pin 12
#define data_pin 11                                     // Pin conocido como data, en el integrado es el pin 14
#define clock_pin 13                                    // Pin conocido como Clock, en el integrado es el pin 13

unsigned long inicio;                                  // variable para almacenar variables de tiempo

byte catodos[tamanoCubo*2];                              // vectores para la secuencia de encendido de los pisos     

byte matrizLED[totalBitesPlanos];                        // matriz donde se guardaràn toda los datos del cubo LED, 

int registros=0;                                       // variable para guardar los datos de los registros para la multiplexion de los pisos
int nivel=0;                                           // variable para almacenar el piso o nivel a multiplexar

void setup() {

  SPI.setBitOrder(MSBFIRST);                          // el bit mas significativo primero
  SPI.setDataMode(SPI_MODE0);                         // Mode 0 Rising edge of data
  SPI.setClockDivider(SPI_CLOCK_DIV2);                //los datos seran enviados a una velocidad de 16MHz/2 - 8MHpiso
 
  noInterrupts();                                     // desactivar interrupciones
  
  //Configuraciòn de timer 1 para la multiplepisoaciòn del cubo led 
  
  TCCR1A = B00000000;                                 //No es necesario configurar, no se usaràn estas opciones del timer
  TCCR1B = B00001011;                                 //, se habilita la opcion CTC1 (borrado del contador), y  CS12 = 0, CS11 = 1, CS10 = 1, preescaler = 64 por lo tanto:  16MHz/64=250kHz
  TIMSK1 = B00000010;                                 //bit 1 set to call the interrupt on an OCR1A match
  OCR1A=50;                                           //  1/250kHz = 4us .  (24+1)x4us=100us

   catodos[0]=B00010000;                                // matrix de activaciòn de los catodos, se utiliza en la multiplexion de los pisos
   catodos[1]=B00000000;
   catodos[2]=B00100000;
   catodos[3]=B00000000;
   catodos[4]=B01000000;
   catodos[5]=B00000000;
   catodos[6]=B10000000;
   catodos[7]=B00000000;
   catodos[8]=B00000000;
   catodos[9]=B00000001;
  catodos[10]=B00000000;
  catodos[11]=B00000010;
  catodos[12]=B00000000;
  catodos[13]=B00000100;
  catodos[14]=B00000000;
  catodos[15]=B00001000;
  catodos[16]=B00000000;
  catodos[17]=B00010000;
  catodos[18]=B00000000;
  catodos[19]=B00100000;

  pinMode(latch_pin, OUTPUT);                            //pin de Latch
  pinMode(data_pin, OUTPUT);                             //pin de datos MOSI DATA
  pinMode(clock_pin, OUTPUT);                            //pin de SPI Clock

  SPI.begin();                                          //iniciamos la libreria SPI 
  interrupts();                                         //iniciamos las interrupciones
  //Serial.begin(9600);
}


void loop() {                                           // programa principal

// cubo1();   
// aleatorio();
// planos();
// planos();     
// flash();       
animacionSeno1();
animacionSeno2();
}

void LED(int Z, int Y, int X, byte intensidad){         // funcion para asignaciòn de BITs en el cubo LED.     LED(Z,Y,X, 1 o 0)

  // el cubo led tiene coordenadas que inician en el LED (0,0,0) y llega hasta el led (9,9,9)
  
  if(Z<0)                                              // la posicion Z solo puede estar entre los valores 0 y 9 
    Z=0;
  if(Z>tamanoCubo-1)
    Z=tamanoCubo-1;
  
  if(Y<0)                                              // la posicion Y solo puede estar entre los valores 0 y 9 
    Y=0;
  if(Y>tamanoCubo-1)
    Y=tamanoCubo-1;
  
  if(X<0)                                             // la posicion X solo puede estar entre los valores 0 y 9
    X=0;
  if(X>tamanoCubo-1)
    X=tamanoCubo-1;  
  
  if(intensidad<0)                                   // encendido y apagado del led (0 o 1)
  intensidad=0;
  if(intensidad>1)
  intensidad=1;
  
  int posicionLED=(Z*tamanoCubo*tamanoCubo)+(Y*tamanoCubo)+X;   // numeraciòn del led en el cubo
  int posReg = (posicionLED-(tamanoCubo*tamanoCubo*Z));         // numeracion del led en el piso (nivel)

  bitWrite(matrizLED[int(posReg/8)+cantidadRegistros*Z],  posReg-(int(posReg/8)*8), intensidad);    // escribe un bit sobre un vector 
   // bitWrite(byte a escribir, posicion del bit a escribir, bit a escribir)   
  
}


ISR(TIMER1_COMPA_vect){                      // ISR  interrupt services routines, rutina de interrupciòn
  
 digitalWrite(latch_pin,LOW);                // dehabilita los datos a las salidas de los registros
   
         SPI.transfer(catodos[nivel*2+1]);                         // se carga el primer byte el cual controlarà los càtodos      
         SPI.transfer(catodos[nivel*2]+matrizLED[registros+(cantidadRegistros-1)]);   // se carga el segundo byte el cual controlarà los càtodos màs los datos restantes de los anodos  
        
         for(int i=registros+(cantidadRegistros-2);i>=registros;i--){               // se cargan los datos de los registros para controlar los anodos de los niveles 
           SPI.transfer(matrizLED[i]);  
         }
         
 digitalWrite(latch_pin,HIGH);             // despliega los datos a las salidas de los registros
 
 registros = registros + cantidadRegistros;              // aumenta la posicion de carga de bytes, asì se recorren los datos de los diferentes pisos sobre el vector aplanado
 nivel++;                                                // variable para conocer el piso a multiplexar

 if(nivel == tamanoCubo){                               // si ya se han multiplexado todos los niveles o pisos, se debe reiniciar la multiplexiòn
   registros=0;                                         
   nivel = 0;
 }
}


void aleatorio(){          // encender y apagar LEDs aleatorios en todo el cubo
  inicio = millis();
  while((millis()-inicio)<10000){
      LED(random(tamanoCubo),random(tamanoCubo),random(tamanoCubo),random(2));
  }
  
}


void apagar(){          // apagar  todo el cubo LED

  for(int x=0;x<totalBitesPlanos;x++){
    matrizLED[x]=0;

  }
}

void encender(){        // encender todo el cubo LED

  for(int x=0;x<tamanoCubo;x++){
    for(int y=0;y<tamanoCubo;y++){
      for(int z=0;z<tamanoCubo;z++){
        LED(z,y,x,1);
      }
    }
  }
  
}


void cubo1(){                   // animacion para encender los bordes del cubo LED
  apagar();
  int tiempo = 100;
  for(int x=0;x<tamanoCubo;x++){
    LED(0,0,x,1);
    LED(0,x,0,1);
    LED(x,0,0,1);
    delay(tiempo);
  }
  for(int x=0;x<tamanoCubo;x++){
    LED(x,0,tamanoCubo-1,1);
    LED(0,x,tamanoCubo-1,1);
    LED(0,tamanoCubo-1,x,1);
    LED(x,tamanoCubo-1,0,1);
    LED(tamanoCubo-1,x,0,1);
    LED(tamanoCubo-1,0,x,1);
    delay(tiempo);
  }
  for(int x=0;x<tamanoCubo;x++){
    LED(tamanoCubo-1,tamanoCubo-1,x,1);
    LED(x,tamanoCubo-1,tamanoCubo-1,1);
    LED(tamanoCubo-1,x,tamanoCubo-1,1);
    delay(tiempo);
  }
  delay(500);
  for(int x=0;x<tamanoCubo;x++){
    LED(0,0,x,0);
    LED(0,x,0,0);
    LED(x,0,0,0);
    delay(tiempo);
  }
  for(int x=0;x<tamanoCubo;x++){
    LED(x,0,tamanoCubo-1,0);
    LED(0,x,tamanoCubo-1,0);
    LED(0,tamanoCubo-1,x,0);
    LED(x,tamanoCubo-1,0,0);
    LED(tamanoCubo-1,x,0,0);
    LED(tamanoCubo-1,0,x,0);
    delay(tiempo);
  }
  for(int x=0;x<tamanoCubo;x++){
    LED(tamanoCubo-1,tamanoCubo-1,x,0);
    LED(x,tamanoCubo-1,tamanoCubo-1,0);
    LED(tamanoCubo-1,x,tamanoCubo-1,0);
    delay(tiempo);
  }
}


void planos(){                       // animacion de desplazamiento de planos en el eje X,Y y Z
  apagar();
  inicio = millis();
  while(millis()-inicio< 5000){
     for(int z=0;z<tamanoCubo;z++){                   // desplazamiento en el eje Z
        for(int y=0;y<tamanoCubo;y++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int y=0;y<tamanoCubo;y++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int z=tamanoCubo-1;z>=0;z--){
        for(int y=0;y<tamanoCubo;y++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int y=0;y<tamanoCubo;y++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,0);
          }
        }
     }
  }
  inicio = millis();
  while(millis()-inicio< 5000){                   // desplazamiento en el eje Y
     for(int y=0;y<tamanoCubo;y++){
        for(int z=0;z<tamanoCubo;z++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int z=0;z<tamanoCubo;z++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int y=tamanoCubo-1;y>=0;y--){
        for(int z=0;z<tamanoCubo;z++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int z=0;z<tamanoCubo;z++){
          for(int x=0;x<tamanoCubo;x++){
            LED(z,y,x,0);
          }
        }
     }
  }
  inicio = millis();
  while(millis()-inicio< 5000){
     for(int x=0;x<tamanoCubo;x++){            // desplazamiento en el eje X
        for(int y=0;y<tamanoCubo;y++){
          for(int z=0;z<tamanoCubo;z++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int y=0;y<tamanoCubo;y++){
          for(int z=0;z<tamanoCubo;z++){
            LED(z,y,x,0);
          }
        }
     }
  
     for(int x=tamanoCubo-1;x>=0;x--){
        for(int y=0;y<tamanoCubo;y++){
          for(int z=0;z<tamanoCubo;z++){
            LED(z,y,x,1);
          }
        }
        delay(50);
        for(int y=0;y<tamanoCubo;y++){
          for(int z=0;z<tamanoCubo;z++){
            LED(z,y,x,0);
          }
        }
     }
  }
  
}

void flash(){                 // funcion para encendido y apagado de todo el cubo LED.

  apagar();
  delay(200);
  encender();
  delay(200);
  apagar();
  delay(200);
  encender();
  delay(200);
  apagar();
  delay(200);
  encender();
  delay(200);

}

void animacionSeno1(){                                      // funciòn que crea una animacion senoidal, esta funcion no es general debe modificarse segun el tamaño del cubo LED
    int sinewavearray[tamanoCubo], addr, sinemult[tamanoCubo], intensidad=1;
    int sinewavearrayOLD[tamanoCubo], select,  subT=tamanoCubo;
  
   sinewavearray[0]=0;
   sinewavearray[1]=1;
   sinewavearray[2]=2;
   sinewavearray[3]=3;
   sinewavearray[4]=4;
   sinewavearray[5]=5;
   sinewavearray[6]=6;
   sinewavearray[7]=7;
   sinewavearray[8]=8;
   sinewavearray[9]=9;
   
   sinemult[0]=1;
   sinemult[1]=1; 
   sinemult[2]=1;
   sinemult[3]=1;
   sinemult[4]=1;
   sinemult[5]=1;
   sinemult[6]=1;
   sinemult[7]=1;
   sinemult[8]=1;
   sinemult[9]=1;
  
  inicio=millis();
      
  while(millis()-inicio<5000){
    
  for(addr=0; addr<tamanoCubo; addr++){
      if(sinewavearray[addr]==tamanoCubo-1){
        sinemult[addr]=-1;
      }
      if(sinewavearray[addr]==0){
        sinemult[addr]=1;     
      }
      sinewavearray[addr] = sinewavearray[addr] + sinemult[addr];
   }


  for(addr=0; addr<tamanoCubo; addr++){
    
    LED(sinewavearrayOLD[addr], addr, 0, 0);
    LED(sinewavearrayOLD[addr], 0, addr, 0);
    LED(sinewavearrayOLD[addr], subT-addr, tamanoCubo-1, 0);
    LED(sinewavearrayOLD[addr], tamanoCubo-1, subT-addr, 0);
         
    LED(sinewavearray[addr], addr, 0, intensidad);
    LED(sinewavearray[addr], 0, addr, intensidad);
    LED(sinewavearray[addr], subT-addr, tamanoCubo-1, intensidad);
    LED(sinewavearray[addr], tamanoCubo-1, subT-addr, intensidad);
  }
    
  for(addr=1; addr<tamanoCubo-1; addr++){   
     LED(sinewavearrayOLD[addr+1], addr, 1, 0);
     LED(sinewavearrayOLD[addr+1], 1, addr, 0);
     LED(sinewavearrayOLD[addr+1], subT-addr, tamanoCubo-2, 0);
     LED(sinewavearrayOLD[addr+1], tamanoCubo-2, subT-addr, 0);  
      
     LED(sinewavearray[addr+1], addr, 1, intensidad);
     LED(sinewavearray[addr+1], 1, addr, intensidad);
     LED(sinewavearray[addr+1], subT-addr,tamanoCubo-2, intensidad);
     LED(sinewavearray[addr+1], tamanoCubo-2, subT-addr, intensidad);
  }

  for(addr=2; addr<tamanoCubo-2; addr++){   
      LED(sinewavearrayOLD[addr+2], addr, 2, 0);
      LED(sinewavearrayOLD[addr+2], 2, addr, 0);
      LED(sinewavearrayOLD[addr+2], subT-addr, tamanoCubo-3, 0);
      LED(sinewavearrayOLD[addr+2], tamanoCubo-3, subT-addr, 0);  
      
      LED(sinewavearray[addr+2], addr, 2, intensidad);
      LED(sinewavearray[addr+2], 2, addr, intensidad);
      LED(sinewavearray[addr+2], subT-addr,tamanoCubo-3, intensidad);
      LED(sinewavearray[addr+2], tamanoCubo-3, subT-addr, intensidad);
   }  
//   
  for(addr=3; addr<tamanoCubo-3; addr++){   
      LED(sinewavearrayOLD[addr+3], addr, 3, 0);
      LED(sinewavearrayOLD[addr+3], 3, addr, 0);
      LED(sinewavearrayOLD[addr+3], subT-addr, tamanoCubo-4, 0);
      LED(sinewavearrayOLD[addr+3], tamanoCubo-4, subT-addr, 0);  
      
      LED(sinewavearray[addr+3], addr, 3,intensidad);
      LED(sinewavearray[addr+3], 3, addr, intensidad);
      LED(sinewavearray[addr+3], subT-addr,tamanoCubo-4, intensidad);
      LED(sinewavearray[addr+3], tamanoCubo-4, subT-addr, intensidad);
   }      
//
   for(addr=4; addr<tamanoCubo-4; addr++){   
      LED(sinewavearrayOLD[addr+4], addr, 4, 0);
      LED(sinewavearrayOLD[addr+4], 4, addr, 0);
      LED(sinewavearrayOLD[addr+4], subT-addr, tamanoCubo-5, 0);
      LED(sinewavearrayOLD[addr+4], tamanoCubo-5, subT-addr, 0);  
      
      LED(sinewavearray[addr+4], addr, 4,intensidad);
      LED(sinewavearray[addr+4], 4, addr, intensidad);
      LED(sinewavearray[addr+4], subT-addr,tamanoCubo-5, intensidad);
      LED(sinewavearray[addr+4], tamanoCubo-5, subT-addr, intensidad);
   }    
   
     for(addr=0; addr<tamanoCubo; addr++)
         sinewavearrayOLD[addr]=sinewavearray[addr];
         
    delay(50);
  } 
}

void animacionSeno2(){                                      // funciòn que crea una animacion senoidal, esta funcion no es general debe modificarse segun el tamaño del cubo LED
    int sinewavearray[tamanoCubo], addr, sinemult[tamanoCubo], intensidad=1;
    int sinewavearrayOLD[tamanoCubo], select,  subT=tamanoCubo-1;
  
   sinewavearray[0]=0;
   sinewavearray[1]=1;
   sinewavearray[2]=2;
   sinewavearray[3]=3;
   sinewavearray[4]=4;
   sinewavearray[5]=5;
   sinewavearray[6]=6;
   sinewavearray[7]=7;
   sinewavearray[8]=8;
   sinewavearray[9]=9;
   
   sinemult[0]=1;
   sinemult[1]=1; 
   sinemult[2]=1;
   sinemult[3]=1;
   sinemult[4]=1;
   sinemult[5]=1;
   sinemult[6]=1;
   sinemult[7]=1;
   sinemult[8]=1;
   sinemult[9]=1;
  
  inicio=millis();
      
  while(millis()-inicio<5000){
    
  for(addr=0; addr<tamanoCubo; addr++){
      if(sinewavearray[addr]==tamanoCubo-1){
        sinemult[addr]=-1;
      }
      if(sinewavearray[addr]==0){
        sinemult[addr]=1;     
      }
      sinewavearray[addr] = sinewavearray[addr] + sinemult[addr];
   }


  for(addr=0; addr<tamanoCubo; addr++){   
     LED(sinewavearrayOLD[addr], 0, subT-addr, 0);
     LED(sinewavearrayOLD[addr], subT-addr, 0, 0);
     LED(sinewavearrayOLD[addr], tamanoCubo-1, addr, 0);
     LED(sinewavearrayOLD[addr], addr, tamanoCubo-1, 0);
      
     LED(sinewavearray[addr],  0, subT-addr, intensidad);
     LED(sinewavearray[addr], subT-addr, 0, intensidad);
     LED(sinewavearray[addr],tamanoCubo-1, addr , intensidad);
     LED(sinewavearray[addr],addr, tamanoCubo-1 , intensidad);
  }
//    
  for(addr=1; addr<tamanoCubo-1; addr++){   
     LED(sinewavearrayOLD[addr+1], 1, subT-addr, 0);
     LED(sinewavearrayOLD[addr+1], subT-addr, 1, 0);
     LED(sinewavearrayOLD[addr+1], tamanoCubo-2, addr, 0);
     LED(sinewavearrayOLD[addr+1], addr, tamanoCubo-2, 0);
      
     LED(sinewavearray[addr+1],  1, subT-addr, intensidad);
     LED(sinewavearray[addr+1], subT-addr, 1, intensidad);
     LED(sinewavearray[addr+1],tamanoCubo-2, addr , intensidad);
     LED(sinewavearray[addr+1],addr, tamanoCubo-2 , intensidad);
  }
////
  for(addr=2; addr<tamanoCubo-2; addr++){   
     LED(sinewavearrayOLD[addr+2], 2, subT-addr, 0);
     LED(sinewavearrayOLD[addr+2], subT-addr, 2, 0);
     LED(sinewavearrayOLD[addr+2], tamanoCubo-3, addr, 0);
     LED(sinewavearrayOLD[addr+2], addr, tamanoCubo-3, 0);
      
     LED(sinewavearray[addr+2],  2, subT-addr, intensidad);
     LED(sinewavearray[addr+2], subT-addr, 2, intensidad);
     LED(sinewavearray[addr+2],tamanoCubo-3, addr , intensidad);
     LED(sinewavearray[addr+2],addr, tamanoCubo-3 , intensidad);
   }  
////   
  for(addr=3; addr<tamanoCubo-3; addr++){   
     LED(sinewavearrayOLD[addr+3], 3, subT-addr, 0);
     LED(sinewavearrayOLD[addr+3], subT-addr, 3, 0);
     LED(sinewavearrayOLD[addr+3], tamanoCubo-4, addr, 0);
     LED(sinewavearrayOLD[addr+3], addr, tamanoCubo-4, 0);
      
     LED(sinewavearray[addr+3],  3, subT-addr, intensidad);
     LED(sinewavearray[addr+3], subT-addr, 3, intensidad);
     LED(sinewavearray[addr+3],tamanoCubo-4, addr , intensidad);
     LED(sinewavearray[addr+3],addr, tamanoCubo-4 , intensidad);
   }      
//////
  for(addr=4; addr<tamanoCubo-4; addr++){   
     LED(sinewavearrayOLD[addr+4], 4, subT-addr, 0);
     LED(sinewavearrayOLD[addr+4], subT-addr, 4, 0);
     LED(sinewavearrayOLD[addr+4], tamanoCubo-5, addr, 0);
     LED(sinewavearrayOLD[addr+4], addr, tamanoCubo-5, 0);
      
     LED(sinewavearray[addr+4],  4, subT-addr, intensidad);
     LED(sinewavearray[addr+4], subT-addr, 4, intensidad);
     LED(sinewavearray[addr+4],tamanoCubo-5, addr , intensidad);
     LED(sinewavearray[addr+4],addr, tamanoCubo-5 , intensidad);
   }      
   
     for(addr=0; addr<tamanoCubo; addr++)
         sinewavearrayOLD[addr]=sinewavearray[addr];
         
    delay(50);
  } 
}
