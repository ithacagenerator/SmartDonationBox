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

#define NUM_ACCEPTABLE_DENOMINATIONS (6)
uint16_t filename_index[NUM_ACCEPTABLE_DENOMINATIONS] = {0};

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
  
  char filename[32] = {0};             // allocate space for the file name
  getNextFilename(dollars, filename);  // maps dollars to denomination index
                                       // advances the pointer, and possibly wraps around
  
  if(strlen(filename) != 0){
    Serial.print("Playing Filename: ");
    Serial.println(filename);
    musicPlayer.playFullFile(filename);  
  }

}

// if the file implied by the current index for the denomination
void getNextFilename(uint8_t dollars, char * filename){
  int8_t denomination_index = dollarsToDenominationIndex(dollars);  
  boolean found_file = false;
  
  if((denomination_index >= 0) && (denomination_index < NUM_ACCEPTABLE_DENOMINATIONS)){
    // filenames always start with 'clip', followed by four digits, followed by '.wav' or '.mp3'
    // and clips are stored in folders named denom, followed by three digits (001, 005, 010, 020, 050, or 100)
    // test for the existing of the current index
    char wavname[32] = {0};
    char mp3name[32] = {0};
    uint16_t current_index = filename_index[denomination_index];    
    snprintf(wavname, 31, "denom%03d/clip%04d.wav", dollars, current_index);
    snprintf(mp3name, 31, "denom%03d/clip%04d.mp3", dollars, current_index);    
    
    Serial.print(F("Looking for file named: "));
    Serial.print(wavname);    
    Serial.print(F("..."));
    if(SD.exists(wavname)){
      found_file = true;
      strncpy(filename, wavname, 23);
      filename_index[denomination_index]++;
      Serial.println(F("OK"));
    }
    else{
      Serial.println(F("Fail."));      
    }
    
    if(!found_file){
      Serial.print(F("Looking for file named: "));
      Serial.print(wavname);    
      Serial.print(F("..."));      
      if(SD.exists(mp3name)){
        found_file = true;
        strncpy(filename, mp3name, 23);
        filename_index[denomination_index]++; 
        Serial.println(F("OK"));        
      }
      else{
        Serial.println(F("Fail."));
      }
    }
    
    if(!found_file){
      filename_index[denomination_index] = 0;
      memset(wavname, 0, 32); // clear the name
      memset(mp3name, 0, 32); // clear the name
      current_index = 0;     
      snprintf(wavname, 31, "denom%03d/clip%04d.wav", dollars, current_index); // try again
      snprintf(mp3name, 31, "denom%03d/clip%04d.mp3", dollars, current_index); // try again
      Serial.print(F("Looking for file named: "));
      Serial.print(wavname);    
      Serial.print(F("..."));      
      if(SD.exists(wavname)){
        found_file = true;
        strncpy(filename, wavname, 23);
        filename_index[denomination_index]++; 
        Serial.println(F("OK"));
      }
      else{
        Serial.println(F("Fail."));
      }
      
      if(!found_file){
        found_file = true;
        if(SD.exists(mp3name)){
          strncpy(filename, mp3name, 23);
          filename_index[denomination_index]++; 
          Serial.println(F("OK"));        
        }
        else{
          Serial.println(F("Fail."));
        }             
      }
    }
  }
}

int8_t dollarsToDenominationIndex(uint8_t dollars){
  uint8_t ret = -1;
  switch(dollars){
    case 1: ret = 0; break;
    case 5: ret = 1; break;
    case 10: ret = 2; break;
    case 20: ret = 3; break;
    case 50: ret = 4; break;
    case 100: ret = 5; break;
    default: ret = -1; break;
  }
  
  return ret;
}
