//search for best OSCCAL to ensure 12.8 MHz FOSC based on 1 kHz clock reference
//first do binary search in region of 192..255 (6 steps)
//then find best neighbour (2 steps)
//for atmega's at 12.8 MHz OSCCAL is typically > 210
//16-bit timer1 is used, stopped after

#define OSCCAL_FREF				(1000)				/*1 kHz*/
#define OSCCAL_INITIAL_VALUE	((192 + 256)/2)		/*=224*/
#define OSCCAL_INITIAL_STEP		((256 - 192)/2)		/*=32*/
#define OSCCAL_TARGET_PULSE		(F_CPU/OSCCAL_FREF)	/*=12800 clocks*/

#define OSCCAL_measure_pulse() {							\
	while (1 & (USB_CFG_IOPORT>>USB_CFG_DMINUS_BIT));	\
	measured_pulse = TCNT1;									\
	TCNT1 = 0;												\
}
#define OSCCAL_abs_diff(a, b) ((a>b) ? (a-b) : (b-a))

void calibrateOscillator(void) {
	unsigned char step;
	unsigned int measured_pulse;
	unsigned char osccal0;
	unsigned int error0, error_plus, error_minus;

	TCCR1B = 0x01; //1:1 prescaler
	step = OSCCAL_INITIAL_STEP;
	OSCCAL = OSCCAL_INITIAL_VALUE;
	OSCCAL_measure_pulse(); //wait for the first pulse

	//binary search
	do {
		OSCCAL_measure_pulse();
		if (measured_pulse < OSCCAL_TARGET_PULSE)
			OSCCAL -= step;	//too fast
		else
			OSCCAL += step;
		step >>= 1;
	} while (step > 0);

	//neighbourhood search
	osccal0 = OSCCAL;
	OSCCAL = osccal0 + 1;
	error0 = OSCCAL_abs_diff(measured_pulse, OSCCAL_TARGET_PULSE);
	OSCCAL_measure_pulse();
	error_plus = OSCCAL_abs_diff(measured_pulse, OSCCAL_TARGET_PULSE);

	OSCCAL = osccal0 - 1;
	OSCCAL_measure_pulse();
	error_minus = OSCCAL_abs_diff(measured_pulse, OSCCAL_TARGET_PULSE);

	//pick best value
	if ((error0 <= error_plus) && (error0 <= error_minus))
		OSCCAL = osccal0;
	if ((error_plus <= error0) && (error_plus <= error_minus))
		OSCCAL = osccal0 + 1;
	if ((error_minus <= error0) && (error_minus <= error_plus))
		OSCCAL = osccal0 - 1;

	TCCR1B = 0x00; //stop timer1
}
