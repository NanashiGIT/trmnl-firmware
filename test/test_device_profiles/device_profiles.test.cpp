#include <device_profiles.h>
#include <memory_persistence.h>
#include <unity.h>

// The swap relies on these matching the regular identity keys in config.h.
void test_nvs_keys_are_stable(void) {
  TEST_ASSERT_EQUAL_STRING("profile", DeviceProfiles::ACTIVE_KEY);
  TEST_ASSERT_EQUAL_STRING("api_key", DeviceProfiles::API_KEY_KEY);
  TEST_ASSERT_EQUAL_STRING("friendly_id", DeviceProfiles::FRIENDLY_ID_KEY);
  TEST_ASSERT_EQUAL_STRING("pf_img", DeviceProfiles::IMAGE_KEY);
}

void test_profile_zero_keeps_the_hardware_mac(void) {
  TEST_ASSERT_EQUAL_STRING("44:BD:8D:EC:2D:04", DeviceProfiles::macForProfile("44:BD:8D:EC:2D:04", 0).c_str());
}

void test_other_profiles_get_distinct_locally_administered_macs(void) {
  TEST_ASSERT_EQUAL_STRING("46:BD:8D:EC:2D:05", DeviceProfiles::macForProfile("44:BD:8D:EC:2D:04", 1).c_str());
  TEST_ASSERT_EQUAL_STRING("46:BD:8D:EC:2D:06", DeviceProfiles::macForProfile("44:BD:8D:EC:2D:04", 2).c_str());
}

void test_profile_mac_differs_even_if_hardware_mac_is_locally_administered(void) {
  TEST_ASSERT_EQUAL_STRING("46:00:00:00:00:01", DeviceProfiles::macForProfile("46:00:00:00:00:00", 1).c_str());
}

void test_malformed_mac_is_returned_unchanged(void) {
  TEST_ASSERT_EQUAL_STRING("not-a-mac", DeviceProfiles::macForProfile("not-a-mac", 1).c_str());
}

void test_active_defaults_to_zero_and_ignores_out_of_range_values(void) {
  MemoryPersistence persistence;
  TEST_ASSERT_EQUAL_UINT8(0, DeviceProfiles(persistence, 3).active());

  persistence.writeUChar(DeviceProfiles::ACTIVE_KEY, 7);
  TEST_ASSERT_EQUAL_UINT8(0, DeviceProfiles(persistence, 3).active());
}

void test_select_next_cycles_through_all_profiles(void) {
  MemoryPersistence persistence;
  DeviceProfiles profiles(persistence, 3);
  TEST_ASSERT_EQUAL_UINT8(1, profiles.selectNext());
  TEST_ASSERT_EQUAL_UINT8(2, profiles.selectNext());
  TEST_ASSERT_EQUAL_UINT8(0, profiles.selectNext());
  TEST_ASSERT_EQUAL_UINT8(0, persistence.readUChar(DeviceProfiles::ACTIVE_KEY, 99));
}

void test_select_previous_cycles_backwards(void) {
  MemoryPersistence persistence;
  DeviceProfiles profiles(persistence, 3);
  TEST_ASSERT_EQUAL_UINT8(2, profiles.selectPrevious());
  TEST_ASSERT_EQUAL_UINT8(1, profiles.selectPrevious());
  TEST_ASSERT_EQUAL_UINT8(0, profiles.selectPrevious());
}

void test_previous_after_next_returns_to_the_same_identity(void) {
  MemoryPersistence persistence;
  persistence.writeString("api_key", "key0");
  persistence.writeString("friendly_id", "AAAAAA");
  DeviceProfiles profiles(persistence, 3);

  profiles.selectNext();
  persistence.writeString("api_key", "key1");
  profiles.selectPrevious();

  TEST_ASSERT_EQUAL_UINT8(0, profiles.active());
  TEST_ASSERT_EQUAL_STRING("key0", persistence.readString("api_key", "").c_str());
  TEST_ASSERT_EQUAL_STRING("AAAAAA", persistence.readString("friendly_id", "").c_str());
  TEST_ASSERT_EQUAL_STRING("key1", persistence.readString("api_key_p1", "").c_str());
}

void test_switching_to_a_new_profile_clears_the_identity_so_it_registers(void) {
  MemoryPersistence persistence;
  persistence.writeString("api_key", "key0");
  persistence.writeString("friendly_id", "AAAAAA");
  DeviceProfiles profiles(persistence, 2);

  profiles.selectNext();

  TEST_ASSERT_FALSE(persistence.recordExists("api_key"));
  TEST_ASSERT_FALSE(persistence.recordExists("friendly_id"));
  TEST_ASSERT_EQUAL_STRING("key0", persistence.readString("api_key_p0", "").c_str());
  TEST_ASSERT_EQUAL_STRING("AAAAAA", persistence.readString("friendly_id_p0", "").c_str());
}

void test_switching_back_restores_each_profiles_identity(void) {
  MemoryPersistence persistence;
  persistence.writeString("api_key", "key0");
  persistence.writeString("friendly_id", "AAAAAA");
  DeviceProfiles profiles(persistence, 2);

  profiles.selectNext();
  // profile 1 registers with the server
  persistence.writeString("api_key", "key1");
  persistence.writeString("friendly_id", "BBBBBB");
  profiles.selectNext();

  TEST_ASSERT_EQUAL_UINT8(0, profiles.active());
  TEST_ASSERT_EQUAL_STRING("key0", persistence.readString("api_key", "").c_str());
  TEST_ASSERT_EQUAL_STRING("AAAAAA", persistence.readString("friendly_id", "").c_str());

  profiles.selectNext();
  TEST_ASSERT_EQUAL_STRING("key1", persistence.readString("api_key", "").c_str());
  TEST_ASSERT_EQUAL_STRING("BBBBBB", persistence.readString("friendly_id", "").c_str());
}

void test_identity_is_readable_for_active_and_stashed_profiles(void) {
  MemoryPersistence persistence;
  persistence.writeString("api_key", "key0");
  persistence.writeString("friendly_id", "AAAAAA");
  DeviceProfiles profiles(persistence, 3);

  profiles.selectNext();
  persistence.writeString("api_key", "key1");

  TEST_ASSERT_EQUAL_STRING("key1", profiles.apiKey(1).c_str());
  TEST_ASSERT_EQUAL_STRING("key0", profiles.apiKey(0).c_str());
  TEST_ASSERT_EQUAL_STRING("AAAAAA", profiles.friendlyId(0).c_str());
  TEST_ASSERT_EQUAL_STRING("", profiles.apiKey(2).c_str());
  TEST_ASSERT_EQUAL_STRING("", profiles.friendlyId(1).c_str());
}

void test_cached_image_is_stored_per_profile(void) {
  MemoryPersistence persistence;
  DeviceProfiles profiles(persistence, 3);

  TEST_ASSERT_EQUAL_STRING("", profiles.cachedImage(1).c_str());
  profiles.setCachedImage(1, "/screen-a-1791419471");
  profiles.setCachedImage(2, "/screen-b-1791419471");
  TEST_ASSERT_EQUAL_STRING("/screen-a-1791419471", profiles.cachedImage(1).c_str());
  TEST_ASSERT_EQUAL_STRING("/screen-b-1791419471", profiles.cachedImage(2).c_str());

  uint32_t writes = persistence.writeCount;
  profiles.setCachedImage(1, "/screen-a-1791419471");
  TEST_ASSERT_EQUAL_UINT32(writes, persistence.writeCount);

  profiles.setCachedImage(1, "");
  TEST_ASSERT_FALSE(persistence.recordExists("pf_img_p1"));
}

void test_a_single_profile_never_changes_identity(void) {
  MemoryPersistence persistence;
  persistence.writeString("api_key", "key0");
  DeviceProfiles profiles(persistence, 1);

  TEST_ASSERT_EQUAL_UINT8(0, profiles.selectNext());
  TEST_ASSERT_EQUAL_UINT8(0, profiles.selectPrevious());
  TEST_ASSERT_EQUAL_STRING("key0", persistence.readString("api_key", "").c_str());
  TEST_ASSERT_FALSE(persistence.recordExists("api_key_p0"));
}

void setUp(void) {}

void tearDown(void) {}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_nvs_keys_are_stable);
  RUN_TEST(test_profile_zero_keeps_the_hardware_mac);
  RUN_TEST(test_other_profiles_get_distinct_locally_administered_macs);
  RUN_TEST(test_profile_mac_differs_even_if_hardware_mac_is_locally_administered);
  RUN_TEST(test_malformed_mac_is_returned_unchanged);
  RUN_TEST(test_active_defaults_to_zero_and_ignores_out_of_range_values);
  RUN_TEST(test_select_next_cycles_through_all_profiles);
  RUN_TEST(test_select_previous_cycles_backwards);
  RUN_TEST(test_previous_after_next_returns_to_the_same_identity);
  RUN_TEST(test_switching_to_a_new_profile_clears_the_identity_so_it_registers);
  RUN_TEST(test_switching_back_restores_each_profiles_identity);
  RUN_TEST(test_identity_is_readable_for_active_and_stashed_profiles);
  RUN_TEST(test_cached_image_is_stored_per_profile);
  RUN_TEST(test_a_single_profile_never_changes_identity);
  UNITY_END();
}

int main(int argc, char **argv) {
  process();
  return 0;
}
