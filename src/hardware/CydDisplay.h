#pragma once

#include <SD.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config/pins.h"

namespace hardware {

class CydDisplay : public lgfx::LGFX_Device {
 public:
  CydDisplay() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = config::pins::TftSclk;
      cfg.pin_mosi = config::pins::TftMosi;
      cfg.pin_miso = config::pins::TftMiso;
      cfg.pin_dc = config::pins::TftDc;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }

    {
      auto cfg = panel_.config();
      cfg.pin_cs = config::pins::TftCs;
      cfg.pin_rst = config::pins::TftRst;
      cfg.pin_busy = -1;
      cfg.memory_width = 320;
      cfg.memory_height = 240;
      cfg.panel_width = 320;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }

    {
      auto cfg = light_.config();
      cfg.pin_bl = config::pins::TftBacklight;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_.config(cfg);
      panel_.setLight(&light_);
    }

    {
      auto cfg = touch_.config();
      cfg.spi_host = -1;  // Bitbang avoids conflict with the SD card SPI bus.
      cfg.freq = 2500000;
      cfg.pin_sclk = config::pins::TouchSclk;
      cfg.pin_mosi = config::pins::TouchMosi;
      cfg.pin_miso = config::pins::TouchMiso;
      cfg.pin_cs = config::pins::TouchCs;
      cfg.pin_int = config::pins::TouchIrq;
      cfg.bus_shared = false;
      cfg.x_min = 300;
      cfg.x_max = 3900;
      cfg.y_min = 3700;
      cfg.y_max = 200;
      cfg.offset_rotation = 3;
      touch_.config(cfg);
      panel_.setTouch(&touch_);
    }

    setPanel(&panel_);
  }

 private:
  lgfx::Panel_ILI9342 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
  lgfx::Touch_XPT2046 touch_;
};

}  // namespace hardware
