#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

/*
 Slava Asipenko | slava.asipenko@gmail.com | 2014

 Low power night garage door monitor with self adjusting light level threshold. Sounds periodic alarm if garage door is left open at night.
 
 
 How it works. MCU is configured to sleep in a very low power mode (0.3 mA). Periodically it wakes up (ev. 5 min by default), powers sensors and
 measures light level + whether an object is detected by the infrared sensor (i.e. if the door is closed).
  
 If the light level is close to the minimum light level observed before (i.e. it's night), and the door is open (no object is detected), 
 emits brief alarm sound.
  
 If the light level is higher than the threshold, no alarm will sound regardless of the door status.
 
 Two LED lights for troubleshooting. Needs to run on 4.5-5V for the infrared sensor to work reliably. 
 
 ATMEGA's bootloader must run the chip at 8MHz instead of the default 20 for true low power sleep to work.
 I used avrdude to upload stock ATmegaBOOT_168_atmega328_pro_8MHz.hex (comes with Arduino) with the following switches:
   -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xD8:m -Ulfuse:w:0xe2:m
  
 Ran on ATMEGA328P (make sure it's a P for low power). 3 AA batteries should be able to power the circuit in sleep mode for ~2-3 years. 
 This does not take into account power consumed during active probing sessions, so this is an optimistic estimate.
 
 Thanks to Nate's for the writeup at https://www.sparkfun.com/tutorials/309 with details on running 328P in low power mode.

 Circuit:
  speaker on digital pin 8 (ideally via some sort of amplifier, I just used two TP C945 transistors)
  green OK DOOR CLOSED LED on 7
  red ALARM DOOR OPEN LED on 6
  photoresistor on analog 5 to 5V
  4.7K resistor on analog 5 to ground
  infrared sensor E18-D80NK: green to ground, red to D1 pin #3 (NOT 5V), yellow: pullup 1K to 5V, digital out to D0 (pin 2)

*/

const int statusLedPin = 7;
const int tonePin = 8;
const int photoResPin = A5;
const int infraredSensorPin = 0;
const int doorOpenLedPin = 6;
const int infraredPowerPin = 1;

const int probeEveryWakeUpCycle = 40; // 1 cycle ~8 sec
int wakeUpCounter;

int minLightLevel = 450;
int maxLightLevel = 550;
double warningLightLevel = minLightLevel;

int origADCSRA;
volatile int wdtFlag = 0;

ISR(WDT_vect)
{
  wdtFlag = 1;
}

void enterSleep(int turnOffAdc, int turnOffBod)
{
  for (byte i = 0; i <= A5; i++)
  {
    if( i != photoResPin ) 
    {
      pinMode (i, OUTPUT);    
      digitalWrite (i, LOW);  
    }
  }

  if( turnOffAdc )
  {
    origADCSRA = ADCSRA;
//    ADCSRA = 0;
  }


  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  // SLEEP_MODE_PWR_SAVE  // SLEEP_MODE_PWR_DOWN  // SLEEP_MODE_STANDBY
  sleep_enable();
 
  if( turnOffBod )
  {
    MCUCR = _BV (BODS) | _BV (BODSE);
    MCUCR = _BV (BODS); 
  }
  
  sleep_cpu();   

  if( wdtFlag )
  {
    wdtFlag = 0;
    if( ++wakeUpCounter > probeEveryWakeUpCycle )
    {
      wakeUpCounter = 0;
      probe();
    }
  }  
}

void probe()
{
  boolean objectDetected = powerOnAndReadFromInfraredSensor();
  if (!objectDetected) { 
    turnLedOn(doorOpenLedPin);
  } else {
    turnLedOn(statusLedPin);
  }

  int l = processLightLevel();
  if (l <= warningLightLevel && !objectDetected) {
    soundAlarm(warningLightLevel - l);
  }

  delay(50);

  turnLedOff(statusLedPin);
  turnLedOff(doorOpenLedPin);
}

void turnLedOn(int pin)
{
  pinMode(pin, OUTPUT); 
  digitalWrite(pin, HIGH);
}

void turnLedOff(int pin)
{
  digitalWrite(pin, LOW);
}

// returns TRUE if proximity detection went off, i.e. an obstacle was detected within the range
boolean powerOnAndReadFromInfraredSensor()
{
  pinMode(infraredPowerPin, OUTPUT);
  digitalWrite(infraredPowerPin, HIGH);
  pinMode(infraredSensorPin, INPUT); 
  delay(25);
  
  return digitalRead(infraredSensorPin) == LOW;
}

void powerOffInfraredSensor()
{
  digitalWrite(infraredPowerPin, LOW);
  pinMode(infraredSensorPin, OUTPUT); // for lower power consumption in standby mode? not 100% sure
}

// returns current light level
int processLightLevel()
{
  int l = readPhotoRes();
  
  if( minLightLevel > l )
  {
    minLightLevel = l;
  }
  
  if( maxLightLevel < l )
  {
    maxLightLevel = l + 1;
  }
  
  warningLightLevel = minLightLevel + (maxLightLevel - minLightLevel) / 10.0;
  
  return l;
}

void setupWdt()
{
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
}

int readPhotoRes()
{
//  ADCSRA = origADCSRA;
//  delay(100);
  return analogRead(photoResPin);
}

// maps light level input 0-1000 to duration of the alarm
void soundAlarm(int input)
{
    int duration = map(input, 0, 1000, 0, 10000);
    if( duration > 10 )
    {
      tone(tonePin, 1000, duration);
      delay(duration);
    }
}

void setup() {
  setupWdt();  
}

void loop() {
  enterSleep(true, true);
}

