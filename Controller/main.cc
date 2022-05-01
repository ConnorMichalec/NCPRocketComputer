#include "main.hpp"
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/pwm.h>


// yes i know, vectors would have been easier but i was trying to learn okay?
// Also sorry for not using smart pointers


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

static int chars_rxed = 0;

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
    accumulated_received_buffer = new char[10000];	// Initialize received buffer pointer, allocating enough space
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

	transmission = new Transmission();

    stdio_init_all();

    uart_init(uart0, UART_BAUDRATE);

    gpio_set_function(pin_TXtoGPS, GPIO_FUNC_UART);
    gpio_set_function(pin_RXfromGPS, GPIO_FUNC_UART);


    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // turn off CTS/RTS(Flow control) because gps module works asynchronously
    uart_set_hw_flow(uart0, false, false);



    uart_set_format(uart0, UART_DATABITS, UART_STOPBITS, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0, false);    //dont use a TX fifo

    irq_set_exclusive_handler(UART0_IRQ, handleRXIRQ);
    irq_set_enabled(UART0_IRQ, true);

    uart_set_irq_enables(uart0, true, false);



	transmission->Initialize();

    // LOOP, RX interrupt from GPS data, resume loop
    while (true) {
		statusLEDTick();
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

    //  This identifies when this block of gps data ends. Transmit/Clear the accumulated buffer and begin accumulating the new data.
    const char* endblock_identifier = "\\$GPGLL.*N\\*.."; 


    // concatenate the recently received data to the end of our buffer

    if(accumulated_received_buffer_offset<1000) {
        memcpy(&accumulated_received_buffer[accumulated_received_buffer_offset], rxed, sizeof(uint8_t)*rxed_size);
        accumulated_received_buffer_offset += rxed_size;
    }
    else {
        // clear the buffer to prevent an overflow if it gets too large and no matches for a full block have been found
        memset(accumulated_received_buffer, 0, sizeof(char)*accumulated_received_buffer_offset);
        accumulated_received_buffer_offset = 0;
    }


    
    char temp_buffer_terminated[10000];     // scope level buffer just to hold the same thing but with a null termination character on the end of it.


    // add each char from the pointer sequence to the array

    for(int i = 0; i<accumulated_received_buffer_offset; i++) {
        temp_buffer_terminated[i] = accumulated_received_buffer[i];
    }

    // this is what allows strings to actually know when the string ends so we can do regex with it.
    temp_buffer_terminated[accumulated_received_buffer_offset] = '\0';

    //printf(temp_buffer_terminated);



    // find out of we now have a whole block of gps data, if so, process this gps data to transmit 

    regex expression(endblock_identifier);
    smatch match;
    string accumulated_received_buffer_string = string(temp_buffer_terminated);

    bool success = regex_search(accumulated_received_buffer_string, match, expression);


    // if we found a match this means we are now on the end of this data block, process what we have in the buffer, reset the buffer, and add the recently rxed data.
    if(success) {
        
        Process_gps_block(accumulated_received_buffer, accumulated_received_buffer_offset);

        // clear buffer after everything is nice and transmitted
        memset(accumulated_received_buffer, 0, sizeof(char)*accumulated_received_buffer_offset);
        accumulated_received_buffer_offset = 0;

    }

    
}


void Main::Process_gps_block(char *data, int data_size) {


    /* Split data into seperate lines */

    char **lineBuffer = new char*[300]; 
    char *charBuffer = new char[300];

    int line_offset = 0;
    int char_offset = 0;
    for(int dataIndex = 0; dataIndex<data_size; dataIndex++) {
        if(data[dataIndex] == '$') {
            //  newline
            lineBuffer[line_offset] = charBuffer;
            line_offset++;

            // reset line buffer
            memset(charBuffer, 0, sizeof(char)*char_offset);
            char_offset = 0;
            
        }
        else {
            // continue adding to current line
            charBuffer[char_offset] = data[dataIndex];
            char_offset++;

            
        }
    }

    /*          */

    printf(lineBuffer[0]);

    delete[] lineBuffer;
    delete[] charBuffer;
}

// transmit data over LoRa/RF module
void Main::Transmit(char *data, int data_size) {
}



