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
    fireworks();
    aleatorio();
    letras();
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
    
    linespin(1500,5);
//apagar();
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



void letras(){ // cuenta regresiva
    for(int cuenta=0;cuenta<3;cuenta++){
       
       for(int j = 0; j<9; j++){
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){
                        LED(8-i,8-bitR,j,bitRead(numeros[cuenta][i], bitR));
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
    apagar();
   for(int cuenta=0;cuenta<10;cuenta++){
       
       for(int j = 0; j<9; j++){
            for(int i=0;i<9;i++){ 
                  for(int bitR=0;bitR<9;bitR++){
                        LED(8-i,j,bitR,bitRead(numeros[cuenta][i], bitR));
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
