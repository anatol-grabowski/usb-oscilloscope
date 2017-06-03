#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "requests.h"       /* The custom request numbers we use */

#include "calibrate_12800000.c"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */
#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             0

#define READBUFF_SIZE  1024
#define WRITEBUFF_SIZE READBUFF_SIZE
#define SS 2

static int currByte;
static int bytesLeft;
static int bytesReady;
unsigned char readBuffer[READBUFF_SIZE];
unsigned char* writeBuffer = readBuffer;
#include "sdcard.c"

unsigned char mode = 2;
unsigned char adc = 0;
unsigned char adc2ch[2];


uchar usbFunctionRead(uchar *data, uchar len) {
	uchar i;
	if(len > bytesLeft)
		len = bytesLeft;
			
	for (i=0; i<len; i++) {
		data[i] = readBuffer[currByte++];
	}
	
	bytesLeft -= len;
	return len;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
	uchar i;
	if(len > bytesLeft)
		len = bytesLeft;
		
	for (i=0; i<len; i++) {
		writeBuffer[currByte++] = data[i];
	}
	
	bytesLeft -= len;
	return bytesLeft == 0; /* return 1 if this was the last chunk */
}

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t    *rq = (void *)data;
	static uchar    dataBuffer[4];  /* buffer must stay valid when usbFunctionSetup returns */

	switch (rq->bRequest) {
		case (CUSTOM_RQ_ECHO): /* echo -- used for reliability tests */
			dataBuffer[0] = rq->wValue.bytes[0];
			dataBuffer[1] = rq->wValue.bytes[1];
			dataBuffer[2] = rq->wIndex.bytes[0];
			dataBuffer[3] = rq->wIndex.bytes[1];
			usbMsgPtr = (usbMsgPtr_t)dataBuffer;     /* tell the driver which data to return */
			return 4;
		case (CUSTOM_RQ_SET_STATUS): //toggle adc reference
			if(rq->wValue.bytes[0] & 1){    /* set LED */
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				ADMUX = (ADMUX & 0x3F) | 0xC0;
			}else{                          /* clear LED */
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				ADMUX = (ADMUX & 0x3F) | 0x40;
			}
			break;
		case (2):
			mode = 2;
			dataBuffer[0] = ((LED_PORT_OUTPUT & _BV(LED_BIT)) != 0);
			dataBuffer[0] = adc;
			usbMsgPtr = (usbMsgPtr_t)dataBuffer;
			return 1;
		case (6):
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return READBUFF_SIZE;
		case (7):
			PORTB &= ~(1<<SS);
			SPDR = rq->wValue.bytes[0];
			while (!(SPSR & (1<<SPIF)));
			PORTB |= (1<<SS);
			dataBuffer[0] = SPDR;
			usbMsgPtr = (usbMsgPtr_t)dataBuffer;
			return 1;
		case (8):
			bytesLeft = (rq->wLength.word > WRITEBUFF_SIZE) ? WRITEBUFF_SIZE : rq->wLength.word;
			currByte = 0;
			bytesReady = bytesLeft;
			mode = 8;
			return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
		case (9):
			bytesLeft = bytesReady;
			currByte = 0;
			return USB_NO_MSG;  /* use usbFunctionRead() to send data to host */
		case (10):
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return READBUFF_SIZE;
		case (11):
			if(rq->wValue.bytes[0] == 1){
				PORTB |= (1<<SS);
			}else{
				PORTB &= ~(1<<SS);
			}
			break;
		case (12):
			SD_command(0x40, 0x00000000, 0x95, 8);
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return 8;
		case (13):
			SD_command(0x41, 0x00000000, 0x95, 8);
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return 8;
		case (14):
			SD_command(0x50, 0x00000000, 0x95, 8);
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return 8;
		case (15):
			mode = 15;
			return 0;
		case (16):
			usbMsgPtr = (usbMsgPtr_t)readBuffer;
			return 128;
		case (17):
			//ADCSRA = (ADCSRA | 0xF8) | (rq->wValue.bytes[0] | 0x07);
			ADCSRA = rq->wValue.bytes[0];
			break;
		case (21):
			mode = rq->bRequest;
			usbMsgPtr = (usbMsgPtr_t)adc2ch;
			return 2;
		default:
			mode = rq->bRequest;
    }

    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int main(void) {
	int     i, j;
	uchar	bit;

	DDRB  |= (1<<6); //external pullup resistor for USB D- on B6
	PORTB |= (1<<6);

	CLKPR  = 0x80;
	CLKPR  = 0x00;
	OSCCAL = OSCCAL_VALUE;

	ADMUX  = 0x65;
	ADCSRA = 0x82;
	SPI_init();

	OCR0A = 0;
	OCR0B = 0;
	TCCR0A = 0x50;
	TCCR0B = 0x02;
	DDRD |= (1<<6);	//timer0 output on D6

	DDRB |= (1<<1);	//pulses for fast signal scoping on B1
	DDRB |= (1<<2);	//pulses for fast signal scoping on B2

    LED_PORT_DDR |= (1<<LED_BIT);	//LED on B0

    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
	_delay_ms(260);			/* fake USB disconnect for > 250 ms */
    usbDeviceConnect();
	//calibrateOscillator();
    sei();

    while (1) {                /* main event loop */
		switch (mode) {
			case (2):	//one value scoping (default mode)
				ADCSRA |= 1<<ADSC;
				while (1 & (ADCSRA>>ADSC));
				adc = ADCH;
				break;
			case (3):	//oscilloscope mode
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				for (i=0; i<READBUFF_SIZE; i++) { //scope adc
					ADCSRA |= 1<<ADSC;
					while (1 & (ADCSRA>>ADSC));
					readBuffer[i] = ADCH;
				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (4):	//logic capture on high
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				while ( !(PINC & (1<<5)) );  //wait high
				for (i=0; i<READBUFF_SIZE; i++) {
					readBuffer[i] = 0x00;
					for (bit=0; bit<8; bit++) {
						readBuffer[i] |= ((PINC & (1<<5))>>5)<<bit;
					}
				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (8):	//send SPI byte and get response
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				for (i=0; i<bytesReady; i++) {
					SPDR = writeBuffer[i];
					while (!(SPSR & (1<<SPIF)));
					readBuffer[i] = SPDR;
				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 0;
				break;
			case (15):	//send some SD command
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				SD_command(0x51, 0x00000000, 0x95, 32);
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (18):	//logic capture on rise
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				while ( (PINC & (1<<5)) );  //wait low
				while ( !(PINC & (1<<5)) );  //wait high
				for (i=0; i<READBUFF_SIZE; i++) {
					readBuffer[i] = 0x00;
					for (bit=0; bit<8; bit++) {
						readBuffer[i] |= ((PINC & (1<<5))>>5)<<bit;
					}
				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (19):	//scope weak fast repeating signal (256 valid samples)
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				for (i=0; i<258; i++) {
					PORTB &= ~(1<<1);
					PORTB |= (1<<2);
					_delay_ms(5);	//let the circuit stabilize
					GPIOR0 = i;
					PORTB |= (1<<1);
					PORTB &= ~(1<<2);

					//delay between samples is 3 CK = 0.25 us (at 12 MHz)
					asm("in		r24, 0x1e");	//0x1e - GPIOR0 location in memory
					asm("rjmp	.+2");
					asm("subi	r24, 0x01");	//1 CK
					asm("brne	.-4");			//2 CK
					/*while (GPIOR0) { //delay before scoping
						GPIOR0--;
						//asm("dec 0x1E");
						asm("nop");
					}*/

					ADCSRA |= 1<<ADSC;
					while (1 & (ADCSRA>>ADSC));
					readBuffer[i] = (unsigned char)((ADCL>>6)|(ADCH<<2)); //cuts top of adc
					//readBuffer[i] = ADCH;
				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (20):	//scope less fast repeating signal (more samples)
				LED_PORT_OUTPUT |= _BV(LED_BIT);
				for (i=0; i<READBUFF_SIZE; i++) {
					PORTB &= ~(1<<1);
					PORTB |= (1<<2);
					_delay_ms(5);	//let the circuit stabilize
					j = i;
					PORTB |= (1<<1);
					PORTB &= ~(1<<2);
					while (j) { //delay before scoping
						j--;
						asm("nop");
					}

					ADCSRA |= 1<<ADSC;
					while (1 & (ADCSRA>>ADSC));
					readBuffer[i] = ADCH;

				}
				LED_PORT_OUTPUT &= ~_BV(LED_BIT);
				mode = 2;
				break;
			case (21):	//scope on two channels
				ADCSRA |= 1<<ADSC;
				while (1 & (ADCSRA>>ADSC));
				adc = ADCH;
				break;
		}
		//LED_PORT_OUTPUT &= ~_BV(LED_BIT);
        usbPoll();
    }
}

/* ------------------------------------------------------------------------- */
