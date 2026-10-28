// Harvya AI -- ESP32-S3 Board Implementation
// Hardware: ESP32-S3 + INMP441 (I2S) + MAX98357 (I2S) + ST7735 1.8" SPI LCD
// Created by Shivam Mehra

#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "assets/lang_config.h"
#include "backlight.h"

#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>

#define TAG "HarvyaAI-Board"

// ─────────────────────────────────────────────────────────────────────────────
// Board class: ESP32-S3 + INMP441 + MAX98357 + ST7735
// ─────────────────────────────────────────────────────────────────────────────
class CompactWifiLcdBoard : public WifiBoard {
private:
    esp_lcd_panel_io_handle_t panel_io_   = nullptr;
    esp_lcd_panel_handle_t    panel_      = nullptr;
    Display*                  display_    = nullptr;
    Backlight*                backlight_  = nullptr;

    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    // ── ST7735 SPI LCD Initialization ────────────────────────────────────────
    void InitializeSpiDisplay() {
        ESP_LOGI(TAG, "Initializing ST7735 1.8\" SPI LCD");

        // 1. Register SPI bus
        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num   = DISPLAY_SPI_MOSI;
        bus_cfg.miso_io_num   = -1;
        bus_cfg.sclk_io_num   = DISPLAY_SPI_CLK;
        bus_cfg.quadwp_io_num = -1;
        bus_cfg.quadhd_io_num = -1;
        bus_cfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8;
        ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

        // 2. Attach LCD panel IO (SPI)
        esp_lcd_panel_io_spi_config_t io_cfg = {};
        io_cfg.dc_gpio_num           = DISPLAY_DC_PIN;
        io_cfg.cs_gpio_num           = DISPLAY_SPI_CS;
        io_cfg.pclk_hz               = 40 * 1000 * 1000;  // 40 MHz SPI clock
        io_cfg.lcd_cmd_bits          = 8;
        io_cfg.lcd_param_bits        = 8;
        io_cfg.spi_mode              = 0;
        io_cfg.trans_queue_depth     = 10;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISPLAY_SPI_HOST, &io_cfg, &panel_io_));

        // 3. Init ST7735 panel (uses ST7789-compatible driver with ST7735 init seq)
        esp_lcd_panel_dev_config_t panel_cfg = {};
        panel_cfg.reset_gpio_num   = DISPLAY_RST_PIN;
        panel_cfg.rgb_ele_order    = LCD_RGB_ELEMENT_ORDER_BGR;
        panel_cfg.bits_per_pixel   = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_cfg, &panel_));

        // 4. Reset and initialize
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, DISPLAY_INVERT_COLOR));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        // 5. Init backlight
        backlight_ = new Backlight(DISPLAY_BL_PIN);
        backlight_->SetBrightness(100);

        // 6. Create LVGL display wrapper
        display_ = new LcdDisplay(panel_io_, panel_,
                                  DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                  DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                  DISPLAY_SWAP_XY);

        ESP_LOGI(TAG, "ST7735 LCD initialized (%dx%d)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    // ── Button Handlers ───────────────────────────────────────────────────────
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });

        touch_button_.OnPressDown([this]() {
            Application::GetInstance().StartListening();
        });
        touch_button_.OnPressUp([this]() {
            Application::GetInstance().StopListening();
        });

        volume_up_button_.OnClick([this]() {
            auto codec  = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) volume = 100;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME);
        });

        volume_down_button_.OnClick([this]() {
            auto codec  = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) volume = 0;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification(Lang::Strings::MUTED);
        });
    }

    // ── MCP Tools (GPIO lamp/relay) ───────────────────────────────────────────
    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);
    }

public:
    CompactWifiLcdBoard() :
        boot_button_(BOOT_BUTTON_GPIO),
        touch_button_(TOUCH_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO) {
        InitializeSpiDisplay();
        InitializeButtons();
        InitializeTools();
    }

    // ── LED ───────────────────────────────────────────────────────────────────
    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    // ── Audio Codec: INMP441 (mic) + MAX98357 (speaker) via I2S simplex ───────
    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,  AUDIO_I2S_MIC_GPIO_WS,   AUDIO_I2S_MIC_GPIO_DIN
        );
#else
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN
        );
#endif
        return &audio_codec;
    }

    // ── Display ───────────────────────────────────────────────────────────────
    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(CompactWifiLcdBoard);
