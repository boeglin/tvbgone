// What pins do what
#define LED PB2
#define IRLED PB0
#define REGIONSWITCH PB1

// Two regions!
#define US 0
#define EU 1

// set define to 0 to turn off debug output
#define DEBUG 0
#define DEBUGP(x) if (DEBUG == 1) { x ; }

// Shortcut to insert single, non-optimized-out nop
#define NOP __asm__ __volatile__ ("nop")

// Tweak this if neccessary to change timing
#define DELAY_CNT 11

#define PULSE_CODE 0

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void flashslowLEDx( uint8_t num_blinks );
void quickflashLEDx( uint8_t x );
void tvbgone_sleep( void );
void delay_ten_us(uint16_t us);
void quickflashLED( void );

