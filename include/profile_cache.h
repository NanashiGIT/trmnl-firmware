#pragma once

#include <api_types.h>
#include <stdint.h>

// Keeps a ready-to-draw image per device profile (see DeviceProfiles), so a
// profile button can switch the screen without Wi-Fi or an API round trip.
// Only built when DEVICE_PROFILE_COUNT is defined.

/// Remembers the image currently on screen as the given profile's cached image.
void profileCacheRememberCurrent(uint8_t profile);

/// Draws the profile's cached image. Returns false if there is none.
bool profileCacheShow(uint8_t profile);

/// Fetches the current image of every registered, inactive profile from the
/// API server and stores it in flash. Needs Wi-Fi; advances those profiles'
/// playlists like a regular refresh would.
void profileCachePrefetch(const ApiDisplayInputs &activeInputs);
