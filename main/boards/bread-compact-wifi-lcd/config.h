// Harvya AI -- Board Configuration
// Hardware: ESP32-S3 + INMP441 (I2S Mic) + MAX98357 (I2S Amp) + ST7735 1.8" SPI LCD
// Created by Shivam Mehra

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ─── Audio Sample Rates ───────────────────────────────────────────────────────
#define AUDIO_INPUT_SAMPLE_RATE  16000   // INMP441 mic capture rate
#define AUDIO_OUTPUT_SAMPLE_RATE 24000   // MAX98357 TTS playback rate

// ─── I2S Simplex Mode (separate mic and speaker I2S buses) ───────────────────
// INMP441 and MAX98357 use separate I2S peripherals (simplex)
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

// INMP441 MEMS Microphone (I2S input)
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4   // Word Select (L/R clock)
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5   // Bit Clock
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6   // Data from INMP441

// MAX98357 I2S Amplifier (I2S output)
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7   // Data to MAX98357
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15  // Bit Clock
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16  // Left/Right Clock

#else

// Duplex I2S fallback (not used for INMP441 + MAX98357)
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7

#endif // AUDIO_I2S_METHOD_SIMPLEX

// ─── Board Buttons & LEDs ─────────────────────────────────────────────────────
#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_47
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

// ─── ST7735 1.8" 128x160 SPI LCD ─────────────────────────────────────────────
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_SPI_MOSI        GPIO_NUM_11
#define DISPLAY_SPI_CLK         GPIO_NUM_12
#define DISPLAY_SPI_CS          GPIO_NUM_10
#define DISPLAY_DC_PIN          GPIO_NUM_13
#define DISPLAY_RST_PIN         GPIO_NUM_14
#define DISPLAY_BL_PIN          GPIO_NUM_21   // Backlight (PWM or GPIO high)
#define DISPLAY_WIDTH           128
#define DISPLAY_HEIGHT          160
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         false
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0
#define DISPLAY_INVERT_COLOR    true          // ST7735 typically needs invert

// ─── MCP Lamp/Relay (optional GPIO tool) ─────────────────────────────────────
#define LAMP_GPIO               GPIO_NUM_18

#endif // _BOARD_CONFIG_H_
