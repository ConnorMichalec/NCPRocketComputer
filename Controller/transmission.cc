/* 
Handle communication between the AS01 ML01DP5 and the pico <- this is essentially a repackaged nrf24l01 so the protocols are the same.
The library used as a base is this: https://github.com/nRF24/RF24

More useful resources:	http://m.asnwireless.com/uploads/202022129/AS01-ML01DP6.pdf?rnd=536
						https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf
						https://nrf24.github.io/RF24/md_docs_pico_sdk.html
*/

#include "transmission.hpp"

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <iostream>
#include <cstring>

#define CE 7
#define CSN 8
#define SCK 2
#define COPI 3			// TX
#define CIPO 4			// RX

#define BAUDRATE 1000000 // SPI speed

#define PAYLOADSIZE 5

//RF24 module(CE, CSN, BAUDRATE);
RF24 transmitter(CE, CSN, BAUDRATE);

Transmission::Transmission() {
	spi.begin(spi0, SCK, COPI, CIPO);
	
}

bool Transmission::Initialize() {
	if (!transmitter.begin(&spi)) {
		// Initialization failure
		return(1);	
    }

	transmitter.setPALevel(RF24_PA_MAX);

	// Set to higher channel as more devices use lower channels 
	transmitter.setChannel(100);

	// Set to lowest data rate to avoid interference
	transmitter.setDataRate(RF24_250KBPS);

	transmitter.setPayloadSize(PAYLOADSIZE);	// 5 bytes


	// Pipes are used in a network of transcievers, not very useful for this case
	transmitter.openWritingPipe((uint8_t*) "pipeaddr");	


	transmitter.stopListening();	// TX mode

	return(0);
}

bool Transmission::TransmitData(uint8_t* payload) {
	return(transmitter.write(payload, 8*PAYLOADSIZE));
}

Transmission::~Transmission() {

}
