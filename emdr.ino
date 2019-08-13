// Module for EMDR Device (c) 2019 mrkslack <mrkslack@gmail.com>
// released under the GPLv3 license
// version 1.0

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define STRIP_PIN           10  // pin used to control neo-pixel
#define BTTX_PIN            9   // pin linked to HC-06 RX
#define BTRX_PIN            8   // pin linked to HC-06 TX

#define NP_FIRST     2      // first led to use in strip
#define NP_LAST      47     // last led to use in strip 
#define WAKE_LENGTH  3      // number of led in wake

#define INWARD      -1
#define OUTWARD     1

#define SLOWEST     200     //  max delay in main loop
#define FASTEST     10      //  min delay in main loop


//  commands sent through bluetooth 
#define  BT_START         105
#define  BT_STOP          115 
#define  BT_CHECK_STATUS  120
#define  BT_CHECK_SPEED   121
#define  BT_ERROR         122


SoftwareSerial bt(BTRX_PIN, BTTX_PIN); // RX, TX
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NP_LAST+1, STRIP_PIN, NEO_GRB + NEO_KHZ800);

int status; // 1=ON 0=OFF
int ms_delay;  // from SLOWEST to FASTEST;
int direction;  // 1=OUTWARD -1=INWARD
int np_index;  // actual position in strip
int wake[WAKE_LENGTH] = {255, 120, 50}; // intensity value for wake. Must be (WAKE_LENGTH) values !!!
int last_pixel;  // led to clear

void setup() { 
  pinMode(STRIP_PIN, OUTPUT); 
     
  bt.begin(9600);
  bt.write("AT+NAMEEMDR");   // set the bluetooth name of device to 'EMDR'
  
  strip.begin();
  
//  Serial.begin(9600);
  ms_delay = (int)((SLOWEST-FASTEST)/2)+FASTEST;

  stop();
} 

void loop() {

 if(status) {
   if(direction==OUTWARD) {
     if(np_index == NP_LAST){
       np_index = NP_LAST-1;
       direction = INWARD;
     }
     else
       np_index++;
    }  
    else {
      if(np_index == NP_FIRST){
        np_index = NP_FIRST + 1;
        direction = OUTWARD;
      }
      else      
        np_index--;
    }
     
    //  clear last wake's led
    strip.setPixelColor(last_pixel, 0, 0, 0);      

    for(int i=0;i<WAKE_LENGTH;i++) {
      strip.setPixelColor(np_index-(direction*i), 0, wake[i], 0);    
    }
    // next led to be clear 
    last_pixel = np_index-(direction*(WAKE_LENGTH-1));  

    strip.show();
  }  
    
  if(readBT() == 0)  // if incoming command no need to delay!
    delay(ms_delay);
}

// convert speed value (1-100) to ms for delay
int convertSpeed2Ms(int speed) {
  speed = 101-speed;
  float f = SLOWEST - FASTEST;
  return (int)(f*speed/100)+FASTEST;
}

// convert delay ms to speed (1-100)
int convertMs2Speed(int ms) {
  float f = SLOWEST - FASTEST;
  int speed = (int)(((ms-FASTEST)*100)/f)-FASTEST;
  return 101-speed;
}

void stop(){
  status = 0;
  strip.clear();
  strip.setPixelColor(NP_FIRST, 64, 128, 64);    
  strip.show();
}

void start() {
  status = 1;
  last_pixel = NP_FIRST;
  np_index = last_pixel-1;
  direction = OUTWARD;
  strip.clear();
}

// read command from bluetooth
int readBT() {
  int value = 0;
  while(bt.available())
  // if text arrived in from BT serial...
  {
    value=(bt.read());
    if (value==BT_START)   // go!
    {
      bt.write(value); 
      start();
    }
    else if (value==BT_STOP) // stop
    {
      bt.write(value); 
      stop();
    }
    else if (value>0 && value<101) // adjust speed
    {
      bt.write(value); 
      ms_delay = convertSpeed2Ms(value);
    } 
    else if (value==BT_CHECK_STATUS) // check status
    {
      if(status)
        bt.write(BT_START); 
      else 
        bt.write(BT_STOP);
      value = 0;
    }
    else if (value==BT_CHECK_SPEED) // check speed
    {
        bt.write(convertMs2Speed(ms_delay)); 
        value = 0;
    }
    else {
      bt.write(BT_ERROR);  
//    Serial.print(value);
      value = 0;
    }
  }
  return value;
}
