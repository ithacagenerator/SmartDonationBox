int apex_pin = 3;                      // what pin is the apex bill reader connect to?
int apex_interrupt = 1;                // interrupt to use when sensing a pulse
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

    if (pulses == 1) {            // $1
      dollar = 1;
    } else {

      if (pulses == 5) {         // $5
        dollar = 5;
      } else {

        if (pulses == 10) {      // $10
          dollar = 10;
        } else {

          if (pulses == 20) {   // $20
            dollar = 20;
          }
        }
      }
    }
    
    if( pulses != 0 ){
      Serial.print("dollar: ");
      Serial.print(dollar);
      Serial.print(" pulses: ");
      Serial.print(pulses);
      Serial.print(" last pulse: ");
      Serial.println(now - last_change);
    }
    
    pulses = 0;
    dollar = 0;
    checked = true;

  }
}
