
/******************                                ****************
  
#define	TM1638_DIO_PIN			PB0
#define	TM1638_CLK_PIN			PB1
#define	TM1638_STB_PIN			PB2

adcforward = PC0
ADCreverse = PC1

motor fwd = pC4
motor rev = pc5
motor pwm = pb3




*************************************************************************/



#include <avr/io.h>
#include "avrcommon.h"
#include <avr/interrupt.h>
#include "tm1638.h"

// misc
#define OUTPUT              1
#define INPUT               0


volatile int    position;           // note that this is signe


void Delay(unsigned long v);

void         pwm_init     ( );
void         SetSpeed     ( int Speed );
void         setupEncoder ( void );
void         updatePos    ( void );
void         speed_init   ( ) ;
int          rollDigit    ( int v, int p) ;
void         adcInit      ( void);





volatile unsigned int  AdcValues[2];





int main (void) {    

  uint8_t keys, okeys, newkeys;
  // set up directions 
  int wp;
  int cmdSpeed;
  char automatic;

  DDRB = (INPUT << PB0 | INPUT << PB1 |INPUT << PB2 |OUTPUT << PB3 | INPUT << PB4 |INPUT << PB5 |INPUT << PB6 |INPUT << PB7);     
  DDRC = (INPUT << PC0 | INPUT << PC1 |INPUT << PC2 |INPUT << PC3 | OUTPUT << PC4 |OUTPUT << PC5 |INPUT << PC6 );            
  DDRD = (INPUT << PD0 | INPUT << PD1 |INPUT << PD2 |INPUT << PD3 | INPUT << PD4 |INPUT << PD5 |INPUT << PD6 |INPUT << PD7);        

  PORTC |= 0x06;

  pwm_init();
  adcInit();
  setupEncoder(); 
  sei();
  
  TM1638_init(1, 2);
  okeys = 0;
  
  TM1638_set_led(1, 0);
  TM1638_set_led(2, 1);
  TM1638_set_led(3, 0);
  TM1638_set_led(4, 0);
  TM1638_set_led(5, 0);
  TM1638_set_led(6, 0);
  TM1638_set_led(7, 0);
  TM1638_set_led(8, 0);
  TM1638_display_segments(0, 0);
  TM1638_display_segments(1, 0);
  TM1638_display_segments(2, 0);
  TM1638_display_segments(3, 0);
  TM1638_display_segments(4, 0);
  TM1638_display_segments(5, 0);
  TM1638_display_segments(6, 0);
  TM1638_display_segments(7, 0);
  
  automatic = 1;

  while(1) {              
           
    keys = TM1638_scan_keys();
    
    newkeys = (okeys ^ keys) & keys;
    
    if (!newkeys) {     
    } else if (newkeys & 0x01) {
     // TM1638_set_led(1, 1);
      position = 0;
    } else if (newkeys & 0x02) {     
     // TM1638_set_led(2, 1);
     if (automatic) {
       automatic = 0;
       TM1638_set_led(2, 0);
     } else {
       automatic = 1;
       TM1638_set_led(2, 1);
     }
    } else if (newkeys & 0x04) {
    } else if (newkeys & 0x08) {
      position = rollDigit(position, 4);
     // TM1638_set_led(4, 1);
    } else if (newkeys & 0x10) {
      position = rollDigit(position, 3);
     // TM1638_set_led(5, 1);
    } else if (newkeys & 0x20) {   
      position = rollDigit(position, 2);
    //  TM1638_set_led(6, 1);
    } else if (newkeys & 0x40) {
      position = rollDigit(position, 1);
    //  TM1638_set_led(7, 1);
    } else if (newkeys & 0x80) {   
      position = rollDigit(position, 0);
    //  TM1638_set_led(8, 1);
    }
    okeys = keys;

    wp = (position > 0) ? position : -position;
    
    TM1638_display_segments(0, (position>=0)?0x00:0x40);

    TM1638_display_digit(7, wp/1 %10, 0);
    TM1638_display_digit(6, wp/10 %10, 0);
    TM1638_display_digit(5, wp/100 %10, 0);
    TM1638_display_digit(4, wp/1000 %10, 0);
    TM1638_display_digit(3, wp/10000 %10, 0);
    
    
    cmdSpeed = (AdcValues[0]/4) - (AdcValues[1]/4);
    
    if (automatic) {
    if (0) {
      } else if (position <= 0) {
	SetSpeed ((cmdSpeed > 0)?0:cmdSpeed); // limit 0
      } else if (position < 2) {
	SetSpeed ((cmdSpeed > 50)?50:cmdSpeed); // limit 100
      } else if (position < 4) { 
	SetSpeed ((cmdSpeed > 100)?100:cmdSpeed); // limit 200
      } else {
	SetSpeed(cmdSpeed);
      }
    } else {
      SetSpeed(cmdSpeed);
    }
     
  }
     
} // end of main

//=============================| Functions |==========================


// ======================= display stuff ====================

unsigned long pa[] = {1, 10, 100, 1000, 10000, 100000, 1000000 };

int rollDigit(int v, int p) {
  int  B, C, D, E, F;
  volatile int A;
  A = v / pa[p+1] ; // left half after digit
  B = v / pa[p];
  C = A * 10;
  D = (B - C);                // the digit  
  
  E = D * pa[p];
  F = v - E;          // the position is cleared  
  D = (D>=9)?0:D+1;          // increment with non-carry rollover  

  return (F+(D*pa[p]));

}


//=============== Encoder stuff ======================

void setupEncoder() {
        // clear position
        position = 0; 
 
        // we need to set up int0 and int1 to 
        // trigger interrupts on both edges
        MCUCR =  (1<<ISC00) | (1<<ISC01);
 
        // then enable them.
        GICR  = (1<<INT0) ; 
}

// SIGNAL (SIG_INTERRUPT0) {  // fix this signal name!!! INT0
ISR(INT0_vect){
   if (PIND & 0x08) position++;
   else             position--;
}



// ================= motor stuff =================


void pwm_init() {
  // clear pwm levels
  OCR2 = 64; 
  
  // set up WGM, clock, and mode for timer 0
  TCCR2 = 1 << WGM20  | /* fast pwm */
          1 << COM21  | /* inverted */
          0 << COM20  | /*   this bit 1 for inverted, 0 for normal  */
          1 << WGM21  |
          1 << CS22   | /* prescale by 128*/
          0 << CS21   |
          0 << CS20   ;
  
 }


void  SetSpeed ( int speed ) {
  
  if (speed >= 0) {
     OCR2 = (speed > 8)?speed:0;
     SetBit(4, PORTC);
     ClearBit(5, PORTC);
  } else {
     OCR2 = (-speed > 8)?-speed:0;
     SetBit(5, PORTC);
     ClearBit(4, PORTC);
  }  

}

//=======================


void Delay(unsigned long v) {
 for(; v > 0; v--) NOP();
}

//======================= ADC stuff ======================


void adcInit(void) {
  ADMUX = 0;          
  ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS0)|(1<<ADIE)|(1<<ADSC);   // clock/32  (8Mhz/32 = 250Khz)
}


ISR(ADC_vect) {
  unsigned char i;

  i = ADMUX &7;                         // what channel we on?
  AdcValues[i] = ADC;                     // save value
  i = (i + 1) & 1;                     // next value (0 thru 1)
  ADMUX = i ;                             // select channel
  ADCSR |= _BV(ADSC);                     // start next conversion
   
}







































































