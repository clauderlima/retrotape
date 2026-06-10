#pragma once

#include <Arduino.h>

namespace config {
namespace wifi {

static constexpr char Hostname[] = "retrotape";

static constexpr bool EnableFallbackAccessPoint = true;
static constexpr char FallbackApSsid[] = "RetroTape";
static constexpr char FallbackApPassword[] = "12345678";

static constexpr uint32_t ConnectTimeoutMs = 15000;

}  // namespace wifi
}  // namespace config
