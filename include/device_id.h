#pragma once

#include <Arduino.h>

/// Returns the device's MAC address; overridable by defining TEST_MAC_ADDRESS.
/// With DEVICE_PROFILE_COUNT set, this is the active profile's MAC (see DeviceProfiles).
String device_mac_address();