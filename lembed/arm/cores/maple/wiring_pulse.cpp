#include <wiring_pulse.h>
#include "boards.h"





/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse. */


uint32_t pulseIn( uint32_t pin, uint32_t state, uint32_t timeout )
{
  // cache the port and bit of the pin in order to speed up the
  // pulse width measuring loop and achieve finer resolution.  calling
  // digitalRead() instead yields much coarser resolution.

  gpio_dev *dev = PIN_MAP[pin].gpio_device;
  uint32_t bit = (1U << PIN_MAP[pin].gpio_bit);


  uint32_t width = 0; // keep initialization out of time critical area

  // convert the timeout from microseconds to a number of times through
  // the initial loop; it takes 16 clock cycles per iteration.
  uint32_t numloops = 0;
  uint32_t maxloops =  timeout * ( F_CPU / 16000000);
  volatile uint32_t dummyWidth = 0;

  // wait for any previous pulse to end
  while ( (dev->regs->IDR & bit)  == bit)   {
    if (numloops++ == maxloops)  {
      return 0;
    }
    dummyWidth++;
  }

  // wait for the pulse to start
  while ((dev->regs->IDR & bit) != bit)   {
    if (numloops++ == maxloops) {
      return 0;
    }
    dummyWidth++;
  }

  // wait for the pulse to stop
  while ((dev->regs->IDR & bit) == bit) {
    if (numloops++ == maxloops)  {
      return 0;
    }
    width++;
  }

  // Excluding time taking up by the interrupts, it needs 16 clock cycles to look through the last while loop
  // 5 is added as a fiddle factor to correct for interrupts etc. But ultimately this would only be accurate if it was done one hardware timer
  return (uint32_t)( ( (unsigned long long)(width + 5) *  (unsigned long long) 16000000.0) / (unsigned long long)F_CPU );
}
