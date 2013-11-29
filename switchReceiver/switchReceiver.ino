#define DEBUG
#include "radio.h"

#define PAYLOAD_SIZE 2
#define ACK_SIZE 0

void setup()
{
  radioInit_T rInit;
#ifdef DEBUG  
  // Debug stuff
  Serial.begin(9600);
#endif
  
  rInit.primRx = 1;
  rInit.payloadSize = PAYLOAD_SIZE;
  rInit.ackSize = ACK_SIZE;
  radioInit(rInit);
}

void loop()
{
  unsigned char data[2] = {0, 0};
  radioHop(0);
  if(LOW == digitalRead(RADIO_PIN_IRQ))
  {
    // A packet arrived: read it
    radioReadPacket(data);
    radioAckPacket();
#ifdef DEBUG
      Serial.print("Data: ");
      Serial.println(data[0]);
#endif
    while(0 == radioRxFifoEmpty())
    {
      radioReadPacket(data);
#ifdef DEBUG
      Serial.print("Data: ");
      Serial.println(data[0]);
#endif
    }
    radioHop(1);
  }
}
