//#define DEBUG
#include "radio.h"

#define PAYLOAD_SIZE 2
#define ACK_SIZE 0

void setup()
{
  radioInit_T rInit;
#ifdef DEBUG  
  // Debug stuff
  Serial.begin(9600);
#else
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
#endif
  
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  // All solenoids are closed until we connect
  PORTD = 0x00;
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  
  rInit.primRx = 1;
  rInit.payloadSize = PAYLOAD_SIZE;
  rInit.ackSize = ACK_SIZE;
  radioInit(rInit);
}

void update_output(unsigned char *data)
{
#ifdef DEBUG
      Serial.print("Data: ");
      Serial.print(data[0]);
      Serial.print(" / ");
      Serial.println(data[1]);
#else
      PORTD = data[0] ^ 0x61;// 0, 5 and 6 are inverted to compensate
                             // for switch wiring.
      // SWAPPED to compensate for switch wiring
      // on the transmitter.
      if(data[1] & 0x01)
      {
        digitalWrite(8, LOW);
      }
      else
      {
        digitalWrite(8, HIGH);
      }
      if(data[1] & 0x02)
      {
        digitalWrite(9, HIGH);
      }
      else
      {
        digitalWrite(9, LOW);
      }
#endif
}

void loop()
{
  unsigned char data[2] = {0, 0};
  radioHop(0);
  if(LOW == digitalRead(RADIO_PIN_IRQ))
  {
    // A packet arrived: read it
    radioAckPacket();
    radioReadPacket(data);
    update_output(data);
    while(0 == radioRxFifoEmpty())
    {
#ifdef DEBUG
      Serial.print(" Second ");
#endif
      radioReadPacket(data);
      update_output(data);
    }
    radioHop(1);
  }
}
