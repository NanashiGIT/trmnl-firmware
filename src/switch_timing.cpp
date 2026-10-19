#include <switch_timing.h>

#ifdef SWITCH_TIMING

#include <Arduino.h>
#include <globals.h>

static char timings[480];
static size_t used = 0;

void timing_mark(const char *name) {
  int n = snprintf(timings + used, sizeof(timings) - used, "%s=%lu;", name, (unsigned long)millis());
  if (n > 0 && used + n < sizeof(timings)) used += n;
}

void timing_save() {
  timing_mark("sleep");
  preferences.putString("dbg_t", timings);
}

#endif // SWITCH_TIMING
