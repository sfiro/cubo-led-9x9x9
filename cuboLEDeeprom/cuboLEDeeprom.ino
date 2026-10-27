// programa para cubo LED 10x10x10 y lectura EEPROM

#include <EEPROM.h>
#include <SPI.h>                                          // SPI Library used to clock data out to the shift registers

#define tamanoCubo 10                                             // tamaño del cubo led
#define cantidadRegistros 13 //int((tamanoCubo*tamanoCubo)/8)+1   //cantidad de registros de 74HC595 (solo los registros que manejan los anodos), este valor se consigue encontrando el numero de lEDs en un piso,
                                                                  // en nuestro caso son (10 por 10 = 100 LEDs), 100 LEDs dividido 8 salidas por registro = 12.5 registros, entonces son 13 registros
                                                                  // estos son la cantidad de registros (bytes) necesarios para encender un solo piso independientemente
#define totalBitesPlanos 130 //cantidadRegistros*tamanoCubo       // cantidad de registros (bytes) necesarios para encender todo el cubo LED
                                                  

#define latch_pin 2                                     // Pin conocido como Latch, en el integrado es el pin 12
//#define data_pin 11                                     // Pin conocido como data, en el integrado es el pin 14
//#define clock_pin 13                                    // Pin conocido como Clock, en el integrado es el pin 13

#define data_pin 51// used by SPI, must be pin 11
#define clock_pin 52// used by SPI, must be 13

unsigned long inicio;                                  // variable para almacenar variables de tiempo

byte catodos[tamanoCubo*2];                              // vectores para la secuencia de encendido de los pisos     

byte matrizLED[totalBitesPlanos];                        // matriz donde se guardaràn toda los datos del cubo LED, 

int registros=0;                                       // variable para guardar los datos de los registros para la multiplexion de los pisos
int nivel=0;                                           // variable para almacenar el piso o nivel a multiplexar
int eprom = 0;                                       // creacion de la variable eprom
void setup() {
  
  EEPROM.get(0,eprom);                              // se obtiene el valor de la posicion 0 y se guarda en la variable eprom
  EEPROM.put( 0, eprom+1);                         // Se Graba el valor de la variable eprom+1 en la posicion cero del registro
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
  Serial.begin(9600);
  EEPROM.get(0,eprom);
}


void loop() {                                           // programa principal

  if(eprom == 1)
  logoCelsia();

  if(eprom == 2)
    desplazamientoLogo();
      
  if(eprom == 3)
    esferaCascaraAleatoria();

  if(eprom == 4)
     esferaCascara();

  if(eprom == 5)
    lluvia();

 if(eprom == 6)
   fireworks();

 if(eprom == 7)
   cuboDeslizante();

 if(eprom == 8){
     cuenta();
     letras();
     esferaCascaraAleatoria();
     cubo1();   
     aleatorio();
     planos();  
     flash(); 
     esferaCentro();      
     animacionSeno1();
     animacionSeno2();
     esferaCascara();
     snake();
     rotacion();
     lluvia();
     folder();
     apagar();
     bolas();
     esferaInversa();
     fireworks();
    effect_axis_updown_randsuspend(3, 50,1000,0);
    effect_axis_updown_randsuspend(3, 50,1000,1);
    effect_axis_updown_randsuspend(3, 50,1000,0);
    effect_axis_updown_randsuspend(3, 50,1000,1);
    effect_axis_updown_randsuspend(1, 50,1000,0);
    effect_axis_updown_randsuspend(1, 50,1000,1);
    effect_axis_updown_randsuspend(2, 50,1000,0);
    effect_axis_updown_randsuspend(2, 50,1000,1);
    cuboMovil();
    cuboDeslizante();
    esfera();
 }
   if(eprom >= 9)              // si la memoria eprom es mayor o igual a 7  // se reinicia. 
     EEPROM.put( 0, 0);
}

void LED(int Z, int Y, int X, byte intensidad){         // funcion para asignaciòn de BITs en el cubo LED.     LED(Z,Y,X, 1 o 0)

  // el cubo led tiene coordenadas que inician en el LED (0,0,0) y llega hasta el led (9,9,9)
  
  if(Z < 0)                                              // la posicion Z solo puede estar entre los valores 0 y 9 
    Z=0;
  if(Z>tamanoCubo-1)
    Z=tamanoCubo-1;
  
  if(Y < 0)                                              // la posicion Y solo puede estar entre los valores 0 y 9 
    Y=0;
  if(Y>tamanoCubo-1)
    Y=tamanoCubo-1;
  
  if(X < 0)                                             // la posicion X solo puede estar entre los valores 0 y 9
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
  while(millis()-inicio< 3000){
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
  while(millis()-inicio< 3000){                   // desplazamiento en el eje Y
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
  while(millis()-inicio< 3000){
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

void snake(){                                       // culebra aleatoria en el cubo LED
  
  int  select;
  int oldx[100], oldy[100], oldz[100];
  int x[100], y[100], z[100], addr, ledcount=40, direccion;
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
      
  while(millis()-inicio<4000){
    
      direccion = random(3);

      for(addr=1; addr<ledcount+1; addr++){
        LED(oldx[addr], oldy[addr],oldz[addr], 0);
        LED(x[addr], y[addr], z[addr], 1);
      }
      
      for(addr=1; addr<ledcount+1; addr++){
        oldx[addr]=x[addr];
        oldy[addr]=y[addr];
        oldz[addr]=z[addr];
      }

      delay(20);

      if(direccion==0)    // direccion x
        x[0]= x[0]+xbit;
        
      if(direccion==1)    // direccion y
        y[0]= y[0]+ybit;
        
      if(direccion==2)    // direccion z
        z[0]= z[0]+zbit;
      
      if(x[0] > tamanoCubo-1){         //  mantiene los valores dentro del tamaño del cubo en el eje X
            xbit=-1;
            x[0]=tamanoCubo-1;
      }
      if(x[0] < 0){
          xbit=1;
          x[0]=0;
      }
      
      if(y[0] > tamanoCubo-1){        //  mantiene los valores dentro del tamaño del cubo en el eje Y
          ybit=-1;
          y[0]=tamanoCubo-1;
      }
      if(y[0] < 0){
          ybit=1;
          y[0]=0;
      }
      
      if(z[0] > tamanoCubo-1){       //  mantiene los valores dentro del tamaño del cubo en el eje Z
          zbit=-1;
          z[0]=tamanoCubo-1;
      }
      if(z[0]<0){
          zbit=1;
          z[0]=0;
      }

    for(addr=ledcount; addr>0; addr--){     // 
        x[addr]=x[addr-1];
        y[addr]=y[addr-1];
        z[addr]=z[addr-1];
    }
  }
  apagar();
}

void lluvia(){                          // animacion de lluvia LED
  
  int x[64], y[64], z[64], addr, leds=64, tiempoAnimacion=20000;
  int  xold[64], yold[64], zold[64];
  
  for(addr=0; addr<leds; addr++){ 
    x[addr]=0;
    y[addr]=0;
    z[addr]=0;  
  }

  inicio=millis();
  
  while(millis()-inicio<tiempoAnimacion){

     for(int addr=0; addr<leds; addr++){
        LED(zold[addr], xold[addr], yold[addr], 0);
        if(z[addr]<0){
            LED(z[addr], x[addr], y[addr], 0);  
        } else{
            LED(z[addr], x[addr], y[addr], 1);
        }  
     }
  
     for(addr=0; addr<leds; addr++){
        xold[addr]=x[addr];
        yold[addr]=y[addr];
        zold[addr]=z[addr];
     } 
  
    delay(50);

     for(addr=0; addr<leds; addr++){
        
        z[addr] = z[addr]-1;

        if(z[addr]< random(-(tiempoAnimacion-(millis()-inicio)),0) ){
              x[addr]=random(tamanoCubo);
              y[addr]=random(tamanoCubo);    
              z[addr]=tamanoCubo-1; 
         
        }
      }
   }
}

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
  int xx, yy, zz, pullback[18], state=0;
  int folderaddr[18], LED_Old[18], oldpullback[18];
  int bot=0, top=1, right=0, left=0, back=0, front=0, side=0, side_select;
  
  folderaddr[0]=-9;
  folderaddr[1]=-8;
  folderaddr[2]=-7;
  folderaddr[3]=-6;
  folderaddr[4]=-5;
  folderaddr[5]=-4;
  folderaddr[6]=-3;
  folderaddr[7]=-2;
  folderaddr[8]=-1;
  folderaddr[9]=0;
  
  for(xx=0; xx<10; xx++){
  oldpullback[xx]=0;
  pullback[xx]=0;
  }
  
  inicio=millis();
  while(millis()-inicio<4000){ 
    
    if(top==1){
          if(side==0){
               //top to left-side
              for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(9-LED_Old[yy], yy-oldpullback[yy],xx , 0);
                    LED(9-folderaddr[yy], yy-pullback[yy],xx , 1);
                }
              }
           }
        if(side==2){
          //top to back-side
              for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(9-LED_Old[yy], xx, yy-oldpullback[yy], 0);
                    LED(9-folderaddr[yy], xx, yy-pullback[yy], 1);
                  }
              }
         }
        if(side==3){
            //top-side to front-side
              for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(9-LED_Old[9-yy], xx, yy+oldpullback[yy], 0);
                    LED(9-folderaddr[9-yy], xx, yy+pullback[yy], 1);
                }
              }
         }
         
        if(side==1){
        //top-side to right
             for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(9-LED_Old[9-yy], yy+oldpullback[yy],xx , 0);
                    LED(9-folderaddr[9-yy], yy+pullback[yy],xx , 1);
                }
              }
        }
    }//top
    
    if(right==1){
            if(side==4){
             //right-side to top
                 for(yy=0; yy<10; yy++){
                    for(xx=0; xx<10; xx++){
                    LED(yy+oldpullback[9-yy],9-LED_Old[9-yy],xx , 0);
                    LED( yy+pullback[9-yy],9-folderaddr[9-yy],xx , 1);
                    }
                 }
            }
        if(side==3){
          //right-side to front-side
           for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(xx,9-LED_Old[9-yy],yy+oldpullback[yy], 0);
                    LED(xx,9-folderaddr[9-yy], yy+pullback[yy], 1);
                }  
           }
        }
        if(side==2){
           //right-side to back-side
           for(yy=0; yy<10; yy++){
              for(xx=0; xx<10; xx++){
                  LED(xx,9-LED_Old[yy],yy-oldpullback[yy], 0);
                  LED(xx,9-folderaddr[yy], yy-pullback[yy], 1);
              }
            }
        }
        if(side==5){
        //right-side to bottom
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(yy-oldpullback[yy],9-LED_Old[yy],xx , 0);
                    LED( yy-pullback[yy],9-folderaddr[yy],xx , 1);
                }
            }
        }
    }//right
    
     if(left==1){
             if(side==4){
                 //left-side to top
                for(yy=0; yy<10; yy++){
                    for(xx=0; xx<10; xx++){
                        LED(yy+oldpullback[yy],LED_Old[9-yy],xx , 0);
                        LED( yy+pullback[yy],folderaddr[9-yy],xx , 1);
                    }
                }
             }
            if(side==3){
               //left-side to front-side
                 for(yy=0; yy<10; yy++){
                    for(xx=0; xx<10; xx++){
                        LED(xx, LED_Old[9-yy],yy+oldpullback[yy], 0);
                        LED(xx,folderaddr[9-yy], yy+pullback[yy], 1);
                    }
                 }
            }
            if(side==2){
              //left-side to back-side
                for(yy=0; yy<10; yy++){
                    for(xx=0; xx<10; xx++){
                        LED(xx, LED_Old[yy],yy-oldpullback[yy], 0);
                        LED(xx,folderaddr[yy], yy-pullback[yy], 1);
                    }
                }
            }
            if(side==5){
                //left-side to bottom
                 for(yy=0; yy<10; yy++){
                      for(xx=0; xx<10; xx++){
                          LED(yy-oldpullback[yy],LED_Old[yy],xx , 0);
                          LED( yy-pullback[yy],folderaddr[yy],xx , 1);
                      }
                 }
            }
      }//left
  
  
   if(back==1){
      if(side==1){
          //back-side to right-side
          for(yy=0; yy<10; yy++){
              for(xx=0; xx<10; xx++){
                  LED(xx, yy+oldpullback[yy],LED_Old[9-yy], 0);
                  LED(xx, yy+pullback[yy],folderaddr[9-yy], 1);
              }
          }
      }
      if(side==4){
          // back-side to top-side
          for(yy=0; yy<10; yy++){
              for(xx=0; xx<10; xx++){
                  LED(yy+oldpullback[yy],xx,LED_Old[9-yy] , 0);
                  LED( yy+pullback[yy],xx,folderaddr[9-yy] , 1);
              }
          }
      }
      if(side==5){
          // back-side to bottom
          for(yy=0; yy<10; yy++){
              for(xx=0; xx<10; xx++){
                  LED(yy-oldpullback[yy],xx,LED_Old[yy] , 0);
                  LED( yy-pullback[yy],xx,folderaddr[yy] , 1);
              }
          }
      }//state1
     if(side==0){
        //back-side to left-side
          for(yy=0; yy<10; yy++){
              for(xx=0; xx<10; xx++){
                  LED(xx, yy-oldpullback[yy],LED_Old[yy], 0);
                  LED(xx, yy-pullback[yy],folderaddr[yy], 1);
              }
          }
     }
   }//back

   
  if(bot==1){
        if(side==1){
          // bottom-side to right-side
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(LED_Old[9-yy], yy+oldpullback[yy],xx , 0);
                    LED(folderaddr[9-yy], yy+pullback[yy],xx , 1);
                }
            }
        }
        if(side==3){
              //bottom to front-side
              for(yy=0; yy<10; yy++){
                  for(xx=0; xx<10; xx++){
                      LED(LED_Old[9-yy], xx, yy+oldpullback[yy], 0);
                      LED(folderaddr[9-yy], xx, yy+pullback[yy], 1);
                  }
               }
         }
      if(side==2){
        //bottom to back-side
        for(yy=0; yy<10; yy++){
            for(xx=0; xx<10; xx++){
                LED(LED_Old[yy], xx, yy-oldpullback[yy], 0);
                LED(folderaddr[yy], xx, yy-pullback[yy], 1);
            }
        }
      }
      if(side==0){
        //bottom to left-side
        for(yy=0; yy<10; yy++){
            for(xx=0; xx<10; xx++){
                LED(LED_Old[yy], yy-oldpullback[yy],xx , 0);
                LED(folderaddr[yy], yy-pullback[yy],xx , 1);
            }
        }
     }
   }//bot
   
  if(front==1){
         if(side==0){
              //front-side to left-side
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(xx, yy-oldpullback[yy],9-LED_Old[yy], 0);
                    LED(xx, yy-pullback[yy],9-folderaddr[yy], 1);
                }
             }
        }
        if(side==5){
            // front-side to bottom
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(yy-oldpullback[yy],xx,9-LED_Old[yy] , 0);
                    LED( yy-pullback[yy],xx,9-folderaddr[yy] , 1);
                }
            }
        }
        if(side==4){
            // front-side to top-side
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(yy+oldpullback[yy],xx,9-LED_Old[9-yy] , 0);
                    LED( yy+pullback[yy],xx,9-folderaddr[9-yy] , 1);
                }
            }
        }
        
        if(side==1){
            //front-side to right-side
            for(yy=0; yy<10; yy++){
                for(xx=0; xx<10; xx++){
                    LED(xx, yy+oldpullback[yy],9-LED_Old[9-yy], 0);
                    LED(xx, yy+pullback[yy],9-folderaddr[9-yy], 1);
                }
            }
        }
    }//front

  delay(20);//               DELAY   DELAY  DELAY
  for(xx=0; xx<10; xx++){
      LED_Old[xx]=folderaddr[xx];
      oldpullback[xx]=pullback[xx];
  }
  

  if(folderaddr[9]==9){
   // pullback=8;
              for(zz=0; zz<10; zz++)
                 pullback[zz] = pullback[zz]+1;
           
             if(pullback[9]==10){//finished with fold
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
                    
                          for(xx=0; xx<10; xx++){
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
                            folderaddr[9]=0;
            }//pullback
  }//folderaddr   

  if(folderaddr[9]!=9)
    for(zz=0; zz<10; zz++)
    folderaddr[zz] = folderaddr[zz]+1;
  
  }//while
  
}//folder


void bolas(){                  //Dos bolas rebotan dentro de cubo de manera aleatoria
  int xxx=0, yyy=0, zzz=0;
  int fx=random(tamanoCubo-1), fy=random(tamanoCubo-1), fz=random(tamanoCubo-1), direct, fxm=1, fym=1, fzm=1, fxo=0, fyo=0, fzo=0;
  int  ftx=random(tamanoCubo-1), fty=random(tamanoCubo-1), ftz=random(tamanoCubo-1), ftxm=1, ftym=1, ftzm=1, ftxo=0, ftyo=0, ftzo=0;
  int select, intensidad1, intensidad2;
  int tiempo = 30;
  apagar();
    
  intensidad1=1;
  intensidad2=1;
     
  inicio=millis();
      
  while(millis()-inicio<5000){
  
        //fx=random(8); fy=random(8); fz=random(8);
    
        LED(fxo, fyo, fzo, 0);                       // BOLA 1 antigua apagar
        LED(fxo, fyo, fzo+1, 0);
        LED(fxo, fyo, fzo-1, 0);
        LED(fxo+1, fyo, fzo, 0);
        LED(fxo-1, fyo, fzo, 0);
        LED(fxo, fyo+1, fzo, 0);
        LED(fxo, fyo-1, fzo, 0);
        
        LED(ftxo, ftyo, ftzo, 0);                   // BOLA 2 antigua apagar
        LED(ftxo, ftyo, ftzo+1, 0);
        LED(ftxo, ftyo, ftzo-1, 0);
        LED(ftxo+1, ftyo, ftzo, 0);
        LED(ftxo-1, ftyo, ftzo, 0);
        LED(ftxo, ftyo+1, ftzo, 0);
        LED(ftxo, ftyo-1, ftzo, 0);
    
        LED(ftx, fty, ftz, intensidad1);            // BOLA 1 nueva encender   
        LED(ftx, fty, ftz+1, intensidad1);
        LED(ftx, fty, ftz-1,  intensidad1);
        LED(ftx+1, fty, ftz, intensidad1);
        LED(ftx-1, fty, ftz, intensidad1);
        LED(ftx, fty+1, ftz, intensidad1);
        LED(ftx, fty-1, ftz, intensidad1);     
        
        LED(fx, fy, fz, intensidad2);                // BOLA 2 nueva encender   
        LED(fx, fy, fz+1, intensidad2);
        LED(fx, fy, fz-1, intensidad2);
        LED(fx+1, fy, fz, intensidad2);
        LED(fx-1, fy, fz, intensidad2);
        LED(fx, fy+1, fz, intensidad2);
        LED(fx, fy-1, fz, intensidad2);  
        
        
         delay(tiempo);
         
        fxo=fx;            // posicion antigua = posicion nueva   BOLA 1
        fyo=fy;
        fzo=fz; 
        
        ftxo=ftx;         // posicion antigua = posicion nueva   BOLA 2
        ftyo=fty;
        ftzo=ftz; 
     
        direct=random(3);    // definiciòn de la direccion aleatoria BOLa 1
        
        if(direct==0)
            fx= fx+fxm;
        if(direct==1)
            fy= fy+fym;  
        if(direct==2)
            fz= fz+fzm;  
     
        if(fx<0){           // no violar los limites del cubo
          fx=0; fxm=1;}
        if(fx>tamanoCubo-1){
          fx=tamanoCubo-1; fxm=-1;}  
        if(fy<0){
          fy=0; fym=1;}
        if(fy>tamanoCubo-1){
          fy=tamanoCubo-1; fym=-1;}    
        if(fz<0){
          fz=0; fzm=1;}
        if(fz>tamanoCubo-1){
          fz=tamanoCubo-1; fzm=-1;}  
      
          direct=random(3);     // definiciòn de la direccion aleatoria BOLa 2
          
        if(direct==0)
        ftx= ftx+ftxm;
        if(direct==1)
        fty= fty+ftym;  
        if(direct==2)
        ftz= ftz+ftzm;  
        
        if(ftx<0){              // no violar los limites del cubo
          ftx=0; ftxm=1;}
        if(ftx>tamanoCubo-1){
          ftx=tamanoCubo-1; ftxm=-1;}  
        if(fty<0){
          fty=0; ftym=1;}
        if(fty>tamanoCubo-1){
          fty=tamanoCubo-1; ftym=-1;}    
        if(ftz<0){
          ftz=0; ftzm=1;}
        if(ftz>tamanoCubo-1){
          ftz=tamanoCubo-1; ftzm=-1;} 
  }//while
  apagar();
}//wipeout

void fireworks(){      //fuegos pirotecnicos
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
      LED(e,origin_x,origin_y,1);
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

        LED(particles[f][2],particles[f][0],particles[f][1],1);

      }

      delay(delay_ms);
      
      for (f=0; f<n; f++)
      {
        LED(particles[f][2],particles[f][0],particles[f][1],0);
      }      
    }
  }
}

void effect_axis_updown_randsuspend (char axis, int delay_ms, int sleep_m, int invert)
{
  unsigned char positions[100];
  unsigned char destinations[100];

  int i,px;
  
  for (i=0; i<100; i++)
  {
    positions[i] = 0; // Set all starting positions to 0
    destinations[i] = rand()%9;
  }

    // Loop 8 times to allow destination 7 to reach all the way
  for (i=0; i<tamanoCubo; i++)
  {
        // For every iteration, move all position one step closer to their destination
    for (px=0; px<tamanoCubo*tamanoCubo; px++)
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
  for (i=0; i<tamanoCubo*tamanoCubo; i++)
  {
    destinations[i] = tamanoCubo-1;
  }
  
    // Suspend the positions in mid-air for a while
  delay(sleep_m);
  
    // Then do the same thing one more time
  for (i=0; i<tamanoCubo; i++)
  {
    for (px=0; px<tamanoCubo*tamanoCubo; px++)
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

void draw_positions_axis (char axis, unsigned char positions[100], int invert)
{
  int x, y, p;
  
  apagar();
  
  for (x=0; x<tamanoCubo; x++)
  {
    for (y=0; y<tamanoCubo; y++)
    {
      if (invert)
      {
        p = (9-positions[(x*tamanoCubo-1)+y]);
      } else
      {
        p = positions[(x*tamanoCubo-1)+y];
      }
    
      if (axis == 3)
        LED(p,x,y,1);
        
      if (axis == 2)
        LED(y,x,p,1);
        
      if (axis == 1)
        LED(x,p,y,1);
    }
  }
  
}


void cuboMovil(){   // cubo que se agranda y se reduce
 
  //int tamCubo = 3;
  int tiempo = 100;
  int desp = 2;
  int tamY = 9;   // 9 - 0
  int tamZ = 0;   // 0 - 9
  int tamX = 0;   // 9 - 0

 for(int tamCubo=0;tamCubo < tamanoCubo;tamCubo++){
    for(int j=0;j<=tamCubo;j++){
        LED(tamZ,tamY,tamX+j,1);
        LED(tamZ,tamY-j,tamX+tamCubo,1);
        LED(tamZ,tamY-tamCubo,tamX+j,1);
        LED(tamZ,tamY-j,tamX,1);
        LED(j+tamZ,tamY,tamX,1);
        LED(j+tamZ,tamY-tamCubo,tamX,1);
        LED(j+tamZ,tamY,tamX+tamCubo,1);
        LED(j+tamZ,tamY-tamCubo,tamX+tamCubo,1);
        LED(tamCubo+tamZ,tamY,tamX+j,1);
        LED(tamCubo+tamZ,tamY-j,tamX+tamCubo,1);
        LED(tamCubo+tamZ,tamY-tamCubo,tamX+j,1);
        LED(tamCubo+tamZ,tamY-j,tamX,1);
    }
    delay(tiempo);
    apagar();
  }

 for(int tamCubo = tamanoCubo-1; tamCubo > 0; tamCubo--){
    for(int j=0;j<=tamCubo;j++){
        LED(tamZ,tamY,tamX+j,1);
        LED(tamZ,tamY-j,tamX+tamCubo,1);
        LED(tamZ,tamY-tamCubo,tamX+j,1);
        LED(tamZ,tamY-j,tamX,1);
        LED(j+tamZ,tamY,tamX,1);
        LED(j+tamZ,tamY-tamCubo,tamX,1);
        LED(j+tamZ,tamY,tamX+tamCubo,1);
        LED(j+tamZ,tamY-tamCubo,tamX+tamCubo,1);
        LED(tamCubo+tamZ,tamY,tamX+j,1);
        LED(tamCubo+tamZ,tamY-j,tamX+tamCubo,1);
        LED(tamCubo+tamZ,tamY-tamCubo,tamX+j,1);
        LED(tamCubo+tamZ,tamY-j,tamX,1); 
    }
    delay(tiempo);
    apagar();
  }


 for(int tamCubo=0;tamCubo < tamanoCubo;tamCubo++){
    for(int j=0;j<=tamCubo;j++){
        LED(0,0,j,1);
        LED(0,j,0,1);
        LED(j,0,0,1);
        LED(tamCubo,0,j,1);
        LED(tamCubo,j,0,1);
        LED(0,tamCubo,j,1);
        LED(j,tamCubo,0,1);
        LED(0,j,tamCubo,1);
        LED(j,0,tamCubo,1);
        
        LED(tamCubo,j,tamCubo,1);
        LED(j,tamCubo,tamCubo,1);
        LED(tamCubo,tamCubo,j,1);
      
    }
    delay(tiempo);
    apagar();
  }

 for(int tamCubo = tamanoCubo-1; tamCubo > 0; tamCubo--){
    for(int j=0;j<=tamCubo;j++){
        LED(0,0,j,1);
        LED(0,j,0,1);
        LED(j,0,0,1);
        LED(tamCubo,0,j,1);
        LED(tamCubo,j,0,1);
        LED(0,tamCubo,j,1);
        LED(j,tamCubo,0,1);
        LED(0,j,tamCubo,1);
        LED(j,0,tamCubo,1);
        
        LED(tamCubo,j,tamCubo,1);
        LED(j,tamCubo,tamCubo,1);
        LED(tamCubo,tamCubo,j,1);
      
    }
    delay(tiempo);
    apagar();
  }
}

void cuboDeslizante(){
  int tiempo = 100;
  int tamCubo=9;
  
  byte M[10][10];
  byte M_ant[10][10];
   
      for(int y=0;y<=tamCubo;y++){    // creacion de los bordes 
        for(int x=0;x<=tamCubo;x++){
            M[x][y]=0;
            
           if(x == 0)
             M[x][y]=1;
             
           if(y == 0)
             M[x][y]=1;
             
           if(x == tamCubo)
             M[x][y]=1;
           
           if(y == tamCubo)
             M[x][y]=1;
        }
      }

       for(int desp = 0; desp<=tamCubo;desp++){   // llenado y vaciado del cubo 
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
              
               LED(desp,y,x,M[x][y]);
            }
          }
         delay(tiempo);
      }
      for(int desp = 0; desp<tamCubo;desp++){
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
              
               LED(desp,y,x,0);
            }
          }
         delay(tiempo);
      }
      
    delay(50);
    apagar();

      for(int desp = 0; desp<=tamCubo;desp++){   // deslizamiento del cubo a la siguiente cara del cubo
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
               if(y-desp >= 0)
               LED(9,y-desp,x,M[x][y]);
               
               if(9+(y-desp) < 9)
               LED(9+(y-desp),0,x,M[x][y]);
            }
          }
         delay(tiempo);
         if(desp < tamCubo)
         apagar();
      }
 
     delay(tiempo+1000);
 

    for(int desp = 0; desp<=tamCubo;desp++){   // llenado y vaciado del cubo 
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
              
               LED(y,desp,x,M[x][y]);
            }
          }
         delay(tiempo);
      }
      for(int desp = 0; desp<tamCubo;desp++){
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
              
               LED(y,desp,x,0);
            }
          }
         delay(tiempo);
      }
     
    delay(50);
    apagar();

      for(int desp = 0; desp<=tamCubo;desp++){   // deslizamiento del cubo a la siguiente cara del cubo
      
          for(int y=0;y<=tamCubo;y++){
            for(int x=0;x<=tamCubo;x++){
               if(y+desp < 9)
               LED(y+desp,9,x,M[x][y]);
               
               if(9+(y-desp) <= 9)
               LED(tamCubo,9+(y-desp),x,M[x][y]);
            }
          }
         delay(tiempo);
         if(desp < tamCubo)
         apagar();
      }
 
     delay(tiempo+1000);
}

void esfera(){       // generar una esfera aleatoria 
  
  int tiempo = 30;
  double scala = 0.5;
  int tamCubo = 9;
  
  inicio=millis();
  while(millis()-inicio<5000){
    int xc = random(0,10);
    int yc = random(0,10);
    int zc = random(0,10);
    for(int r=1;r<=18;r++){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r)){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      delay(tiempo);
    }
  }
}

void esferaCentro(){       // generar una esfera aleatoria 
  int tiempo = 30;
  float xc = 4.5;
  float yc = 4.5;
  float zc = 4.5;
  double scala = 0.5;
  int tamCubo = 9;
  
  inicio=millis();    
  while(millis()-inicio<5000){
    for(int r=1;r<=18;r++){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r)){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      
      delay(tiempo);
     
    }
    
    for(int r=18;r>=1;r--){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r)){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      
      delay(tiempo);
     
    }
  }
}

void esferaInversa(){       // generar una esfera aleatoria 
  int tiempo = 50;

  double scala = 0.5;
  int tamCubo = 9;

  inicio=millis();
  while(millis()-inicio<5000){
    int xc = random(0,10);
    int yc = random(0,10);
    int zc = random(0,10);
    for(int r=1;r<=26;r++){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) >= (scala*r)*(scala*r)){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      delay(tiempo);
      //apagar();
    }
  }
}

void esferaCascara(){       // generar una esfera aleatoria 
  int tiempo = 50;
  float xc = 4.5;
  float yc = 4.5;
  float zc = 4.5;
  double scala = 0.5;
  int tamCubo = 9;

  inicio=millis();
  while(millis()-inicio<5000){
    for(int r=1;r<=22;r++){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r) && (x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) >= ((scala-0.1)*r)*((scala-0.1)*r) ){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      
      delay(tiempo);
    }
    
    for(int r=22;r>=1;r--){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
    
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r) && (x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) >= ((scala-0.1)*r)*((scala-0.1)*r) ){
              LED(z,y,x,1);
            }
            else{
                LED(z,y,x,0);
            }
            
          }
        }
      }
      
      delay(tiempo);
    }
  }
}

void esferaCascaraAleatoria(){       // generar una esfera aleatoria 
  int tiempo = 30;
 
  double scala = 0.5;
  int tamCubo = 9;
  
  inicio=millis();
  while(millis()-inicio<5000){
     int xc = random(0,10);
     int yc = random(0,10);
     int zc = random(0,10);
  
    for(int r=1;r<=22;r++){  
      for(int z=0;z<=tamCubo;z++){
        for(int y=0;y<=tamCubo;y++){
          for(int x=0;x<=tamCubo;x++){
            
            if((x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) <= (scala*r)*(scala*r) && (x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc) >= ((scala-0.1)*r)*((scala-0.1)*r) ){ 
                LED(z,y,x,1);
            }
           else{
                LED(z,y,x,0);
           }
          }
        }
      }
      delay(tiempo);
    }
  }
   
}

char caracteres[128][8] = {
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

void cuenta(){ // cuenta regresiva
  char numeros[10] = {'0','1','2','3','4','5','6','7','8','9'};
  for(int cuenta = 9; cuenta >= 0; cuenta--){
    for(int desp=46;desp >= 0;desp--){
       for(int y=0;y<8;y++){ 
          for(int z = 0; z<8; z++){
            // if( desp > 38 && desp <=28){ 
               if( y+desp-36 >= 0 && y+desp-36 < 10){
                LED(z,y+desp-36,0,bitRead(caracteres[numeros[cuenta]][8-z], 6-y));         
               }  
               if( y+desp-26 >= 0 && y+desp-26 < 10){
                LED(z,0,9-(y+desp-26),bitRead(caracteres[numeros[cuenta]][8-z],6-y));       
               } 
               if( y+desp-16 >= 0 && y+desp-16 < 10){
                LED(z,9-(y+desp-16),9,bitRead(caracteres[numeros[cuenta]][8-z], 6-y));            
               }
               if( y+desp-6 >= 0 && y+desp-6 < 10){
                LED(z,9,y+desp-6,bitRead(caracteres[numeros[cuenta]][8-z],6-y));   
               } 
          }  
       }      
       delay(50);
       apagar();
    }
  }
}

void letras(){ // cuenta regresiva
  char string[4] = {'H','O','L','A'};
    for(int cuenta=0;cuenta<4;cuenta++){
       
       for(int j = 0; j<tamanoCubo; j++){
          for(int i=0;i<9;i++){ 
                for(int bitR=0;bitR<9;bitR++){
                      LED(8-i,8-bitR,j,bitRead(caracteres[string[cuenta]][i], bitR));
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
       for(int j = 0; j<tamanoCubo; j++){
          for(int i=0;i<9;i++){ 
            for(int bitR=0;bitR<9;bitR++){
                  LED(8-i,j,bitR+1,bitRead(caracteres[string[cuenta]][i], bitR));
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
  int tiempo = 100;
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
           for(int j = 0; j<=5; j++){                   // deslizamiento de los numeros por el eje X hasta la mitad del cubo
                for(int i=0;i<9;i++){ 
                      for(int bitR=0;bitR<9;bitR++){
                            LED(8-i,8-bitR,j,bitRead(caracteres[k][i], bitR));
                             if (j == 4){
                                matrixA[cuenta]=8-i;
                                matrixB[cuenta]=8-bitR;
                                matrixC[cuenta]=j;
                                matrixD[cuenta]=bitRead(caracteres[k][i], bitR);
                                cuenta++;
                             }
    
                      } 
                }
                delay(tiempo);
                 if (j!= 5){
                   apagar(); 
                 }
           }   
       delay(tiempo);
       apagar();
       
       for(int j=0;j<8;j++){                            //rotacion en el eje Z
         angle=angle-pi/16;
           for(int i=0;i<cuenta;i++){
            xr=0;
            yr=0;
              if(matrixD[i]==1){
                xr=(matrixB[i]-5)*cos(angle)-(matrixC[i]-5)*sin(angle);
                yr=(matrixB[i]-5)*sin(angle)+(matrixC[i]-5)*cos(angle);
                LED(matrixA[i],xr+5,yr+5,1);
              }
              matA[i]=matrixA[i];         
              matB[i]=xr+5;   
              matC[i]=yr+5;
              matD[i]=matrixD[i];
            }
      
           delay(60);
           if (j!= 7){
              apagar();
           }
       }
       
       delay(2*tiempo);
       apagar();
       
       for(int j=0;j<6;j++){                   // desplazamiento del cubo sobre el eje Y
          for(int i=0;i<cuenta;i++){
         
             if(matD[i]==1){
               
                LED(matA[i],5-j,matC[i],15);
               
              }
             
          }
              delay(tiempo);
              if (j!= 5){
               apagar();
              }
       }
      delay(tiempo*3);
      apagar();
  }
}

  int celsia[10] = { 72,  204,  462,  204,  717,  636,  891,  438,  252,  120 };
            
                //              |    *  *    |  0x48
                //              |   **  **   |  0xCC
                //              |  ***  ***  |  0x1CE
                //              |   **  **   |  0xCC
                //              | * **  ** * |  0x2CD
                //              | *  ****  * |  0x249
                //              | ** **** ** |  0x37B
                //              |  ** ** **  |  0x1B6
                //              |   ******   |  0xB4
                //              |    ****    |  0x78


void logoCelsia(){ // cuenta regresiva

       for(int z = 0; z<10; z++){
          for(int y=0; y<10;y++){ 

                LED(9-z,y,3,bitRead(celsia[z], y));  
                LED(9-z,y,4,bitRead(celsia[z], y));
                LED(9-z,y,5,bitRead(celsia[z], y));
          }  
       }      
       delay(5000);
       apagar();
   
}

void desplazamientoLogo(){  // probando la matriz rotacional en 2 dimensiones.
  int tiempo=100;
      for(int desp = -2; desp<10;desp++){
         for(int z = 0; z<10; z++){
            for(int y=0; y<10;y++){ 
  
                  LED(9-z,y,desp,bitRead(celsia[z], y));  
                  LED(9-z,y,desp+1,bitRead(celsia[z], y));
                  LED(9-z,y,desp+2,bitRead(celsia[z], y));
            }  
         }      
         delay(tiempo);
         apagar();
      }
      for(int desp = 8; desp > -3;desp--){
         for(int z = 0; z<10; z++){
            for(int y=0; y<10;y++){ 
  
                  LED(9-z,y,desp,bitRead(celsia[z], y));  
                  LED(9-z,y,desp+1,bitRead(celsia[z], y));
                  LED(9-z,y,desp+2,bitRead(celsia[z], y));
            }  
         }      
         delay(tiempo);
         apagar();
      }
      apagar();
      for(int desp = -2; desp<10;desp++){
         for(int z = 0; z<10; z++){
            for(int y=0; y<10;y++){ 
  
                  LED(9-z,desp,y,bitRead(celsia[z], y));  
                  LED(9-z,desp+1,y,bitRead(celsia[z], y));
                  LED(9-z,desp+2,y,bitRead(celsia[z], y));
            }  
         }      
         delay(tiempo);
         apagar();
      }
      for(int desp = 8; desp > -3;desp--){
         for(int z = 0; z<10; z++){
            for(int y=0; y<10;y++){ 
  
                  LED(9-z,desp,y,bitRead(celsia[z], y));  
                  LED(9-z,desp+1,y,bitRead(celsia[z], y));
                  LED(9-z,desp+2,y,bitRead(celsia[z], y));
            }  
         }      
         delay(tiempo);
         apagar();
      }
}
