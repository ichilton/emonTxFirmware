/*
                          _____    
                         |_   _|   
  ___ _ __ ___   ___  _ __ | |_  __
 / _ \ '_ ` _ \ / _ \| '_ \| \ \/ /
|  __/ | | | | | (_) | | | | |>  < 
 \___|_| |_| |_|\___/|_| |_\_/_/\_\
 
//--------------------------------------------------------------------------------------
// CT based current + AC voltage measurement - real power, apparentpower
// This example can be used for up to 3 CT's 
// It is configured by default for one CT.
// Enable the others by uncommenting the lines that state uncomment for 2nd/3rd CT
// See http://openenergymonitor.org/emon/emontx/acac for calibration for different AC-AC adapters 

// Based on JeeLabs RF12 library http://jeelabs.org/2009/02/10/rfm12b-library-for-arduino/

// Contributors: Glyn Hudson, Trystan Lea, Michele 
// openenergymonitor.org
// GNU GPL V3
//--------------------------------------------------------------------------------------
*/

#define UNO       //anti crash wachdog reset only works with Uno (optiboot) bootloader, comment out the line if using delianuova


#include <avr/wdt.h>

#include "Emon.h"
EnergyMonitor emon1;
// EnergyMonitor emon2;  // Uncomment as if you need a second CT
// EnergyMonitor emon3;  // Uncomment as if you need a third CT

//JeeLabs libraries - https://github.com/jcw
#include <JeeLib.h>

#include <avr/eeprom.h>
#include <util/crc16.h>  

ISR(WDT_vect) { Sleepy::watchdogEvent(); } 	

//---------------------------------------------------------------------------------------------------
// Serial print settings - disable all serial prints if SERIAL 0 - increases long term stability 
//---------------------------------------------------------------------------------------------------
#define SERIAL 1
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// RF12 settings 
//---------------------------------------------------------------------------------------------------
// fixed RF12 settings

#define myNodeID     10         //in the range 1-30
#define network     210      //default network group (can be in the range 1-250). All nodes required to communicate together must be on the same network group
#define freq RF12_433MHZ     //Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

#define COLLECT 0x20 // collect mode, i.e. pass incoming without sending acks
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// LED Indicator  
//--------------------------------------------------------------------------------------------------
# define LEDpin 9          //hardwired on emonTx PCB
//--------------------------------------------------------------------------------------------------

//########################################################################################################################
//Data Structure to be sent
//######################################################################################################################## 
typedef struct 
{ 
  int powerA, appA;
  //int powerB, appB; // Uncomment as if you need a second CT
  //int powerC, appC; // Uncomment as if you need a third CT
  
} PayloadTX;
PayloadTX emontx;   
//########################################################################################################################

//********************************************************************
//SETUP
//********************************************************************
void setup()
{  
  
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, HIGH);    //turn on LED 
  
  Serial.begin(9600);
  Serial.println("emonTx single CT AC voltage real power example");
  Serial.println("openenergymonitor.org");
  
  rf12_initialize(myNodeID,freq,network); 
  rf12_sleep(RF12_SLEEP); 
  delay(100);
  
  Serial.print("Node: "); 
  Serial.print(myNodeID); 
  Serial.print(" Freq: "); 
   if (freq == RF12_433MHZ) Serial.print("433Mhz");
   if (freq == RF12_868MHZ) Serial.print("868Mhz");
   if (freq == RF12_915MHZ) Serial.print("915Mhz");  
  Serial.print(" Network: "); 
  Serial.println(network);
  
  if (!SERIAL) {
    Serial.println("serial disabled"); 
    Serial.end();
  }
  
  digitalWrite(LEDpin, LOW);              //turn off LED
  
  #ifdef UNO
  wdt_enable(WDTO_8S); 
  #endif;
}

//********************************************************************
//LOOP
//********************************************************************
void loop()
{
  #ifdef UNO
  wdt_reset();
  #endif
  
  int vcc = readVcc();  //read emontx supply voltage
  
  //------------------------------------------------
  // MEASURE FROM CT'S
  // Calibration notes:
  // 212.6 is a suitable calibration value for the Euro AC-AC adapter http://uk.rs-online.com/web/p/products/459-853/
  // 237.3 is a suitable calibration value for the UK AC-AC adapter http://uk.rs-online.com/web/p/products/400-6484/
  //------------------------------------------------
  emon1.setPins(2,0);                     //emonTX AC-AC voltage (ADC2), current pin (CT1 - ADC3) 
  emon1.calibration(238.5, 138.8,1.7);    //voltage calibration , current calibration, power factor calibration. See: http://openenergymonitor.org/emon/emontx/acac
  emon1.calc(20,2000,vcc);               //No.of wavelengths, time-out , emonTx supply voltage 
  emontx.powerA = emon1.realPower;
  emontx.appA = emon1.apparentPower;
  emon1.serialprint();
 
  /* UNCOMMENT AS REQURED FOR 2ND CT
  
  emon2.setPins(2,0);                     //emonTX AC-AC voltage (ADC2), current pin (CT2 - ADC0) 
  emon2.calibration(280.0,126.5,1.7);    //voltage calibration , current calibration, power factor calibration. See: http://openenergymonitor.org/emon/emontx/acac
  emon2.calc(20,2000,vcc);               //No.of wavelengths, time-out , emonTx supply voltage 
  emontx.powerB = emon2.realPower;
  emontx.appB = emon2.apparentPower;
  //emon2.serialprint(); 
  */
  
  /* UNCOMMENT AS REQURED FOR 3RD CT
  
  emon3.setPins(2,1);                     //emonTX AC-AC voltage (ADC2), current pin (CT3 - ADC1) 
  emon3.calibration(280.0,126.5,1.7);    //voltage calibration , current calibration, power factor calibration. See: http://openenergymonitor.org/emon/emontx/acac
  emon3.calc(20,2000,vcc);               //No.of wavelengths, time-out , emonTx supply voltage 
  emontx.powerC = emon3.realPower;
  emontx.appC = emon3.apparentPower;
  //emon3.serialprint();
  */
  
  //Serial.println();
  //delay(100);
  
  #ifdef UNO
  wdt_reset();
  #endif
  //--------------------------------------------------------------------------------------------------
  // 2. Send data via RF, see:  http://jeelabs.net/projects/cafe/wiki/RF12 for library documentation
  //--------------------------------------------------------------------------------------------------
  rf12_sleep(RF12_WAKEUP);                                            // wake up RF module
  int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}  // if ready to send + exit loop if    
  rf12_sendStart(0, &emontx, sizeof emontx);                          // send emontx data
  rf12_sendWait(2);                                                   // wait for RF to finish sending wh
  rf12_sleep(RF12_SLEEP);                                             // put RF module to sleep
  //--------------------------------------------------------------------------------------------------   
 
  digitalWrite(LEDpin, HIGH);    //flash LED - very quickly 
  delay(2); 
  digitalWrite(LEDpin, LOW); 
  
//sleep controll code  
//Uses JeeLabs power save function: enter low power mode and update Arduino millis 
//only be used with time ranges of 16..65000 milliseconds, and is not as accurate as when running normally.http://jeelabs.org/2010/10/18/tracking-time-in-your-sleep/

  if ( (vcc) > 3250 ) {//if emonTx is powered by 5V usb power supply (going through 3.3V voltage reg) then don't go to sleep
    for (int i=0; i<5; i++)
    { 
      delay(1000); // 1 second delay x 5
      
      #ifdef UNO
      wdt_reset();  //8s anti-crash watchdog reset, if hang/delay for more than 8s then reset 
      #endif
      
    } //delay 10s 
  } else {
    //if battery voltage drops below 2.7V then enter battery conservation mode (sleep for 60s in between readings) (need to fine tune this value) 
    if ( (vcc) < 2700) Sleepy::loseSomeTime(60000); else Sleepy::loseSomeTime(5000);
  }

}
//********************************************************************

//--------------------------------------------------------------------------------------------------
// Read current emonTx battery voltage - not main supplyV!
//--------------------------------------------------------------------------------------------------
long readVcc() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;
  return result;
}

