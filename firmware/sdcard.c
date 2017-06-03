#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#define CS (1<<PB2)
#define MOSI (1<<PB3)
#define MISO (1<<PB4)
#define SCK (1<<PB5)
#define CS_DDR DDRB
#define CS_ENABLE() (PORTB &= ~CS)
#define CS_DISABLE() (PORTB |= CS)

void SPI_init() {
    CS_DDR |= CS; // SD card circuit select as output
    DDRB |= MOSI + SCK; // MOSI and SCK as outputs
    PORTB |= MISO; // pullup in MISO, might not be needed
        
    // Enable SPI, master, set clock rate fck/128
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPR1);
}

unsigned char SPI_write(unsigned char ch) {
    SPDR = ch;
    while(!(SPSR & (1<<SPIF))) {}       
    return SPDR;
}

void SD_command(unsigned char cmd, unsigned long arg, 
                unsigned char crc, int read) {
    unsigned char i, buffer[8];
        
    CS_ENABLE();
    SPI_write(cmd);
    SPI_write(arg>>24);
    SPI_write(arg>>16);
    SPI_write(arg>>8);
    SPI_write(arg);
    SPI_write(crc);
                
    for(i=0; i<read; i++)
        readBuffer[i] = SPI_write(0xFF);
                
    CS_DISABLE();               
                
    // print out read bytes
}
