//#define DEBUG
#include "radio.h"
#include <avr/sleep.h>

#define PAYLOAD_SIZE 2
#define ACK_SIZE 0

byte switches[2];
byte cnt = 0;


ISR(PCINT0_vect)
{
#ifdef DEBUG
  Serial.println("PCINT0");
#endif
}

ISR(PCINT1_vect)
{
#ifdef DEBUG
  Serial.println("PCINT1");
#endif
}

ISR(PCINT2_vect)
{
#ifdef DEBUG
  Serial.println("PCINT2");
#endif
}

void setup()
{
  radioInit_T rInit;
  // Set processor speed to 4MHz to allow running down to 1.8V
  CLKPR = 0x80;
  CLKPR = 0x01;
#ifdef DEBUG  
  // Debug stuff
  Serial.begin(19200);
#endif
  switches[0] = switches[1] = 0;
  rInit.primRx = 0;
  rInit.payloadSize = PAYLOAD_SIZE;
  rInit.ackSize = ACK_SIZE;
  radioInit(rInit);
#ifdef DEBUG
  Serial.println("End of setup");
#else
  // Turn off power to peripherals we don't need
  PRR = 0x03;
  // Make sure both GPIOs used by the bootloader as UART are input
  pinMode(0, INPUT);
  pinMode(1, INPUT);
#endif
  // Make sure all pull ups are disabled on the inputs
  PORTD = 0x00;
  PORTB = 0x00;
}

void loop()
{
  // Read 10 input pins into two bytes.
  switches[0] = PIND;
  switches[1] = PINB & 0x03;   
  // Try to send the data 4 times before giving up.
  // Each try is done on four frequencies by the library.
  cnt = 4;
  while(cnt--)
  {
    if(radioSendPacket(switches))
    {
      break;
    }
  }

  // Turn off the radio
  radioSetPower(0);
  // Go to sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
  sleep_enable();
  cli();
  PCICR = 0x05;
  PCMSK0 = 0b00000011;
  PCMSK2 = 0b11111111;
    // Turn off power to peripherals we don't need
  PRR = 0xFF;
  sei();
  sleep_mode();
  // Program resumes here after wakeup.
  sleep_disable(); 
  PRR = 0x03;
 // Setup the SPI interface
  SPCR = 0x50; // Enable in Master mode. CPOL and CPHA are both low
  // Turn the radio back on (includes a 2mS delay)
  radioSetPower(1);
 }

