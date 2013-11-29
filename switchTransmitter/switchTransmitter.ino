#define DEBUG
#include "radio.h"

#define PAYLOAD_SIZE 2
#define ACK_SIZE 0

byte switches[2];
byte cnt = 0;

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
#ifdef DEBUG
  Serial.println("Before setup");
#endif
  radioInit(rInit);
#ifdef DEBUG
  Serial.println("End of setup");
#endif
}

void loop()
{
  
  Serial.println(cnt);
#ifdef DEBUG
  delay(500); // Wait one second
  switches[0] = cnt++;
#else
  
#endif // DEBUG
  radioSendPacket(switches);
}
