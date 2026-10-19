#include <profile_cache.h>

#ifdef DEVICE_PROFILE_COUNT

#include <WiFi.h>
#include <api-client/display.h>
#include <api-client/request_headers.h>
#include <display.h>
#include <filesystem.h>
#include <globals.h>
#include <png.h>
#include <services/http_retry_request.h>
#include <trmnl_log.h>

#include "displayed_image.h"

void profileCacheRememberCurrent(uint8_t profile) {
  deviceProfiles.setCachedImage(profile, preferences.getString(PREFERENCES_CURRENT_PATH_KEY, ""));
}

bool profileCacheShow(uint8_t profile) {
  String path = deviceProfiles.cachedImage(profile);
  if (path.isEmpty() || !filesystem_file_exists(path.c_str())) {
    Log_info("No cached image for profile %u", profile);
    return false;
  }

  uint8_t *image = nullptr;
  size_t size = filesystem_read_and_allocate(path.c_str(), &image);
  if (!image || size == 0) {
    if (image) free(image);
    Log_error("Cached image %s of profile %u is unreadable", path.c_str(), profile);
    return false;
  }

  Log_info("Showing cached image %s of profile %u", path.c_str(), profile);
  display_show_image(image, size, true);
  free(image);
  DisplayedImage::remember(path.c_str());
  preferences.putString(PREFERENCES_CURRENT_PATH_KEY, path);
  return true;
}

static void prefetchProfile(const ApiDisplayInputs &activeInputs, uint8_t profile) {
  ApiDisplayInputs inputs = activeInputs;
  inputs.apiKey = deviceProfiles.apiKey(profile);
  if (inputs.apiKey.isEmpty()) {
    Log_info("Profile %u is not registered yet, nothing to prefetch", profile);
    return;
  }
  inputs.friendlyId = deviceProfiles.friendlyId(profile);
  inputs.macAddress = DeviceProfiles::macForProfile(WiFi.macAddress(), profile);

  ApiDisplayResult result = fetchApiDisplay(inputs);
  if (result.error != HTTPS_NO_ERR || result.response.status != 0 || result.response.image_url.isEmpty()) {
    Log_info("Prefetch of profile %u skipped (%s, status %d)", profile, https_request_err_str(result.error),
             (int)result.response.status);
    return;
  }

  char path[36];
  filesystem_fix_filename(result.response.filename.c_str(), path);
  if (!filesystem_file_exists(path)) {
    HttpRetryRequestConfig config;
    config.url = result.response.image_url;
    config.apiBaseUrl = inputs.baseUrl; // ID / Access-Token only go to the API server
    config.headers = buildImageHeaders(inputs);
    config.readTimeoutSeconds = result.response.image_url_timeout;

    HttpRetryRequest request(config);
    https_request_err_e fetched = request.execute();
    if (fetched != HTTPS_NO_ERR) {
      Log_info("Prefetch download for profile %u failed: %s", profile, https_request_err_str(fetched));
      return;
    }
    // Only cache what the cached-switch path can draw: PNG or JPEG
    bool isJPEG = request.contentType() == "image/jpeg";
    if (!isJPEG && parsePNGHeader(request.body(), request.bodySize()) != PNG_NO_ERR) {
      Log_info("Prefetched image for profile %u is not a PNG/JPEG, not caching it", profile);
      return;
    }
    filesystem_purge_old_file(path); // keep only the newest version of each screen, like the active download
    writeImageToFile(path, request.body(), request.bodySize());
  }

  deviceProfiles.setCachedImage(profile, String(path));
  Log_info("Prefetched %s for profile %u", path, profile);
}

void profileCachePrefetch(const ApiDisplayInputs &activeInputs) {
  if (WiFi.status() != WL_CONNECTED) {
    Log_info("No Wi-Fi, skipping profile prefetch");
    return;
  }
  uint8_t active = deviceProfiles.active();
  for (uint8_t profile = 0; profile < deviceProfiles.count(); profile++) {
    if (profile != active) prefetchProfile(activeInputs, profile);
  }
}

#endif // DEVICE_PROFILE_COUNT
