arduino-garage-door-monitor
===========================

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