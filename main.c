/*
TV-B-Gone Firmware version 1.2
for use with ATtiny85v and v1.2 hardware
(c) Mitch Altman + Limor Fried 2009
Last edits, August 16 2009

With some code from:
Kevin Timmerman & Damien Good 7-Dec-07

Distributed under Creative Commons 2.5 -- Attib & Share Alike

This is the 'universal' code designed for v1.2 - it will select EU or NA
depending on a pulldown resistor on pin B1 !
*/

#include <msp430.h>
#include <stdint.h>

#include "main.h"
#include "codes.h"

#define BIT_BUTTON BIT3
#define BIT_LED BIT6
#define BIT_IRLED BIT5

/*
This project transmits a bunch of TV POWER codes, one right after the other,
with a pause in between each.  (To have a visible indication that it is
transmitting, it also pulses a visible LED once each time a POWER code is
transmitted.)  That is all TV-B-Gone does.  The tricky part of TV-B-Gone
was collecting all of the POWER codes, and getting rid of the duplicates and
near-duplicates (because if there is a duplicate, then one POWER code will
turn a TV off, and the duplicate will turn it on again (which we certainly
do not want).  I have compiled the most popular codes with the
duplicates eliminated, both for North America (which is the same as Asia, as
far as POWER codes are concerned -- even though much of Asia USES PAL video)
and for Europe (which works for Australia, New Zealand, the Middle East, and
other parts of the world that use PAL video).

Before creating a TV-B-Gone Kit, I originally started this project by hacking
the MiniPOV kit.  This presents a limitation, based on the size of
the Atmel ATtiny2313 internal flash memory, which is 2KB.  With 2KB we can only
fit about 7 POWER codes into the firmware's database of POWER codes.  However,
the more codes the better! Which is why we chose the ATtiny85 for the
TV-B-Gone Kit.

This version of the firmware has the most popular 100+ POWER codes for
North America and 100+ POWER codes for Europe. You can select which region
to use by soldering a 10K pulldown resistor.
*/


/*
This project is a good example of how to use the AVR chip timers.
*/


/*
The hardware for this project is very simple:
     ATtiny85 has 8 pins:
       pin 1   RST + Button
       pin 2   one pin of ceramic resonator MUST be 8.0 mhz
       pin 3   other pin of ceramic resonator
       pin 4   ground
       pin 5   OC1A - IR emitters, through a '2907 PNP driver that connects
               to 4 (or more!) PN2222A drivers, with 1000 ohm base resistor
               and also connects to programming circuitry
       pin 6   Region selector. Float for US, 10K pulldown for EU,
               also connects to programming circuitry
       pin 7   PB0 - visible LED, and also connects to programming circuitry
       pin 8   +3-5v DC (such as 2-4 AA batteries!)
    See the schematic for more details.

    This firmware requires using an 8.0MHz ceramic resonator
       (since the internal oscillator may not be accurate enough).

    IMPORTANT:  to use the ceramic resonator, you must perform the following:
                    make burn-fuse_cr
*/

/* This function is the 'workhorse' of transmitting IR codes.
   Given the on and off times, it turns on the PWM output on and off
   to generate one 'pair' from a long code. Each code has ~50 pairs! */
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code )
{
  if(PWM_code) {
    // Start PWM output
    P1SEL |= BIT_IRLED;
  } else {
    // However some codes dont use PWM in which case we just turn the IR
    // LED on for the period of time.
    P1OUT |= BIT_IRLED;
  }

  // Now we wait, allowing the PWM hardware to pulse out the carrier
  // frequency for the specified 'on' time
  delay_ten_us(ontime);

  // Now we have to turn it off so disable the PWM output
  if(PWM_code)
    P1SEL &= ~BIT_IRLED;

  // And make sure that the IR LED is off too (since the PWM may have
  // been stopped while the LED is on!)
  P1OUT &= ~BIT_IRLED;

  // Now we wait for the specified 'off' time
  delay_ten_us(offtime);
}

/* This is kind of a strange but very useful helper function
   Because we are using compression, we index to the timer table
   not with a full 8-bit byte (which is wasteful) but 2 or 3 bits.
   Once code_ptr is set up to point to the right part of memory,
   this function will let us read 'count' bits at a time which
   it does by reading a byte into 'bits_r' and then buffering it. */

uint8_t bitsleft_r = 0;
uint8_t bits_r=0;
const uint8_t* code_ptr;

// we cant read more than 8 bits at a time so dont try!
uint8_t read_bits(uint8_t count)
{
  uint8_t tmp=0;

  uint8_t i;
  // we need to read back count bytes
  for (i=0; i<count; i++) {
    // check if the 8-bit buffer we have has run out
    if (bitsleft_r == 0) {
      // in which case we read a new byte in
      bits_r = *code_ptr++;
      // and reset the buffer size (8 bites in a byte)
      bitsleft_r = 8;
    }
    // remove one bit
    bitsleft_r--;
    // and shift it off of the end of 'bits_r'
    tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count-1-i));
  }

  // return the selected bits in the LSB part of tmp
  return tmp;
}


/*
The C compiler creates code that will transfer all constants into RAM when
the microcontroller resets.  Since this firmware has a table (powerCodes)
that is too large to transfer into RAM, the C compiler needs to be told to
keep it in program memory space.  This is accomplished by the macro PROGMEM
(this is used in the definition for powerCodes).  Since the C compiler assumes
that constants are in RAM, rather than in program memory, when accessing
powerCodes, we need to use the pgm_read_word() and pgm_read_byte macros, and
we need to use powerCodes as an address.  This is done with PGM_P, defined
below.
For example, when we start a new powerCode, we first point to it with the
following statement:
    PGM_P thecode_p = pgm_read_word(powerCodes+i);
The next read from the powerCode is a byte that indicates the carrier
frequency, read as follows:
      const uint8_t freq = pgm_read_byte(code_ptr++);
After that is a byte that tells us how many 'onTime/offTime' pairs we have:
      const uint8_t numpairs = pgm_read_byte(code_ptr++);
The next byte tells us the compression method. Since we are going to use a
timing table to keep track of how to pulse the LED, and the tables are
pretty short (usually only 4-8 entries), we can index into the table with only
2 to 4 bits. Once we know the bit-packing-size we can decode the pairs
      const uint8_t bitcompression = pgm_read_byte(code_ptr++);
Subsequent reads from the powerCode are n bits (same as the packing size)
that index into another table in ROM that actually stores the on/off times
      const PGM_P time_ptr = (PGM_P)pgm_read_word(code_ptr);
*/


int main(void)
{
  // Stop watchdog
  WDTCTL = WDTPW + WDTHOLD;

  // Set clock to 8 MHz
  // Set XT2 off
  DCOCTL = 0;
  BCSCTL1 = XT2OFF | CALBC1_8MHZ;
  DCOCTL = CALDCO_8MHZ;
  BCSCTL2 = 0;
  BCSCTL3 = 0;

  // Now, MCLK & SMCLK are 8 MHz

  // Make everything output, button an input
  P1DIR = ~BIT_BUTTON;
  P2DIR = 0xff;
  // Pull Up resistor
  P1REN = BIT_BUTTON;

  // Make everything a Digital I/O
  P1SEL = 0;
  P2SEL = 0;
  P1SEL2 = 0;
  P2SEL2 = 0;

  // turn off LEDs, and everything else
  // BIT_BUTTON is High for Pull Up
  P1OUT = BIT_BUTTON;
  P2OUT = 0;

  // enable button (P1.3) interrupt on low-to high transition
  P1IE = BIT_BUTTON;
  P1IES = 0;
  P2IE = 0;

  // enable interupts
  __eint();

  while(1)
  {
    // Low Power Mode
    LPM3;

    // Interrupt wakeup!
    delay_ten_us(25000);

    // Blast codes
    int i;
    for(i = 0; i < num_NAcodes; ++i)
    {
      blast_code(NApowerCodes[i]);
      quickflashLED();
      delay_ten_us(25000);
    }
    for(i = 0; i < num_EUcodes; ++i)
    {
      blast_code(EUpowerCodes[i]);
      quickflashLED();
      delay_ten_us(25000);
    }
  }

  return 0;
}

__attribute__((interrupt(PORT1_VECTOR)))
void port1_isr(void)
{
  // Reset interrupt flag
  P1IFG = 0;

  LPM3_EXIT;
}

void blast_code(const struct IrCode* code)
{
  code_ptr = code->codes;

  // Set carrier timer

  // use SMCLK, clear
  TACTL = TASSEL_2 | TACLR;
  // Set period
  TACCR0 = code->timer_val;
  // Set Compare mode, Toggle output
  TACCTL0 = OUTMOD_4;

  // Start timer
  TACTL |= MC_1;

  int i;
  for(i = 0; i < code->numpairs; ++i)
  {
    // Read the next 'n' bits as indicated by the compression variable
    uint8_t time_index = read_bits(code->bitcompression);
    xmitCodeElement(code->times[time_index * 2], code->times[time_index * 2 + 1], code->timer_val != 0);
  }
  //Flush remaining bits, so that next code starts
  //with a fresh set of 8 bits.
  bitsleft_r=0;

  // Stop Timer
  TACTL = TACLR;
}

#if 0
  uint16_t ontime, offtime;
  uint8_t i,j, Loop;
  uint8_t region = US;     // by default our code is US

  Loop = 0;                // by default we are not going to loop

  do {	//Execute the code at least once.  If Loop is on, execute forever.
      // Read the carrier frequency from the first byte of code structure
      const uint8_t freq = pgm_read_byte(code_ptr++);
      // set OCR for Timer1 to output this POWER code's carrier frequency
      OCR0A = freq;

      // Print out the frequency of the carrier and the PWM settings
      DEBUGP(putstring("\n\rOCR1: "); putnum_ud(freq););
      DEBUGP(uint16_t x = (freq+1) * 2; putstring("\n\rFreq: "); putnum_ud(F_CPU/x););

      // Get the number of pairs, the second byte from the code struct
      const uint8_t numpairs = pgm_read_byte(code_ptr++);
      DEBUGP(putstring("\n\rOn/off pairs: "); putnum_ud(numpairs));

      // Get the number of bits we use to index into the timer table
      // This is the third byte of the structure
      const uint8_t bitcompression = pgm_read_byte(code_ptr++);
      DEBUGP(putstring("\n\rCompression: "); putnum_ud(bitcompression));

      // Get pointer (address in memory) to pulse-times table
      // The address is 16-bits (2 byte, 1 word)
      const void* time_ptr = (void*)pgm_read_word(code_ptr);
      code_ptr+=2;

      // Transmit all codeElements for this POWER code
      // (a codeElement is an onTime and an offTime)
      // transmitting onTime means pulsing the IR emitters at the carrier
      // frequency for the length of time specified in onTime
      // transmitting offTime means no output from the IR emitters for the
      // length of time specified in offTime

      // For EACH pair in this code....
      for (uint8_t k=0; k<numpairs; k++) {
	uint8_t ti;

	// Read the next 'n' bits as indicated by the compression variable
	// The multiply by 4 because there are 2 timing numbers per pair
	// and each timing number is one word long, so 4 bytes total!
	ti = (read_bits(bitcompression)) * 4;

	// read the onTime and offTime from the program memory
	ontime = pgm_read_word(time_ptr+ti);  // read word 1 - ontime
	offtime = pgm_read_word(time_ptr+ti+2);  // read word 2 - offtime

	// transmit this codeElement (ontime and offtime)
	xmitCodeElement(ontime, offtime, (freq!=0));
      }

      //Flush remaining bits, so that next code starts
      //with a fresh set of 8 bits.
      bitsleft_r=0;

      // delay 250 milliseconds before transmitting next POWER code
      delay_ten_us(25000);

      // visible indication that a code has been output.
      quickflashLED();
    }
  } while (Loop == 1);

#endif


/****************************** LED AND DELAY FUNCTIONS ********/


// This function delays the specified number of 10 microseconds
void delay_ten_us(uint16_t us) {
  while(us--) {
    __delay_cycles(F_CPU / 100000);
  }
}


// This function quickly pulses the visible LED (connected to PB0, pin 5)
// This will indicate to the user that a code is being transmitted
void quickflashLED(void) {
  P1OUT |= BIT_LED;	// turn on visible LED at PB0 by pulling pin to ground
  delay_ten_us(3000);	// 30 millisec delay
  P1OUT &= ~BIT_LED;	// turn off visible LED at PB0 by pulling pin to +3V
}

