#include <device_profiles.h>
#include <stdio.h>

DeviceProfiles::DeviceProfiles(Persistence &persistence, uint8_t count)
    : persistence(persistence), profileCount(count > 0 ? count : 1) {}

uint8_t DeviceProfiles::active() {
  // Cached so the MAC stays stable even after preferences are closed for sleep.
  if (cachedActive < 0) {
    uint8_t stored = persistence.readUChar(ACTIVE_KEY, 0);
    cachedActive = stored < profileCount ? stored : 0;
  }
  return (uint8_t)cachedActive;
}

uint8_t DeviceProfiles::selectNext() { return select((active() + 1) % profileCount); }

uint8_t DeviceProfiles::selectPrevious() { return select((active() + profileCount - 1) % profileCount); }

uint8_t DeviceProfiles::select(uint8_t profile) {
  uint8_t current = active();
  if (profile == current) return current;

  stash(API_KEY_KEY, current);
  stash(FRIENDLY_ID_KEY, current);
  restore(API_KEY_KEY, profile);
  restore(FRIENDLY_ID_KEY, profile);

  persistence.writeUChar(ACTIVE_KEY, profile);
  cachedActive = profile;
  return profile;
}

String DeviceProfiles::apiKey(uint8_t profile) { return identity(API_KEY_KEY, profile); }

String DeviceProfiles::friendlyId(uint8_t profile) { return identity(FRIENDLY_ID_KEY, profile); }

String DeviceProfiles::cachedImage(uint8_t profile) {
  String slot = slotKey(IMAGE_KEY, profile);
  return persistence.recordExists(slot.c_str()) ? persistence.readString(slot.c_str(), "") : String("");
}

void DeviceProfiles::setCachedImage(uint8_t profile, const String &path) {
  String slot = slotKey(IMAGE_KEY, profile);
  if (path.length() > 0) {
    if (cachedImage(profile) != path) persistence.writeString(slot.c_str(), path.c_str());
  } else if (persistence.recordExists(slot.c_str())) {
    persistence.remove(slot.c_str());
  }
}

String DeviceProfiles::macForProfile(const String &hardwareMac, uint8_t profile) {
  unsigned int octets[6];
  if (profile == 0 || sscanf(hardwareMac.c_str(), "%2x:%2x:%2x:%2x:%2x:%2x", &octets[0], &octets[1], &octets[2],
                             &octets[3], &octets[4], &octets[5]) != 6) {
    return hardwareMac;
  }

  octets[0] |= 0x02;
  octets[5] ^= profile;

  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", octets[0] & 0xff, octets[1] & 0xff, octets[2] & 0xff,
           octets[3] & 0xff, octets[4] & 0xff, octets[5] & 0xff);
  return String(mac);
}

String DeviceProfiles::identity(const char *key, uint8_t profile) {
  String stored = profile == active() ? String(key) : slotKey(key, profile);
  return persistence.recordExists(stored.c_str()) ? persistence.readString(stored.c_str(), "") : String("");
}

void DeviceProfiles::stash(const char *key, uint8_t profile) {
  String slot = slotKey(key, profile);
  if (persistence.recordExists(key)) {
    persistence.writeString(slot.c_str(), persistence.readString(key, "").c_str());
  } else if (persistence.recordExists(slot.c_str())) {
    persistence.remove(slot.c_str());
  }
}

void DeviceProfiles::restore(const char *key, uint8_t profile) {
  String slot = slotKey(key, profile);
  if (persistence.recordExists(slot.c_str())) {
    persistence.writeString(key, persistence.readString(slot.c_str(), "").c_str());
  } else if (persistence.recordExists(key)) {
    persistence.remove(key);
  }
}

// e.g. "friendly_id_p2", within the 15 character NVS key limit
String DeviceProfiles::slotKey(const char *key, uint8_t profile) { return String(key) + "_p" + String(profile); }
