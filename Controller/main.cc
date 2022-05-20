#include "main.hpp"
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/pwm.h>


// yes i know, vectors would have been easier but i was trying to learn okay?
// Also sorry for not using smart pointers
// Also c++ was a terrible choice and made this so much harder than python


// UART 0
const int pin_TXtoGPS = 0;
const int pin_RXfromGPS = 1;

// Will play tone out of this output to make finding the rocket on the ground easier.
const int pin_beaconSpeaker = 15;
const int beaconSpeakerWaitTime = 100;		// How many seconds to wait till beacon speaker should activate.
const int beaconSpeakerHz = 400;			// Tone to produce 

const int UART_BAUDRATE = 9600;
const int UART_DATABITS = 8;  				// bit width for uart baud 
const int UART_STOPBITS = 1;  				// amount of bits to signal stop of baud


using namespace std;

Main *program;

// GPS RX interrupt handler
void handleRXIRQ() {
    program->RXGPS();
}

// Core 0 main code  For good ref on multicore setup ref: //Ref: https://learnembeddedsystems.co.uk/basic-multicore-pico-example
int main() {
    // Note: NOT using multicore setup...
	
    program = new Main();

    program->Init();
}

Main::Main() {
    accumulated_received_buffer = new char[500];	// Initialize received buffer pointer, allocating enough space
    accumulated_received_buffer_offset = 0;			// start out with empty buffer so pointer offset is 0
}

Main::~Main() {
    delete[] accumulated_received_buffer;
}


/**
 * Simply helps debug if the picos working
 */
void Main::statusLEDTick() {
	gpio_put(PICO_DEFAULT_LED_PIN, true);
	sleep_ms(10);
	gpio_put(PICO_DEFAULT_LED_PIN, false);
	sleep_ms(100);
}


void Main::Init() {

    stdio_init_all();

    uart_init(uart0, UART_BAUDRATE);

    gpio_set_function(pin_TXtoGPS, GPIO_FUNC_UART);
    gpio_set_function(pin_RXfromGPS, GPIO_FUNC_UART);


    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

	transmission = new Transmission();

	if(transmission->Initialize()) {
		// fail
		while(true){
			printf("transmitter fail to init");
			sleep_ms(100);
		}
	}

    // turn off CTS/RTS(Flow control) because gps module works asynchronously
    uart_set_hw_flow(uart0, false, false);



    uart_set_format(uart0, UART_DATABITS, UART_STOPBITS, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0, false);    //dont use a TX fifo

    irq_set_exclusive_handler(UART0_IRQ, handleRXIRQ);
    irq_set_enabled(UART0_IRQ, true);

    uart_set_irq_enables(uart0, true, false);


    // LOOP, RX interrupt from GPS data, resume loop
    while (true) {
		tight_loop_contents();
    }

}


void Main::RXGPS() {


    int amount_rxed = 0;    // our pointer offset, so we know how big our "array" is at any time.

    // char pointer array of type uint8 so we can ensure we are storing the same way received from rx.
    // note for the gt-u7 it only submits one symbol per function call due to it not utilizing the FIFO, so this buffer will probably not exceed a size of one pointer offset.
    uint8_t* rxed_buffer = new uint8_t[100];


    // keep reading while stuff is in FIFO

    while(uart_is_readable(uart0)) {
        char char_received = uart_getc(uart0);
        rxed_buffer[amount_rxed] = char_received;
        amount_rxed++;
    }


    // cut off the extra space at end of buffer

    uint8_t* rxed = new uint8_t[amount_rxed];   
    memcpy(rxed, rxed_buffer, sizeof(uint8_t)*amount_rxed);


    Handle_gps_data(rxed_buffer, amount_rxed);

    delete[] rxed_buffer;
}
	
// format received gps data and seperate it into chunks of positions.
void Main::Handle_gps_data(const uint8_t *rxed, int rxed_size) {

    // This matches the nmea sentence we should send, GPGGA contains all the info we need. 
    const char* line_identifier = "\\$GPGGA.*\\*.."; 


    // concatenate the recently received data to the end of our buffer
    if(accumulated_received_buffer_offset<500) {
        memcpy(&accumulated_received_buffer[accumulated_received_buffer_offset], rxed, sizeof(uint8_t)*rxed_size);
        accumulated_received_buffer_offset += rxed_size;
    }
    else {
        // clear the buffer to prevent an overflow if it gets too large and no matches for a full block have been found
        memset(accumulated_received_buffer, 0, sizeof(char)*accumulated_received_buffer_offset);
        accumulated_received_buffer_offset = 0;
    }


    // scope level buffer just to hold the same thing but with a null termination character on the end of it.
    char temp_buffer_terminated[500];     


    // add each char from the dynamic array to the array
    for(int i = 0; i<accumulated_received_buffer_offset; i++) {
        temp_buffer_terminated[i] = accumulated_received_buffer[i];
    }


    // this is what allows strings to actually know when the string ends so we can do regex with it.
    temp_buffer_terminated[accumulated_received_buffer_offset] = '\0';




    // find out of we now have a whole block of gps data, if so, process this gps data to transmit 
    regex expression(line_identifier);
    smatch match;
    string accumulated_received_buffer_string = string(temp_buffer_terminated);

	bool success = 0;

	// Only check if we are at the end of line. to save mcu cpu power
	if(rxed[0] == '\n') {
		success = regex_search(accumulated_received_buffer_string, match, expression);
		
	}
    // if we found a match, transmit what we want, reset the buffer, and add the recently rxed data.
    if(success) {
		printf("%s",temp_buffer_terminated);

		// Appending a newline because the match is only to right before the newline.
		string toTransmit = match[0].str().append("\n");

        Transmit(toTransmit.c_str(), match.length(0));


        // clear buffer after everything is nice and transmitted
		delete[] accumulated_received_buffer;
		accumulated_received_buffer = new char[500];
		accumulated_received_buffer_offset = 0;
        

    }


    
}



// transmit data over LoRa/RF module
void Main::Transmit(const char *data, int data_size) {

	// Transmit data in 5 byte chunks
	
	uint8_t* transmissionBuffer = new uint8_t[5];
	
	for(int bufferOffset = 0; bufferOffset<data_size; bufferOffset++) {
		transmissionBuffer[bufferOffset%5] = data[bufferOffset];

		// tarnsmit buffer if it gets full or we reach end of data segment
		if(((bufferOffset % 5) == 4) || bufferOffset == data_size-1) {
			gpio_put(PICO_DEFAULT_LED_PIN, true);
			//printf("%s", transmissionBuffer);
			if(!transmission->TransmitData(transmissionBuffer)) {
				printf("No ack response");
			}
			gpio_put(PICO_DEFAULT_LED_PIN, false);

			memset(transmissionBuffer, 0, 5*sizeof(char));

		}
	}

	delete[] transmissionBuffer;

}



