#pragma once

#include "RF24.h"

class Transmission {
	private:

	public:
		Transmission();
		~Transmission();
		bool TransmitData(uint8_t* payload);
		bool Initialize();

};