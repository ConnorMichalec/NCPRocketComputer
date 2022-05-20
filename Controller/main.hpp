#pragma once

#include <stdio.h>
#include <map>
#include <string>
#include <regex>
#include <iostream>
#include <cstring>
#include "transmission.hpp"

class Main {
    private:
        char* accumulated_received_buffer;			// this holds the data we have reecieved from gps but have yet to sort out into chunks and proccessed to transmit.
        int accumulated_received_buffer_offset;   	// pointer offset for accumulated received data so it can act as array
		Transmission *transmission;
		void statusLEDTick();


    public:
        Main();
        ~Main();
        void Init();
        void RXGPS();
        void Handle_gps_data(const uint8_t *rxed, int rxed_size);
        void Transmit(const char *data, int data_size);
};