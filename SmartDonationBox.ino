#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

char * filenames[] = {
  "applause.mp3",
  "dudeness.wav"
};
int filename_index = 0;

int apex_pin = 2;                      // what pin is the apex bill reader connect to?
int apex_interrupt = 0;                // interrupt to use when sensing a pulse
int dollar = 0;                        // what is the dollar amount that was read?
volatile unsigned long last_change = millis();  // when was the last time a pulse was received?
volatile int pulses = 0;                        // counting the pulses sent
int pulses_per_dollar = 1;             // how many pulses are sent per dollar
int done_pulsing = 20 * pulses_per_dollar * 100;
                                       // how many milliseconds after the last pulse to 
                                       // consider the bill reading done
int led_pin = 6;
volatile int checked = true;


void setup()
{
  Serial.begin(115200);                    // setup the serial port for communications with the host computer
  pinMode(apex_pin, INPUT_PULLUP);
  attachInterrupt(apex_interrupt, count_pulses, CHANGE);
  pinMode(led_pin, OUTPUT);     
  
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
  SD.begin(CARDCS);    // initialise the SD card
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(1,1);
  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
}


void count_pulses()
{
  int val = digitalRead(apex_pin);
  checked = false;

  if (val == HIGH) {
    last_change = millis();
    pulses += 1;
  }
}


void loop() 
{
  unsigned long int now = millis();

  if (((now - last_change) > done_pulsing) && ! checked) { // no pulses for more than 1/10 second
    
    dollar = pulses;    
    if( pulses != 0 ){
      reactToDonation(dollar);
    }
    
    pulses = 0;
    dollar = 0;
    checked = true;
  }
}

void reactToDonation(int dollars){
  Serial.print(dollars);
  Serial.println(" dollars detected");
  
  char * filename = NULL;
  
  // this switch does nothing
  switch(dollars){
    case 1:
      filename = getRandomSoundFileName(0);
      break;
    case 2:
      filename = getRandomSoundFileName(1);
      break;
    case 5: 
      filename = getRandomSoundFileName(2);
      break;
    case 10:
      filename = getRandomSoundFileName(3);
      break; 
    case 20:
      filename = getRandomSoundFileName(4);
      break;
    case 50:
      filename = getRandomSoundFileName(5);
      break;
    case 100:
      filename = getRandomSoundFileName(6);
      break;
  }
  
  if(filename != NULL){
    Serial.print("Playing Filename: ");
    Serial.println(filename);
    musicPlayer.playFullFile(filename);  
  }

}

char * getRandomSoundFileName(uint8_t bin){    
  int num_filenames = 2;
  filename_index++;
  if(filename_index >= num_filenames){
    filename_index = 0;
  }  
  return filenames[filename_index];  
}
