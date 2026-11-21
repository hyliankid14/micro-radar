#pragma once

#include <LovyanGFX.hpp>

#ifdef CROWPANEL_7
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#ifdef CROWPANEL_BUFFERED
#include "CrowPanelRGB.h"
#else
#include <hal/lcd_ll.h>
#endif

class LGFX : public lgfx::LGFX_Device
{
#ifdef CROWPANEL_BUFFERED
    CrowPanelRGBPanel panel;
    CrowPanelRGBBus bus;
#else
    lgfx::Panel_RGB panel;
    lgfx::Bus_RGB bus;
#endif

public:
    LGFX()
    {
        auto cfg = bus.config();
        cfg.panel = &panel;
        cfg.pin_d0 = 15; cfg.pin_d1 = 7; cfg.pin_d2 = 6; cfg.pin_d3 = 5; cfg.pin_d4 = 4;
        cfg.pin_d5 = 9; cfg.pin_d6 = 46; cfg.pin_d7 = 3; cfg.pin_d8 = 8; cfg.pin_d9 = 16; cfg.pin_d10 = 1;
        cfg.pin_d11 = 14; cfg.pin_d12 = 21; cfg.pin_d13 = 47; cfg.pin_d14 = 48; cfg.pin_d15 = 45;
        cfg.pin_henable = 41; cfg.pin_vsync = 40; cfg.pin_hsync = 39; cfg.pin_pclk = 0;
        cfg.freq_write = 20000000;
        cfg.hsync_polarity = 0; cfg.hsync_front_porch = 40;
        cfg.hsync_pulse_width = 48; cfg.hsync_back_porch = 40;
        cfg.vsync_polarity = 0; cfg.vsync_front_porch = 1;
        cfg.vsync_pulse_width = 31; cfg.vsync_back_porch = 13;
        cfg.pclk_active_neg = 1;
        cfg.de_idle_high = 0;
        cfg.pclk_idle_high = 0;
        bus.config(cfg);
        auto panelCfg = panel.config();
        panelCfg.memory_width = panelCfg.panel_width = 800;
        panelCfg.memory_height = panelCfg.panel_height = 480;
        panel.config(panelCfg);
        panel.setBus(&bus);
        setPanel(&panel);
    }

#ifndef CROWPANEL_BUFFERED
    bool init()
    {
        if (!lgfx::LGFX_Device::init()) return false;
        // LovyanGFX's RGB bus ignores pclk_active_neg when configuring LCD_CAM.
        lcd_ll_set_pixel_clock_edge(&LCD_CAM, bus.config().pclk_active_neg);
        LCD_CAM.lcd_user.lcd_update = 1;
        return true;
    }
#endif
};
#else
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.freq_write = 40000000;
            cfg.spi_mode = 0;
            cfg.pin_sclk = 38;
            cfg.pin_mosi = 39;
            cfg.pin_miso = -1;
            cfg.pin_dc = 45;
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 21;
            cfg.pin_rst = 40;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 240;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = 46;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        
        setPanel(&_panel_instance);
    }
};
#endif
