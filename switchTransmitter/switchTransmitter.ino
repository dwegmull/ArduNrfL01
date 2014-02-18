//#define DEBUG
// Using A4 (pin 18) in replacement for pin 7 which appears
// to have its pull up always enabled, on my board.
//#define USE_A4

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
#ifdef USE_A4
  PORTD = 0x80;
  PORTB = 0x00;
  pinMode(18, INPUT);
  digitalWrite(18, 0);
#else
  PORTD = 0x00;
  PORTB = 0x00;
#endif
}

void loop()
{
  // Read 10 input pins into two bytes.
#ifdef USE_A4
  switches[0] = PIND & 0x7F;
  if(PINC & 0x10)
  {
    switches[0] |= 0x80;
  }
#else
  switches[0] = PIND;
#endif
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
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is selected here
  sleep_enable();
  cli();
#ifdef USE_A4
  PCICR = 0x07;
  PCMSK0 = 0b00000011;
  PCMSK1 = 0b00010000;
  PCMSK2 = 0b01111111;
#else
  PCICR = 0x05;
  PCMSK0 = 0b00000011;
  PCMSK2 = 0b11111111;
#endif
    // Turn off power to peripherals we don't need (all of them)
  PRR = 0xEF;
  sei();
  sleep_mode();
  // Program resumes here after wakeup.
  sleep_disable(); 
  PRR = 0x03;
  // Set all switch pins to input no pull up.
  DDRD = 0x00;
  PORTD = 0x00;
  DDRB &= ~0x03;
  PORTB &= ~0x03;
 // Setup the SPI interface
  SPCR = 0x50; // Enable in Master mode. CPOL and CPHA are both low
  // Turn the radio back on (includes a 2mS delay)
  radioSetPower(1);
 }

