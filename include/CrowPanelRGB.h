#pragma once

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

class CrowPanelRGBBus : public lgfx::Bus_RGB
{
    esp_lcd_panel_handle_t handle = nullptr;
    void* framebuffer = nullptr;

public:
    bool init() override
    {
        if (handle) return true;
        const auto& bus = config();
        esp_lcd_rgb_panel_config_t cfg = {};
        cfg.clk_src = LCD_CLK_SRC_DEFAULT;
        cfg.timings.pclk_hz = bus.freq_write;
        cfg.timings.h_res = bus.panel->config().panel_width;
        cfg.timings.v_res = bus.panel->config().panel_height;
        cfg.timings.hsync_pulse_width = bus.hsync_pulse_width;
        cfg.timings.hsync_back_porch = bus.hsync_back_porch;
        cfg.timings.hsync_front_porch = bus.hsync_front_porch;
        cfg.timings.vsync_pulse_width = bus.vsync_pulse_width;
        cfg.timings.vsync_back_porch = bus.vsync_back_porch;
        cfg.timings.vsync_front_porch = bus.vsync_front_porch;
        cfg.timings.flags.hsync_idle_low = bus.hsync_polarity;
        cfg.timings.flags.vsync_idle_low = bus.vsync_polarity;
        cfg.timings.flags.de_idle_high = bus.de_idle_high;
        cfg.timings.flags.pclk_active_neg = bus.pclk_active_neg;
        cfg.timings.flags.pclk_idle_high = bus.pclk_idle_high;
        cfg.data_width = 16;
        cfg.bits_per_pixel = 16;
        cfg.num_fbs = 1;
        cfg.bounce_buffer_size_px = cfg.timings.h_res * 10;
        cfg.dma_burst_size = 64;
        cfg.hsync_gpio_num = bus.pin_hsync;
        cfg.vsync_gpio_num = bus.pin_vsync;
        cfg.de_gpio_num = bus.pin_henable;
        cfg.pclk_gpio_num = bus.pin_pclk;
        cfg.disp_gpio_num = -1;
        for (int i = 0; i < 16; ++i) cfg.data_gpio_nums[i] = bus.pin_data[i];
        cfg.flags.fb_in_psram = true;

        esp_err_t error = esp_lcd_new_rgb_panel(&cfg, &handle);
        if (error == ESP_OK) error = esp_lcd_panel_reset(handle);
        if (error == ESP_OK) error = esp_lcd_panel_init(handle);
        if (error == ESP_OK) error = esp_lcd_rgb_panel_get_frame_buffer(handle, 1, &framebuffer);
        if (error != ESP_OK) {
            Serial.printf("[DISPLAY] RGB initialization failed: %s\n", esp_err_to_name(error));
            release();
            return false;
        }
        Serial.printf("[DISPLAY] RGB bounce buffers: %u pixels each\n",
                      static_cast<unsigned>(cfg.bounce_buffer_size_px));
        return true;
    }

    uint8_t* getDMABuffer(uint32_t) override { return static_cast<uint8_t*>(framebuffer); }

    void release() override
    {
        if (handle) {
            esp_err_t error = esp_lcd_panel_del(handle);
            if (error != ESP_OK) Serial.printf("[DISPLAY] RGB release failed: %s\n", esp_err_to_name(error));
        }
        handle = nullptr;
        framebuffer = nullptr;
    }
};

class CrowPanelRGBPanel : public lgfx::Panel_RGB
{
public:
    CrowPanelRGBPanel()
    {
        _write_depth = lgfx::color_depth_t::rgb565_nonswapped;
        _read_depth = lgfx::color_depth_t::rgb565_nonswapped;
    }
};
