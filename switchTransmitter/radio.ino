#include "radio.h"

unsigned long hopTime;
unsigned char payloadSize;
unsigned char ackSize;
unsigned char currentChannelNumber;
boolean primRx;
const unsigned char hopChannels[] = {HOP_CHANNEL0, HOP_CHANNEL1, HOP_CHANNEL2, HOP_CHANNEL3};

#define RX_HOP_TIME 20

void radioInit(radioInit_T init)
{
  // Set CSN high, to deselect the SPI interface of the radio
  digitalWrite(RADIO_PIN_CSN, 1);
  pinMode(RADIO_PIN_CSN, OUTPUT);
  // Make sure the radio is disabled by setting CE to low
  digitalWrite(RADIO_PIN_CE, 0);
  pinMode(RADIO_PIN_CE, OUTPUT);
  //  Set SPI pins to be outputs
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
  // IRQ is setup as an input with pull up
  pinMode(RADIO_PIN_IRQ, INPUT);  
  digitalWrite(RADIO_PIN_IRQ, 1);
  // Setup the SPI interface
  SPCR = 0x50; // Enable in Master mode. CPOL and CPHA are both low
  // Configure the chip and the local data
  payloadSize = init.payloadSize;
  ackSize = init.ackSize;
  currentChannelNumber = 0;
  primRx = init.primRx;
  radioSetChannel(hopChannels[currentChannelNumber]);
  if(primRx)
  {
    // Setup as a receiver
    unsigned char buffer[5];
    // Enable Enhanced ShockBurst mode
    buffer[0] = ENAA_P0;
    RadioAccessRegister(W_REGISTER | EN_AA, buffer, 1);
    // Enable Rx pipe 0
    buffer[0] = ERX_P0;
    RadioAccessRegister(W_REGISTER | EN_RXADDR, buffer, 1);
    // Set the Rx address
    buffer[0] = 0xDE;
    buffer[1] = 0xAD;
    buffer[2] = 0xBE;
    buffer[3] = 0xEF;
    buffer[4] = 0x42;
    RadioAccessRegister(W_REGISTER | RX_ADDR_P0, buffer, 5);
    // Set the receive buffer size
    buffer[0] = payloadSize;
    RadioAccessRegister(W_REGISTER | RX_PW_P0, buffer, 1);
    
    // Config register
    buffer[0] = MASK_TX_DS | MASK_MAX_RT | EN_CRC | CRCO | PRIM_RX | PWR_UP;
    RadioAccessRegister(W_REGISTER | CONFIG, buffer, 1);
    hopTime = millis() + RX_HOP_TIME;
    // Start listening
    digitalWrite(RADIO_PIN_CE, 1);
  }
  else
  {
    // Setup as a transmitter
    unsigned char buffer[5];
    // Enable Enhanced ShockBurst mode
    buffer[0] = ENAA_P0;
    RadioAccessRegister(W_REGISTER | EN_AA, buffer, 1);
    // Enable Rx pipe 0
    buffer[0] = ERX_P0;
    RadioAccessRegister(W_REGISTER | EN_RXADDR, buffer, 1);
    // Set the Rx and Tx addresses
    buffer[0] = 0xDE;
    buffer[1] = 0xAD;
    buffer[2] = 0xBE;
    buffer[3] = 0xEF;
    buffer[4] = 0x42;
    RadioAccessRegister(W_REGISTER | RX_ADDR_P0, buffer, 5);
    buffer[0] = 0xDE;
    buffer[1] = 0xAD;
    buffer[2] = 0xBE;
    buffer[3] = 0xEF;
    buffer[4] = 0x42;
    RadioAccessRegister(W_REGISTER | TX_ADDR, buffer, 5);
    // Config register
    buffer[0] = MASK_RX_DR | EN_CRC | CRCO | PWR_UP;
    RadioAccessRegister(W_REGISTER | CONFIG, buffer, 1);
  }
}


unsigned char exchangeByte(unsigned char data)
{
  // Start the transfer
  SPDR = data;
  // Wait for the transfer to be complete
  while(0 == (SPSR & 0x80));
  return SPDR;
}

unsigned char RadioAccessRegister(unsigned char commandWord, 
                                  unsigned char *data,
                                  unsigned char argSize)
{
  unsigned char radioStatus = 0;
  digitalWrite(RADIO_PIN_CSN, 0);
  radioStatus = exchangeByte(commandWord);
  while(argSize--)
  {
    if(W_TX_PAYLOAD == commandWord)
    {
      exchangeByte(*data); // Don't overwrite the payload!
    }
    else
    {
      *data = exchangeByte(*data);
    }
    data++;
  }
  digitalWrite(RADIO_PIN_CSN, 1);
  return(radioStatus);
}

void radioSetPower(unsigned char onOff)
{
  if(0 != onOff)
  {
    if(primRx)
    {
      onOff = MASK_TX_DS | MASK_MAX_RT | EN_CRC | CRCO | PRIM_RX | PWR_UP;
    }
    else
    {
      onOff = MASK_RX_DR | EN_CRC | CRCO | PWR_UP;
    }
    RadioAccessRegister(W_REGISTER | CONFIG, &onOff, 1);
    // Now wait 2mS for the crystal to stabilize
    delay(2);
  }
  else
  {
    RadioAccessRegister(W_REGISTER | CONFIG, &onOff, 1);
  }
}

void radioSetChannel(unsigned char channel)
{
  RadioAccessRegister(W_REGISTER | RF_CH, &channel, 1);
}

void radioHop(boolean hopNow)
{
  // Go to the next channel, staying within the four defined channels.
  // behavior is different based on radio role (Rx or Tx)
  if(primRx)
  {
    if(false == hopNow)
    {
      // Only hop if enogh time has ellapsed since the last hop
      if(millis() > hopTime)
      {
        hopTime = millis() + RX_HOP_TIME;
      }
      else
      {
        return;
      }
    }
    else
    {
        hopTime = millis() + RX_HOP_TIME;
    }
    // Stop listening
    digitalWrite(RADIO_PIN_CE, 0);
  }
  currentChannelNumber = (currentChannelNumber + 1) & 0x03;
  radioSetChannel(hopChannels[currentChannelNumber]);
  if(primRx)
  {
    // Start listening on the new channel
    digitalWrite(RADIO_PIN_CE, 1);
  }
}

void radioAckPacket(void)
{
  byte buffer = RX_DR;
  RadioAccessRegister(W_REGISTER | STATUS, &buffer, 1);
}

byte radioRxFifoEmpty(void)
{
  byte rxStatus;
  RadioAccessRegister(R_REGISTER | FIFO_STATUS, &rxStatus, 1);
  return(rxStatus & RX_EMPTY);
}

byte radioSendPacket(unsigned char *data)
{
  byte radioStatus;
  byte TxCnt;
  // Clear any pending interrupt
  radioStatus = TX_DS | MAX_RT | RX_DR;
  RadioAccessRegister(W_REGISTER | STATUS, &radioStatus, 1);
  // Flush the FIFO
  RadioAccessRegister(FLUSH_TX, &radioStatus, 0);
  // Load the packet into the radio FIFO
  RadioAccessRegister(W_TX_PAYLOAD, data, payloadSize);
  // Power up the radio and send the packet
  radioStatus = MASK_RX_DR | EN_CRC | CRCO | PWR_UP;
  RadioAccessRegister(W_REGISTER | CONFIG, &radioStatus, 1);
  delay(3); // Wait for the crystal to start up and the pll to stabilize.
  for(TxCnt = 0; TxCnt < 4; TxCnt++)
  {
    digitalWrite(RADIO_PIN_CE, 1);
    delayMicroseconds(5);
    digitalWrite(RADIO_PIN_CE, 0);
    // Wait to find out the outcome
    while(HIGH == digitalRead(RADIO_PIN_IRQ));
    // Get the status from the radio
    radioStatus = RadioAccessRegister(NOP, &radioStatus, 0);
    // No matter the outcome of this Tx,
    // the next one will be on another channel
    radioHop(1);
    if(radioStatus & TX_DS)
    {
      // All done!
      return(1);
    }
    delayMicroseconds(130); // Wait for the PLL to settle on the new channel
    // Clear any pending interrupt
    radioStatus = TX_DS | MAX_RT | RX_DR;
    RadioAccessRegister(W_REGISTER | STATUS, &radioStatus, 1);
  }
  return(0);
}

void radioReadPacket(unsigned char *data)
{
  RadioAccessRegister(R_RX_PAYLOAD, data, payloadSize);
}
