#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


#define INPUT 4 // PB4
#define DISPLAY_PWM 3 // PB3
#define DS 0 // PB0
#define STCP 2 // PB2
#define SHCP 1 // PB1


/*** HELPERS ***/

static void custom_delay_ms(unsigned short ms)
{
	while (ms--) {
		_delay_us(1);
	}
}

static void switchOn(char reg, unsigned short ms)
{
	PORTB |= 1 << reg;
	asm("nop");
	custom_delay_ms(ms);
}

static void switchOff(char reg, unsigned short ms)
{
	PORTB &= ~(1 << reg);
	asm("nop");
	custom_delay_ms(ms);
}


static void sendBit(unsigned char bit)
{
	if (bit) {
		switchOn(DS, 0);
	} else {
		switchOff(DS, 0);
	}
	switchOn(SHCP, 0);
	switchOff(SHCP, 0);
	switchOff(DS, 0);
}

static unsigned char bits[8];

static void decToBytes(unsigned char d)
{
	unsigned char bytesLeft = 7;
	while (1) {
		bits[bytesLeft] = d % 2;
		d = d / 2;
		if (! bytesLeft) {
			break;
		} else {
			bytesLeft--;
		}
	}
}


static unsigned char segments[10] = {
	/**
	 * [A] [B] [C] [D] [E] [F] [G] [.]
	 *
	 * + A +
	 * F   B
	 * + G +
	 * E   C
	 * + D +
	 */
	0xFC, // 1 1 0 1 1 1 0 0
	0x60, // 0 1 1 0 0 0 0 0
	0xDA, // 1 1 0 1 1 0 1 0
	0xF2, // 1 1 1 1 0 0 1 0
	0x66, // 0 1 1 0 0 1 1 0

	0xB6, // 1 0 1 1 0 1 1 0
	0xBE, // 1 0 1 1 1 1 1 0
	0xE0, // 1 1 1 0 0 0 0 0
	0xFE, // 1 1 1 1 1 1 1 0
	0xE6, // 1 1 1 0 0 1 1 0
};


void sendToShift(unsigned char digit)
{
	decToBytes(segments[digit]);

	sendBit(bits[6]);
	sendBit(bits[5]);
	sendBit(bits[4]);
	sendBit(bits[3]);
	sendBit(bits[2]);
	sendBit(bits[1]);
	sendBit(bits[0]);
	sendBit(0);
}


void sendToOutput()
{
	switchOn(STCP, 0);
	switchOff(STCP, 0);
}


static unsigned long counter = 0;
static unsigned short displayCounter = 0;


/*** MAIN ***/

int __attribute__((noreturn)) main(void)
{
	// inputs
	DDRB &= ~(1 << INPUT);
	PINB |= 1 << INPUT; // set it high

	// outputs
	DDRB |= 1 << DS;
	DDRB |= 1 << STCP;
	DDRB |= 1 << SHCP;
	DDRB |= 1 << DISPLAY_PWM;

	sendToShift(8);
	sendToShift(8);
	sendToShift(8);
	sendToOutput();

	unsigned long displayFreq = 0;
	unsigned char isReady = 1;

	cli();
	TCCR0B |= (1 << CS00);
	TIMSK0 |= (1 << TOIE0);
	TCNT0 = 0;
	sei();

	counter = 0;
	_delay_ms(200);


	for (;;) {

		if (~PINB & (1 << INPUT)) { // key down

			if (isReady) {
				isReady = 0;
				displayFreq = 6000000 / (counter * 228); // timer cycle length in ms * 100
				displayFreq *= 2;
				counter = 0;

				sendToShift((displayFreq / 100) % 10);
				sendToShift((displayFreq / 10) % 10);
				sendToShift(displayFreq % 10);

				sendToOutput();

				_delay_us(100);
			}

		} else { // key up
			isReady = 1;
		}

	}
}


/**
 * Called when TIMER0 overflows which is
 * ca. F_CPU / 256 times a second (without timer prescaling).
 */
ISR(TIM0_OVF_vect)
{
	counter++;

	/**
	 * Display PWM
	 */
	/*
	displayCounter++;

	if (displayCounter > 10) {
		displayCounter = 0;
	}

	if (displayCounter < 1) {
		PORTB |= (1 << DISPLAY_PWM); // ON
	} else {
		PORTB &= ~(1 << DISPLAY_PWM); // OFF
	}
	*/

}

