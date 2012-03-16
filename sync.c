#include <msp430.h>
#include <stdint.h>
#include "sync.h"

// Divide by 4096
#define DELTA (F_CPU >> 12)

void sync(void)
{
  uint16_t last;
  int16_t diff = 0;

  // use SMCLK, clear
  TACTL = TASSEL_2 | TACLR;
  // capture on rising edge of ACLK 
  TACCTL0 = CM_1 | CCIS_1 | CAP;

  /* calibrate */
  while(diff != DELTA)
  {
    // Start the timer
    TACTL |= MC_2;

    // Capture 0
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));
    last = TACCR0;

    // Capture 1
    TACCTL0 &= ~CCIFG;
    while(!(TACCTL0 & CCIFG));
    diff = TACCR0 - last;

    // Stop the timer
    TACTL |= TACLR;

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
}

