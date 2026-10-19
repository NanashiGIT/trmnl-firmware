#pragma once

#include <persistence_interface.h>
#include <stdint.h>

// Lets one physical device register with the server under several identities
// ("profiles"), so each identity can be given its own playlist. Every profile
// sends its own MAC address to /api/setup and keeps its own API key and
// friendly ID. Only the active profile's identity lives under the regular
// NVS keys, so the rest of the firmware does not need to know about profiles.
class DeviceProfiles {
public:
  static constexpr const char *ACTIVE_KEY = "profile";
  // Must match PREFERENCES_API_KEY and PREFERENCES_FRIENDLY_ID in config.h.
  static constexpr const char *API_KEY_KEY = "api_key";
  static constexpr const char *FRIENDLY_ID_KEY = "friendly_id";
  static constexpr const char *IMAGE_KEY = "pf_img";

  DeviceProfiles(Persistence &persistence, uint8_t count);

  uint8_t count() const { return profileCount; }

  // Active profile index, 0 when unset or out of range.
  uint8_t active();

  // Stash the active profile's identity, swap in the next / previous
  // profile's (or clear it, so the next API call registers the profile) and
  // return the new active profile. Both wrap around.
  uint8_t selectNext();
  uint8_t selectPrevious();

  // Identity of any profile, active or stashed; empty if it never registered.
  String apiKey(uint8_t profile);
  String friendlyId(uint8_t profile);

  // Image file last shown or prefetched for a profile, empty if none.
  String cachedImage(uint8_t profile);
  void setCachedImage(uint8_t profile, const String &path);

  // Profile 0 keeps the hardware MAC. Other profiles set the locally
  // administered bit and XOR the profile index into the last octet.
  static String macForProfile(const String &hardwareMac, uint8_t profile);

private:
  uint8_t select(uint8_t profile);
  String identity(const char *key, uint8_t profile);
  void stash(const char *key, uint8_t profile);
  void restore(const char *key, uint8_t profile);
  static String slotKey(const char *key, uint8_t profile);

  Persistence &persistence;
  uint8_t profileCount;
  int16_t cachedActive = -1;
};
