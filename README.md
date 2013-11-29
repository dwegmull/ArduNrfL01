ArduNrfL01
==========

A pair of Arduino projects that use Nordic's Nrf24L01 RF link. One is the transmitter, the other the receiver. 
The system is optimized for very low power consumption on the transmit side. 
The system uses a four frequency hopping scheme. The receiver is constantly listening, changing frequency every 20mS.
The transmiter only sends when new data is present. It tries all four frequencies until the data goes through (with a timeout).
The data is the state of the digital inputs. It is reflected on the same pins, configured as outputs, on the receiver.
In Debug mode on the transmitter, a counter is sent as data once per second. Its value is sent out on the UART.
In Debug mode on the receiver prints out the data to the UART instead of ouptuting it to the GPIOs.

NOTE: The transmitter is designed to be powered directly from two alcaline batteries. In order to acheive this, a 3.3V 
      Arduino must be used. In addition, the voltage regulator on the board must be removed and bypassed.
      The voltage regulator on the RF board must also be removed and bypassed
      Finaly, the AVR processor configuration registers must be altered to allow operation down to 1.8V
