#include "device_id.h"

#include <WiFi.h>
#include <globals.h>

String device_mac_address() {
#ifdef TEST_MAC_ADDRESS
  return TEST_MAC_ADDRESS;
#elif defined(DEVICE_PROFILE_COUNT)
  return DeviceProfiles::macForProfile(WiFi.macAddress(), deviceProfiles.active());
#else
  return WiFi.macAddress();
#endif
}