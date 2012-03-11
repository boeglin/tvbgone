// Changes to the 'demo-breathing-led' code by Rusty Haddock.
// Removed 1-sin(x) data array and now calculate an x^2 curve
// on the fly.

#include <msp430.h>
#include <stdint.h>

#define START_FREQ 15250000
#define CPU_FREQ 16000000
#define DELTA ((CPU_FREQ + 512) / 1024)
#define TONE 440

int main(void)
{
  // Stop watchdog
  WDTCTL = WDTPW + WDTHOLD;

  // Set clock to 15.25 MHz: RSEL15, DCO3, MOD0;
  // XT2 off, XTS lf, DIVA 3
  DCOCTL = 0;
  BCSCTL1 = XT2OFF | DIVA_3 | 0x0f;
  DCOCTL = 0x60;

  // SELM dco, DIVM 0, SELS dco, DIVS 0, DCOR 0
  // BCSCTL2 No change

  // XT2S 0, LFXT1S 0, XCAP 12.5pF
  BCSCTL3 |= XCAP_3;

  // Make P1.6 (green led) an output. SLAU144E p.8-3
  P1DIR |= BIT6 | BIT0;
  P1DIR &= ~BIT3;

  // P1.6 = TA0.1 (timer A's output). SLAS694C p.41
  P1SEL |= BIT6;
  P1SEL &= ~(BIT0 | BIT3);

  // turn off LED
  P1OUT &= ~BIT0;

  // set cycle length
  TACCR0 = (START_FREQ / TONE) - 1;

  // Source Timer A from SMCLK (TASSEL_2), up mode (MC_1).
  // Up mode counts up to TACCR0. SLAU144E p.12-20
  TACTL = TASSEL_2 | MC_1;

  // OUTMOD_3 = Set/reset output when the timer counts to TACCR1/TACCR0
  // CCIE = Interrupt when timer counts to TACCR1
  TACCTL1 = OUTMOD_3;// | CCIE;

  // CCR1 (= CCR0 / 2)
  TACCR1 = (TACCR0 / 2) + 1;

  // enable button (P1.3) interrupt
  P1IE |= BIT3;

  // enable interupts
  __eint();

  while(1);
  return 0;
}

__attribute__((interrupt(PORT1_VECTOR)))
void port1_isr(void)
{
  uint16_t last, ldco, lbcs;
  int16_t diff = 0;

  P1IFG = 0;
  P1SEL &= ~BIT6;
  P1OUT &= ~BIT6;

  // store previous values
  ldco = DCOCTL;
  lbcs = BCSCTL1;
  P1OUT &= ~BIT0;

  /* calibrate */
  while(diff != DELTA)
  {
    // use SMCLK, clear
    TACTL = TASSEL_2 | TACLR;
    // capture on rising edge of ACLK 
    TACCTL0 = CM_1 | CCIS_1 | CAP;

    // start the timer
    TACTL |= MC_2;

    // capture 0
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));
    last = TACCR0;

    // capture 1
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));

    // capture 2
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));

    // capture 3
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));

    // capture 4
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));
    diff = TACCR0 - last;

    if(diff > DELTA)
    {
      DCOCTL--;
      if(DCOCTL == 0xff)
      {
	DCOCTL = 0;
	BCSCTL1--;
	DCOCTL = 0xff;
      }
    }
    else if (diff < DELTA)
    {
      DCOCTL++;
      if(DCOCTL == 0)
      {
	BCSCTL1++;
      }
    }
  }

  // check for a change
  if(ldco != DCOCTL || lbcs != BCSCTL1)
  {
    P1OUT |= BIT0;
  }

  // reset timer A, P1.6
  TACCR0 = (CPU_FREQ / TONE) - 1;
  TACCR1 = (TACCR0 / 2) + 1;
  TACTL = TASSEL_2 | MC_1;
  TACCTL0 = 0;
  TACCTL1 = OUTMOD_3;

  P1SEL |= BIT6;
}

