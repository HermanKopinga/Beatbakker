#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <Bounce.h>

#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/

// Beatbakker, based on Teensy audio example.
// Hardware: Teensy 3.1 with Teensy audio shield
// by: Herman Kopinga   // Update all the button objects

AudioPlayMemory    sound0;
AudioPlayMemory    sound1;  // two memory players
AudioMixer4        mix1;    // one 4-channel mixer
AudioOutputI2S     headphones;
AudioOutputAnalog  dac;     // play to both I2S audio board and on-chip DAC


AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(mix1, 0, headphones, 0);
AudioConnection c4(mix1, 0, headphones, 1);
AudioConnection c10(mix1, 0, dac, 0);

AudioControlSGTL5000 audioShield;

// Bounce objects to read six pushbuttons (pins 0-3)
//
Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 5 ms debounce time
Bounce button2 = Bounce(2, 15);
Bounce button3 = Bounce(3, 15);

// Optional options:
//
// Accent

boolean debug = 1;

int bpm = 127;
int currentBeat = 0;
int bassPerBeat = 1;
int beatLength = 60000/bpm;
boolean bassSwitches[4];
boolean snareSwitches[4];

int snareOffset = 0;

unsigned long currentMilli = 0;
unsigned long nextBass = 2000;
unsigned long nextSnare = 2000;
unsigned long nextBeat = 2000;

// To average the two pots.
//
const int numReadings = 10;
const int analogThreshold = 25;
int readings0[numReadings];      // the readings from the analog input
int readings1[numReadings];      // the readings from the analog input
int arindex = 0;                  // the index of the current reading
int total0 = 0;                  // the running total
int average0 = 0;                // the average
int total1 = 0;                  // the running total
int average1 = 0;                // the average
int analog0 = 0;
int analog1 = 0;
int analogStored0 = 0;
int analogStored1 = 0;

void setup () {
  if (debug) Serial.begin(115200);
  
  // Configure the pushbutton pins for pullups.
  // Each button should connect from the pin to GND.
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // turn on the output
  audioShield.enable();
  audioShield.volume(0.2);

  // by default the Teensy 3.1 DAC uses 3.3Vp-p output
  // if your 3.3V power has noise, switching to the
  // internal 1.2V reference can give you a clean signal
  //dac.analogReference(INTERNAL);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping
  mix1.gain(0, 0.4);
  mix1.gain(1, 0.4);  
  bassSwitches[0] = 1;
  bassSwitches[1] = 1;
  bassSwitches[2] = 0;
  bassSwitches[3] = 1;  
   
}

// op de milliseconde getimed.

void loop () {
  // subtract the last reading:
  total0 = total0 - readings0[arindex];         
  total1 = total1 - readings1[arindex];         

  // read from the sensor:  
  readings0[arindex] = analogRead(A2); 
  readings1[arindex] = analogRead(A3); 

  // add the reading to the total:
  total0= total0 + readings0[arindex];       
  total1= total1 + readings1[arindex];       
  // advance to the next position in the array:  
  arindex++;                    

  // if we're at the end of the array...
  if (arindex >= numReadings) {      
    // ...wrap around to the beginning: 
    arindex = 0;              
  }    

  // calculate the average:
  analog0 = total0 / numReadings;        
  analog1 = total1 / numReadings;         

  // Update all the button objects
  button0.update();
  button1.update();
  button2.update();
  button3.update();
  if (button0.fallingEdge()) {
    bassSwitches[0] = !bassSwitches[0];
  }
  if (button1.fallingEdge()) {
    bassSwitches[1] = !bassSwitches[1];
  }
  if (button2.fallingEdge()) {
    bassSwitches[2] = !bassSwitches[2];
  }
  if (button3.fallingEdge()) {
    bassSwitches[3] = !bassSwitches[3];
    Serial.println("4");
  }


  if (currentMilli != millis()){
    currentMilli = millis();
    
    if (nextBeat <= millis()) {
      nextBeat = nextBeat + beatLength;
      currentBeat++;
      if (currentBeat == 4) {
        currentBeat = 0;
      }      
    } 
    
    if (analog0 - analogStored0 >= analogThreshold || analogStored0 - analog0 >= analogThreshold) {
      if (debug) Serial.println(analog0);  
      snareOffset = analogStored0 = analog0;
      snareOffset = map(snareOffset, 0, 1024, 0, 100);
      snareOffset = beatLength * snareOffset / 100;
    }
    
    if (analog1 - analogStored1 >= analogThreshold || analogStored1 - analog1 >= analogThreshold) {
      if (debug) Serial.println(analog1);
      bpm = analogStored1 = analog1;
      bpm = map(bpm, 0, 1024, 60, 320);
      beatLength = 60000/bpm;
      //snareOffset = map
    }
    
/*    if (bassPerBeat) {
      bassPerBeat = switchPos;
    }*/
    
    if (nextBass <= millis() && bassSwitches[currentBeat]) {
      if (debug) Serial.print("Bass ");          
      sound0.play(AudioSampleKick);
      nextBass = nextBass + beatLength / bassPerBeat;
      if (debug) Serial.print(currentMilli);
      if (debug) Serial.print(" ");
      if (debug) Serial.print(nextBass);
      if (debug) Serial.print(" ");
      if (debug) Serial.println(beatLength);
    }
    
    if (nextSnare <= millis()) {
      if (debug) Serial.print("Snar ");                
      sound1.play(AudioSampleSnare);
      nextSnare = nextBeat + snareOffset;
      if (debug) Serial.print(currentMilli);
      if (debug) Serial.print(" ");
      if (debug) Serial.print(nextSnare);
      if (debug) Serial.print(" ");
      if (debug) Serial.println(beatLength);      
    }
  }
}
