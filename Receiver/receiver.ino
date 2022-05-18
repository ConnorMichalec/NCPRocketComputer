#include "nRF24L01.h" //NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include "RF24.h"
#include "SPI.h"

#define CE 7
#define CSN 8
// No need to initialize spi pins, only one of them

#define BAUDRATE 1000000

#define PAYLOADSIZE 5

RF24 transciever(CE, CSN, BAUDRATE);

void setup(void){
	Serial.begin(9600);

	if (!transciever.begin()) {
		Serial.println(F("radio hardware is not responding!!"));
		while (1) {} // hold in infinite loop
  	}

	Serial.println("Transciever initialized");

	transciever.setPALevel(RF24_PA_MAX);

	transciever.setPayloadSize(PAYLOADSIZE);

	transciever.openReadingPipe(0, (uint8_t*) "pipeaddr");	// Open pipe 0

	transciever.startListening();
}

void loop(void){
	if (transciever.available()) {
		char receivedBuffer[5]; 
		transciever.read(&receivedBuffer, transciever.getPayloadSize());

		// Rally the data to the USB serial port for the computer to handle
		Serial.print(receivedBuffer);
	}
}