#pragma once

// This extern C is critical as it will tell the linker that nrf24_driver is c code, not c++
// Otherwise C++ will mangle the functions up and try to make them OOP
extern "C" {
	#include "nrf24_driver.h"
}

class Transmission {
	private:
		nrf_client_t *nrf_handler;

	public:
		Transmission();
		~Transmission();
		void TransmitData(uint8_t payload[5]);
		void Initialize();

};