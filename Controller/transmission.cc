/* 
Handle communication between the AS01 ML01DP5 and the pico <- this is essentially a repackaged nrf24l01 so the protocols are the same.
The library used as a base is this: https://github.com/AndyRids/pico-nrf24
Even though the modules are not the same, communication works similarily so the library works.

More useful resources:	http://m.asnwireless.com/uploads/202022129/AS01-ML01DP6.pdf?rnd=536
						https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf
*/

#include "transmission.hpp"

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>


Transmission::Transmission() {
	nrf_handler = new nrf_client_t();

	nrf_driver_create_client(nrf_handler);
}

void Transmission::Initialize() {

	//	Use of designated initializers for ease of reading.
	pin_manager_t module_pins = {
		.copi = 7,
		.cipo = 8,
		.sck = 6,
		.csn = 9,
		.ce = 10
	};

	uint32_t baudrate = 1000000;

	nrf_handler->configure(&module_pins, baudrate);

	nrf_handler->initialise(NULL);											// NULL Specifies default config

	nrf_handler->standby_mode();


	nrf_handler->payload_size(DATA_PIPE_0, FIVE_BYTES);

	nrf_handler->dyn_payloads_disable();									// Disable dynamic payload, same size every time.

	// Create destination TX array here because C++ doesn't allow for temporary arrays unlike C
	uint8_t destination[] = {0xE7,0xD3,0xFC0,0x35,0x77};

	// Transmit an array of data to addresses on the module, address array is defined by the width/amount of data specified in payload_size
	nrf_handler->tx_destination(destination);			

}

void Transmission::TransmitData(uint8_t payload[5]) {
	nrf_handler->send_packet(payload, sizeof(payload));	
}

Transmission::~Transmission() {
	delete nrf_handler;
}
