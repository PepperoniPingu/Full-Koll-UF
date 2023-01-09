#include "wdt.h"

void wdt_enable() {
  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_4KCLK_gc); // No window, 2 seconds
}

void wdt_reset() {
  __asm__ __volatile__ ("wdr"::);
}

void wdt_disable() {
  _PROTECTED_WRITE(WDT.CTRLA,0);
}
