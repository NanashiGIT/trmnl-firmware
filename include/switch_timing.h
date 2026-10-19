#pragma once

// Debug-only wake profiling: TIMING_MARK records millis() at named points and
// TIMING_SAVE stores "name=ms;..." in NVS (key "dbg_t") right before deep sleep,
// where it can be read back with esptool. Enabled by -D SWITCH_TIMING.
#ifdef SWITCH_TIMING
void timing_mark(const char *name);
void timing_save();
#define TIMING_MARK(name) timing_mark(name)
#define TIMING_SAVE() timing_save()
#else
#define TIMING_MARK(name)
#define TIMING_SAVE()
#endif
