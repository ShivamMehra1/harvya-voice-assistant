# Harvya AI — ESP32-S3 Smart Embedded Voice Assistant

> **An Edge-AI Powered Voice Assistant with On-Device Wake Word Detection, Duplex Audio Streaming, Real-Time Emotion UI, and Model Context Protocol (MCP) Hardware Control.**
> 
> *Created for ECE 327 Embedded Systems Project*  
> *Author: Shivam Mehra*

---

## 📑 Table of Contents

1. [Executive Summary](#-executive-summary)
2. [Key Highlights & Capabilities](#-key-highlights--capabilities)
3. [System Architecture & Working Principle](#-system-architecture--working-principle)
   - [End-to-End Pipeline (Microphone to Speaker)](#end-to-end-pipeline)
   - [Audio Processing Subsystem & Threading Model](#audio-processing-subsystem--threading-model)
   - [Device State Machine](#device-state-machine)
4. [Hardware Specifications & Pinout Configuration](#-hardware-specifications--pinout-configuration)
   - [Bill of Materials (BOM)](#bill-of-materials-bom)
   - [Detailed Wiring & Pin Mapping](#detailed-wiring--pin-mapping)
5. [Software Architecture & Technology Stack](#-software-architecture--technology-stack)
   - [Core Technologies & Frameworks](#core-technologies--frameworks)
   - [Managed Components & Libraries](#managed-components--libraries)
6. [Model Context Protocol (MCP) & Hardware Control](#-model-context-protocol-mcp--hardware-control)
7. [Display, UI & Emotion System (LVGL 9)](#-display-ui--emotion-system-lvgl-9)
8. [Dual-Language Localization (English & Hindi)](#-dual-language-localization-english--hindi)
9. [Networking, Protocols & Cloud Integration](#-networking-protocols--cloud-integration)
10. [Flash Partition Layout & OTA Updates](#-flash-partition-layout--ota-updates)
11. [Project Directory Structure](#-project-directory-structure)
12. [Step-by-Step Setup & Build Instructions](#-step-by-step-setup--build-instructions)
13. [Quick Team Presentation Cheat-Sheet](#-quick-team-presentation-cheat-sheet)
14. [License & Acknowledgments](#-license--acknowledgments)

---

## 🌟 Executive Summary

**Harvya AI** is an advanced embedded smart voice assistant engineered on the **Espressif ESP32-S3** microcontroller. It bridges low-power microcontroller hardware with modern cloud-scale generative AI (Large Language Models like Qwen, DeepSeek, and OpenAI).

Unlike simple microcontroller projects that play pre-recorded sounds or send raw uncompressed audio, Harvya AI runs a complete digital signal processing (DSP) pipeline directly on the chip:
- **Acoustic Echo Cancellation (AEC)**, **Noise Suppression (NS)**, and **Voice Activity Detection (VAD)** via Espressif Audio Front-End (`ESP-AFE`).
- **Offline On-Device Wake Word Detection** ("*Hi Harvya*") powered by Espressif's neural network engine (`ESP-SR / Wakenet`).
- **Real-Time Duplex Audio Streaming** using the low-latency **Opus codec** over WebSocket / MQTT.
- **Dynamic LCD UI & Emotion Rendering** powered by **LVGL 9** on a 1.8" ST7735 SPI display.
- **Model Context Protocol (MCP)** execution right on the ESP32-S3, enabling AI models to directly inspect device state (battery, Wi-Fi RSSI, volume) and manipulate physical peripherals (relays, lamps, speakers, LEDs).

---

## 🚀 Key Highlights & Capabilities

| Capability | Technical Detail |
|---|---|
| **Processing Core** | ESP32-S3 Dual-Core Xtensa LX7 @ 240 MHz with Vector Instructions |
| **Memory** | 16 MB Octal-SPI Flash + 8 MB Octal-SPI PSRAM |
| **Audio Input** | INMP441 MEMS Omnidirectional Microphone (I2S Simplex, 16 kHz, 16-bit PCM) |
| **Audio Output** | MAX98357 Class-D I2S Digital Amplifier (I2S Simplex, 24 kHz, 16-bit PCM) |
| **Acoustic Processing** | Hardware-accelerated AEC, BSS/NS, AGC, and VAD using ESP-DSP & ESP-SR |
| **Wake Word Engine** | On-device offline wake word detection ("*Hi Harvya*") without sending audio to the cloud |
| **Audio Compression** | Real-time Opus encoding (60ms frame duration, 16 kHz uplink, 24 kHz downlink) |
| **Display & Graphics** | 1.8" ST7735 128×160 RGB SPI TFT LCD driven by LVGL 9 with hardware-accelerated DMA |
| **Visual Feedback** | Real-time AI Emotion Avatars (21+ Twemoji expressions), streaming subtitles & status indicators |
| **AI Tool Execution** | Embedded MCP (Model Context Protocol) Server for bi-directional LLM function calling |
| **Networking** | 2.4 GHz Wi-Fi (802.11 b/g/n) with Hotspot Captive Portal & BLE Blufi provisioning |
| **Languages** | Native English (`en-US`) and Hindi (`hi-IN`) with localized OGG voice prompts |
| **Firmware Integrity** | Dual-OTA (A/B Partition scheme) with automatic rollback and memory-mapped assets |

---

## 🧠 System Architecture & Working Principle

### End-to-End Pipeline

The overall architecture operates on a low-latency loop between the edge device and the AI cloud backend:

```
+-------------------------------------------------------------------------------------------------------+
|                                           HARVYA AI HARDWARE                                          |
|                                                                                                       |
|  [ INMP441 Mic ]                  [ ESP32-S3 Dual-Core LX7 + 8MB PSRAM ]            [ MAX98357 Amp ]  |
|         |                                      |                                           ^          |
|    16kHz I2S                              FreeRTOS Tasks                               24kHz I2S      |
|         v                                      |                                           |          |
|  +---------------+      +---------------+      v       +---------------+            +---------------+ |
|  | I2S In Driver | ---> | AFE Processor | ---> | --->  | Opus Encoder  |            | Opus Decoder  | |
|  +---------------+      | (AEC/NS/VAD)  |      |       +---------------+            +---------------+ |
|                         +---------------+      |               |                            ^         |
|                                 |              |          Opus Packets                 Opus Packets   |
|                         [ Wake Word Net ]      |          (60ms frames)                (60ms frames)  |
|                           "Hi Harvya"          |               |                            |         |
+------------------------------------------------|---------------|----------------------------|---------+
                                                 |               v                            |
                                      Wi-Fi 802.11 b/g/n  [ WebSocket / MQTT ]                |
                                                 |               |                            |
                                                 v               v                            |
+---------------------------------------------------------------------------------------------|---------+
|                                         AI BACKEND SERVER                                   |         |
|                                                                                             |         |
|            +-----------------------+     +------------------------+     +-----------------------+     |
|            | Speech-to-Text (STT)  | --> | LLM Reasoning & MCP    | --> | Neural TTS Engine     |-----+ |
|            | (Whisper / Fast ASR)  |     | (Qwen / DeepSeek / AI) |     | (EdgeTTS / Neerja)    |       |
|            +-----------------------+     +------------------------+     +-----------------------+       |
|                                                      |                                                |
|                                              [ MCP Tool Calls ]                                       |
|                                                      |                                                |
|                                           (JSON RPC back to ESP32)                                    |
+-------------------------------------------------------------------------------------------------------+
```

### Audio Processing Subsystem & Threading Model

Harvya AI separates audio ingestion, speech processing, compression, and playback into independent FreeRTOS tasks to guarantee hard real-time latency without dropped audio frames:

```
                     +---------------------------------------+
                     |         INMP441 Microphone            |
                     +---------------------------------------+
                                         |  (I2S Simplex, 16 kHz)
                                         v
                     +---------------------------------------+
                     |            AudioInputTask             |
                     |  • Reads raw PCM samples              |
                     |  • Feeds AFE / ESP-SR Engine          |
                     +---------------------------------------+
                                         |
                       +-----------------+-----------------+
                       |                                   |
         [ Idle / Waiting for Wake ]              [ Active Conversation ]
                       |                                   |
                       v                                   v
             +--------------------+               +--------------------+
             | Wakenet (ESP-SR)   |               | AfeAudioProcessor  |
             | Detects "Hi Harvya"|               | AEC + NS + VAD     |
             +--------------------+               +--------------------+
                       |                                   |
         Triggers Device State Machine                     | Clean 16 kHz PCM
                       |                                   v
                       |                         +--------------------+
                       |                         | audio_encode_queue |
                       |                         +--------------------+
                       |                                   |
                       +-----------------------------------+
                                         |
                                         v
                     +---------------------------------------+
                     |             OpusCodecTask             |
                     |  • Encodes PCM -> 60ms Opus frames    |
                     |  • Pushes into `audio_send_queue_`    |
                     +---------------------------------------+
                                         |
                                         v
                     +---------------------------------------+
                     |           WebSocket Protocol          |
                     |  • Sends binary frames to cloud       |
                     +---------------------------------------+
                                         |
                                         v
                                [ Cloud AI Server ]
                                         |
                                         v (24 kHz Opus Audio Packets)
                     +---------------------------------------+
                     |           WebSocket Protocol          |
                     |  • Pushes to `audio_decode_queue_`    |
                     +---------------------------------------+
                                         |
                                         v
                     +---------------------------------------+
                     |             OpusCodecTask             |
                     |  • Decodes Opus -> 24 kHz PCM         |
                     |  • Pushes to `audio_playback_queue_`  |
                     +---------------------------------------+
                                         |
                                         v
                     +---------------------------------------+
                     |            AudioOutputTask            |
                     |  • Feeds I2S Output Buffer            |
                     +---------------------------------------+
                                         |  (I2S Simplex, 24 kHz)
                                         v
                     +---------------------------------------+
                     |         MAX98357 Class-D Amp          |
                     +---------------------------------------+
```

### Device State Machine

Harvya AI is governed by a thread-safe deterministic state machine (`DeviceStateMachine`) enforcing valid state transitions:

```
               +-----------------------+
               | kDeviceStateStarting  |
               +-----------------------+
                           |
             +-------------+-------------+
             |                           |
             v                           v
+-----------------------------+  +-------------------+
| kDeviceStateWifiConfiguring |  | kDeviceStateIdle  | <--------------------+
+-----------------------------+  +-------------------+                      |
                                         |                                  |
                           (Wake Word / Touch Button)                       |
                                         v                                  |
                                 +-----------------------+                  |
                                 | kDeviceStateListening |                  |
                                 +-----------------------+                  |
                                         |                                  |
                                   (User Speaks)                            |
                                         v                                  |
                                 +-----------------------+                  |
                                 | kDeviceStateSpeaking  | -----------------+
                                 | (Playing TTS Audio)   |   (TTS Finished)
                                 +-----------------------+
                                         |
                                (Error Occurred)
                                         v
                                 +-----------------------+
                                 |  kDeviceStateUnknown  |
                                 |  kDeviceStateFatal    |
                                 +-----------------------+
```

---

## 🔌 Hardware Specifications & Pinout Configuration

### Bill of Materials (BOM)

| Component | Specification | Purpose |
|---|---|---|
| **MCU Board** | ESP32-S3-WROOM-1 / DevKitC-1 (16MB Flash, 8MB PSRAM) | Main application processor |
| **Microphone** | INMP441 MEMS Omnidirectional Sensor | Digital audio input (I2S) |
| **Amplifier** | MAX98357A I2S Mono 3.2W Class-D Amp | Digital audio output (I2S) |
| **Speaker** | 4Ω or 8Ω 3W Speaker | Acoustic output |
| **Display** | 1.8" ST7735 TFT SPI LCD (128×160 RGB) | Visual interface & emotions |
| **Input Buttons** | Push buttons / Capacitive touch pads | Interaction (Boot, Touch, Vol+, Vol-) |
| **Relay / Lamp** | 5V Optocoupler Relay Module | Smart home appliance switching via MCP |
| **Power Supply** | 5V / 2A Micro-USB or Type-C adapter | System power |

---

### Detailed Wiring & Pin Mapping

> **Important**: The ESP32-S3 utilizes two independent I2S peripheral channels in **Simplex Mode** (one for microphone capture at 16 kHz and one for speaker audio at 24 kHz) to avoid clock-synchronization jitter.

#### 1. INMP441 MEMS Microphone (I2S Input Bus)
| INMP441 Pin | ESP32-S3 Pin | Function | Description |
|---|---|---|---|
| **WS** | **GPIO 4** | `AUDIO_I2S_MIC_GPIO_WS` | Word Select (Left/Right Clock) |
| **SCK** | **GPIO 5** | `AUDIO_I2S_MIC_GPIO_SCK` | Serial Bit Clock |
| **SD** | **GPIO 6** | `AUDIO_I2S_MIC_GPIO_DIN` | Serial Data Out (Mic to MCU) |
| **L/R** | **GND** | Channel Select | Tied to GND for Left channel |
| **VDD** | **3.3V** | Power | Regulated 3.3V Power |
| **GND** | **GND** | Ground | Common Ground |

#### 2. MAX98357A I2S Class-D Amplifier (I2S Output Bus)
| MAX98357 Pin | ESP32-S3 Pin | Function | Description |
|---|---|---|---|
| **DIN** | **GPIO 7** | `AUDIO_I2S_SPK_GPIO_DOUT` | Serial Data In (MCU to Speaker) |
| **BCLK** | **GPIO 15** | `AUDIO_I2S_SPK_GPIO_BCLK` | Bit Clock |
| **LRC** | **GPIO 16** | `AUDIO_I2S_SPK_GPIO_LRCK` | Left/Right Word Select Clock |
| **GAIN** | **GND** | Gain Mode | 12 dB Gain (leave open for 9 dB) |
| **SD** | *Not Connected* | Shutdown | Internal pull-up keeps amp active |
| **VIN** | **5V / 3.3V** | Power | 5V recommended for maximum audio volume |
| **GND** | **GND** | Ground | Common Ground |

#### 3. ST7735 1.8" 128×160 SPI TFT LCD Display
| ST7735 Pin | ESP32-S3 Pin | Function | Description |
|---|---|---|---|
| **MOSI (SDA)** | **GPIO 11** | `DISPLAY_SPI_MOSI` | SPI Master-Out-Slave-In |
| **SCLK (SCL)** | **GPIO 12** | `DISPLAY_SPI_CLK` | SPI Master Clock (40 MHz) |
| **CS** | **GPIO 10** | `DISPLAY_SPI_CS` | SPI Chip Select |
| **DC (A0)** | **GPIO 13** | `DISPLAY_DC_PIN` | Data / Command register select |
| **RESET** | **GPIO 14** | `DISPLAY_RST_PIN` | Hardware Display Reset |
| **BLK (LED)** | **GPIO 21** | `DISPLAY_BL_PIN` | Backlight PWM control (0–100%) |
| **VCC** | **3.3V / 5V** | Power | Display logic power |
| **GND** | **GND** | Ground | Common Ground |

#### 4. Buttons, Status LEDs & Actuators
| Control | ESP32-S3 Pin | Function | Interaction Details |
|---|---|---|---|
| **Boot Button** | **GPIO 0** | System / Chat Toggle | Short press: Toggle listening / Long press: Wi-Fi setup |
| **Touch Button** | **GPIO 47** | Push-To-Talk | Press-down to record; release to send |
| **Volume Up** | **GPIO 40** | Volume Control | Short press: +10% / Long press: 100% max volume |
| **Volume Down** | **GPIO 39** | Volume Control | Short press: -10% / Long press: Mute (0%) |
| **Built-in LED** | **GPIO 48** | Status Indication | Blinks during provisioning, solid during standby |
| **Lamp / Relay** | **GPIO 18** | MCP Smart Home Output | Toggled by AI model using `self.lamp.turn_on/off` |

---

## 💻 Software Architecture & Technology Stack

### Core Technologies & Frameworks

- **ESP-IDF v5.5.2+**: Espressif IoT Development Framework providing FreeRTOS, LwIP networking, mbedTLS, and low-level peripheral drivers.
- **Modern C++17**: Modular, object-oriented design utilizing smart pointers, RAII, lambda callbacks, and design patterns (Singleton, Strategy, Observer).
- **FreeRTOS**: Multitasking kernel managing task priorities, semaphores, mutexes, event groups, and message queues across CPU Core 0 and Core 1.
- **LVGL v9.5.0**: Light and Versatile Graphics Library delivering smooth animations, font rendering, status bars, and emoji graphics.
- **ESP-SR v2.3.0**: Espressif Speech Recognition engine containing:
  - **Wakenet9**: Deep learning model for wake word detection.
  - **ESP-AFE**: Acoustic Echo Cancellation, Noise Suppression, and Voice Activity Detection.
- **Opus Codec**: Highly optimized lossy audio compression standard running in 60ms frames for duplex VoIP transmission.
- **cJSON**: Lightweight JSON serialization and deserialization for WebSocket and MCP protocol payloads.

### Managed Components & Libraries

The system integrates 18 modular IDF components defined in `main/idf_component.yml`:

| Component | Source | Responsibility |
|---|---|---|
| `78/esp-wifi-connect` | Component Registry | Captive portal & Wi-Fi station connection manager |
| `78/harvya-ai-fonts` | Component Registry | Custom embedded font tables and emoji glyphs for ST7735 |
| `espressif/esp_lvgl_port` | Espressif | Thread-safe FreeRTOS port layer for LVGL |
| `lvgl/lvgl` | LVGL Community | Core UI widgets, canvas rendering, and layout management |
| `espressif/esp_audio_codec` | Espressif | Hardware audio codec HAL and format conversions |
| `espressif/esp_audio_effects` | Espressif | Real-time digital signal processing (AEC, AGC, NS) |
| `espressif/esp_codec_dev` | Espressif | I2S codec driver abstraction for simplex/duplex buses |
| `espressif/esp-sr` | Espressif | Wakenet neural network wake word engine |
| `espressif/esp-dsp` | Espressif | Assembly-optimized DSP algorithms for Xtensa LX7 |
| `espressif/esp_mmap_assets` | Espressif | Memory-mapped access to SPI flash asset partition |
| `espressif/button` | Espressif | Software debouncing, click, double-click, and long-press handler |
| `espressif/led_strip` | Espressif | RMT-based addressable LED driver (WS2812 status) |
| `espressif/adc_battery_estimation` | Espressif | ADC curve calibration for battery percentage calculation |

---

## 🛠️ Model Context Protocol (MCP) & Hardware Control

Harvya AI implements Anthropic's **Model Context Protocol (MCP)** specification directly inside the microcontroller firmware (`main/mcp_server.h` and `main/mcp_server.cc`).

When the user asks:
> *"Harvya, turn on the study lamp and check your battery level."*

The LLM issues an MCP Tool Call over the WebSocket connection. The on-device MCP Server parses the request, executes the corresponding C++ callback, and replies with the result:

### Built-in MCP Tools Available to the AI:

1. **`self.get_device_status`**
   - Returns real-time JSON metadata: current speaker volume, screen brightness, battery percentage, charging state, and Wi-Fi RSSI.
2. **`self.audio_speaker.set_volume`**
   - Sets volume between `0` and `100`% via `AudioCodec::SetOutputVolume()`.
3. **`self.screen.set_brightness`**
   - Regulates LCD backlight brightness via PWM duty cycle.
4. **`self.screen.set_theme`**
   - Dynamically toggles display theme between `light` and `dark`.
5. **`self.lamp.turn_on` / `self.lamp.turn_off`**
   - Drives GPIO 18 high or low to switch external 110V/220V relays or LED desk lamps.
6. **`self.lamp.get_state`**
   - Inspects the current state of GPIO 18.
7. **`self.get_system_info`**
   - Returns chip model, CPU clock, free heap, minimum free heap, and MAC address.
8. **`self.reboot`**
   - Safely flushes storage and restarts the microcontroller.
9. **`self.upgrade_firmware`**
   - Triggers Over-The-Air (OTA) flashing from a remote binary URL.

---

## 🎨 Display, UI & Emotion System (LVGL 9)

The visual interface is rendered on the **ST7735 1.8" SPI TFT LCD** (128×160 pixels, 16-bit 565 BGR color format) using **LVGL v9**:

```
+-----------------------------------+
|  [WiFi] [Vol:80%]      [Battery]  |  <-- Top Status Bar
+-----------------------------------+
|                                   |
|               (^_^)               |  <-- 32x32 Twemoji Expression
|              [HAPPY]              |      (Syncs with LLM Sentiment)
|                                   |
+-----------------------------------+
| Assistant:                        |  <-- Streaming Subtitle Box
| "The room temperature is 24°C"    |      (Auto-scrolls / multiline)
+-----------------------------------+
```

### Emotion Expressions:
When the LLM streams its response, it sends an emotional indicator payload:
```json
{
  "type": "llm",
  "emotion": "thinking"
}
```
Harvya AI maps this to one of **21 built-in Twemoji expressions** rendered on screen:
- `neutral`, `happy`, `laughing`, `funny`, `sad`, `angry`, `crying`, `loving`
- `embarrassed`, `surprised`, `shocked`, `thinking`, `winking`, `cool`, `relaxed`
- `delicious`, `kissy`, `confident`, `sleepy`, `silly`, `confused`

---

## 🌐 Dual-Language Localization (English & Hindi)

Harvya AI supports full bilingual capability:

| Asset / UI String | English (`en-US`) | Hindi (`hi-IN`) |
|---|---|---|
| **Welcome Voice Prompt** | *"Hello, my friend!"* | *"नमस्ते, मैं आपकी क्या सहायता कर सकता हूँ?"* |
| **Wi-Fi Configuration** | *"Entering Wi-Fi configuration mode..."* | *"वाई-फ़ाई कॉन्फ़िगरेशन मोड चालू हो रहा है..."* |
| **Listening Indicator** | *"Listening..."* | *"सुन रहा हूँ..."* |
| **Speaking Indicator** | *"Speaking..."* | *"बोल रहा हूँ..."* |
| **Neural TTS Voice** | `en-IN-NeerjaNeural` | `hi-IN-SwaraNeural` |
| **Embedded Number Audio** | Numbers 0–9 recorded in OGG | Numbers 0–9 recorded in Hindi OGG |

Language selection is configured at compile time via `menuconfig` (`CONFIG_LANGUAGE_EN_US` or `CONFIG_LANGUAGE_HI_IN`) with automatic fallback to English for any missing locale keys.

---

## 📡 Networking, Protocols & Cloud Integration

### Communication Protocols

Harvya AI supports two communication backends:
1. **WebSocket Protocol (Default & Recommended)**:
   - Full-duplex single connection carrying both binary audio frames and JSON control messages.
   - **Binary Protocol v2 / v3 Header**: Includes packet version, timestamp for server-side AEC alignment, and payload size, followed by raw Opus audio bytes.
2. **MQTT + UDP Protocol**:
   - MQTT for control messages, state notifications, and MCP remote execution.
   - Low-latency UDP packets for raw audio transmission.

### Incoming JSON Frame Types Handled:
- **`stt`**: Displays user speech transcription in real time.
- **`llm`**: Supplies assistant emotional state.
- **`tts`**:
  - `start`: Switches state machine to `kDeviceStateSpeaking`.
  - `sentence_start`: Updates LCD subtitle area with the current sentence.
  - `stop`: Reverts state machine to `kDeviceStateListening` or `kDeviceStateIdle`.
- **`mcp`**: Carries Model Context Protocol tool requests to execute on device hardware.
- **`alert`**: Triggers visual warnings and acoustic chime alerts.

---

## 💾 Flash Partition Layout & OTA Updates

Harvya AI utilizes a custom 16 MB flash partition table (`partitions/v2/16m.csv`):

```
+--------------------+------------+------------+----------------------------------------+
| Partition Name     | Type       | Size       | Purpose                                |
+--------------------+------------+------------+----------------------------------------+
| nvs                | data (nvs) | 16 KB      | Wi-Fi credentials, volume & settings   |
| otadata            | data (ota) | 8 KB       | Active OTA boot slot pointer           |
| phy_init           | data (phy) | 4 KB       | RF PHY calibration data                |
| ota_0              | app (ota)  | 4,032 KB   | Primary application firmware           |
| ota_1              | app (ota)  | 4,032 KB   | Secondary firmware (Rollback & Update) |
| assets             | data (spif)| 8,192 KB   | Memory-mapped fonts, models & sounds   |
+--------------------+------------+------------+----------------------------------------+
```

### Failsafe OTA Features:
- **A/B Dual Partitioning**: The device downloads new firmware into the inactive OTA partition. Only after verifying the cryptographic checksum and image header is the boot pointer switched.
- **Automatic Rollback**: If a newly flashed firmware fails during its first startup or crashes the watchdog, the ESP32-S3 bootloader automatically falls back to the previous stable firmware slot.
- **Assets Partition**: Fonts, neural wake word models, and system audio files are stored in an 8MB partition accessed via SPI flash memory mapping (`esp_partition_mmap`), saving runtime RAM.

---

## 📂 Project Directory Structure

```
harvya-ai/
├── CMakeLists.txt                  # Root build configuration (project: harvya_ai)
├── sdkconfig                       # Current active configuration
├── sdkconfig.defaults              # Default baseline configurations
├── sdkconfig.defaults.esp32s3      # S3-specific flags (Octal PSRAM @ 80MHz, 240MHz CPU)
├── LICENSE                         # MIT License
├── README.md                       # Comprehensive project documentation
│
├── main/                           # Primary application source code
│   ├── CMakeLists.txt              # Component build script & binary asset embedding
│   ├── Kconfig.projbuild           # Harvya AI menuconfig settings
│   ├── idf_component.yml           # IDF Component Manager dependencies (18 packages)
│   ├── main.cc                     # Entry point: app_main() initializes NVS & Application
│   ├── application.h / .cc         # Application orchestrator, event loop & queue manager
│   ├── device_state.h              # State enum definitions
│   ├── device_state_machine.h/.cc  # Deterministic device state machine
│   ├── mcp_server.h / .cc          # Model Context Protocol server & tool registry
│   ├── system_info.h / .cc         # Hardware diagnostics (MAC, heap, flash size)
│   ├── settings.h / .cc            # NVS persistent configuration wrapper
│   ├── ota.h / .cc                 # Over-The-Air upgrade & firmware activation client
│   │
│   ├── audio/                      # Audio subsystem
│   │   ├── audio_codec.h / .cc     # Abstract hardware codec interface
│   │   ├── audio_service.h / .cc   # Real-time multi-threaded audio pipeline
│   │   ├── codecs/                 # Hardware drivers (NoAudioCodec simplex/duplex)
│   │   ├── demuxer/                # OGG audio container parser
│   │   ├── processors/             # AEC, Noise Suppression & Audio Debugger
│   │   └── wake_words/             # Offline wake word neural model wrappers
│   │
│   ├── boards/                     # Board abstraction layer
│   │   ├── common/                 # Shared board components (button, backlight, blufi)
│   │   └── bread-compact-wifi-lcd/ # Harvya AI breadboard configuration & pin definitions
│   │       ├── config.h            # Pin assignments for INMP441, MAX98357, ST7735
│   │       └── harvya_wifi_lcd_board.cc # Board initialization & peripheral setup
│   │
│   ├── display/                    # Display & UI subsystem
│   │   ├── display.h / .cc         # Abstract display base class
│   │   ├── lcd_display.h / .cc     # ST7735 driver integration with LVGL
│   │   └── lvgl_display/           # LVGL themes, emoji collections & fonts
│   │
│   ├── protocols/                  # Network communication protocols
│   │   ├── protocol.h / .cc        # Abstract protocol base class
│   │   ├── websocket_protocol.h/.cc# Full-duplex WebSocket client
│   │   └── mqtt_protocol.h / .cc   # MQTT control + UDP audio protocol
│   │
│   ├── led/                        # Status LED drivers
│   │   ├── single_led.h / .cc      # Standard GPIO indicator LED
│   │   └── gpio_led.h / .cc        # PWM-controlled LED
│   │
│   └── assets/                     # Localization & system audio assets
│       ├── lang_config.h           # Auto-generated string tables & audio headers
│       └── locales/                # Language assets
│           ├── en-US/              # English strings & OGG sound prompts
│           └── hi-IN/              # Hindi strings & OGG sound prompts
│
├── managed_components/             # 18 downloaded ESP-IDF component packages
└── partitions/                     # Flash partition layouts (4M, 8M, 16M, 32M)
    └── v2/16m.csv                  # Active 16MB custom partition table
```

---

## 🔨 Step-by-Step Setup & Build Instructions

### Prerequisites
1. **Operating System**: Windows 10/11, Ubuntu 22.04+, or macOS.
2. **ESP-IDF Toolchain**: **v5.5.2 or newer** installed and exported to path.
   - Test installation: `idf.py --version`
3. **Hardware**: ESP32-S3 board with **at least 8MB Octal PSRAM** (e.g., `ESP32-S3-WROOM-1-N16R8`).

---

### Step 1: Open the Project Environment
Open ESP-IDF Terminal (or standard terminal with environment sourced):
```bash
cd "harvya-ai"
```

### Step 2: Set the Target Architecture
```bash
idf.py set-target esp32s3
```

### Step 3: Configure Project Settings (Optional)
```bash
idf.py menuconfig
```
Navigate to **Harvya AI Assistant**:
- **Default Language**: Choose `English` or `Hindi`.
- **Wake Word**: Verify `Wakenet model with AFE` is active.
- **Default OTA URL**: Set the address of your backend server.
- Press `S` to save and `Q` to exit.

### Step 4: Build the Firmware
```bash
idf.py build
```
*Note: The first build will compile LVGL and the ESP-SR neural models; it may take 3–5 minutes.*

### Step 5: Flash to the ESP32-S3
Connect your ESP32-S3 board via USB. Identify the serial port (`COMx` on Windows or `/dev/ttyUSB0` on Linux):

```bash
# Windows
idf.py -p COM3 flash monitor

# Linux / macOS
idf.py -p /dev/ttyUSB0 flash monitor
```

---

### Step 6: Initial Wi-Fi Provisioning
1. On initial power-up, Harvya AI announces: *"Entering Wi-Fi configuration mode..."*
2. The LCD will display the **Wi-Fi Hotspot details**.
3. Connect your smartphone or computer to the hotspot created by the ESP32 (e.g., `Harvya-Setup`).
4. A captive portal page will automatically open. Select your local 2.4 GHz Wi-Fi network and enter the password.
5. The device connects to Wi-Fi, retrieves the server address, and transitions to **Idle Mode**.
6. Say **"Hi Harvya"** to start interacting!

---

## 🎤 Quick Team Presentation Cheat-Sheet

Use this structured section to explain the project to teammates, professors, or evaluators:

### 1. The Core Problem We Solved
> *"Standard smart speakers (like Alexa or Google Home) rely on closed ecosystems and cannot execute custom local hardware actions easily. Traditional microcontroller projects cannot run modern generative AI due to low compute. **Harvya AI bridges this gap**: an open-source, edge-computing device running DSP and neural wake words locally on an ESP32-S3, while streaming compressed voice to any LLM and exposing hardware pins via the Model Context Protocol."*

### 2. How the Audio Pipeline Works (In 30 Seconds)
1. **Local Capture & Clean**: INMP441 mic captures voice at 16 kHz. The ESP32-S3 runs Acoustic Echo Cancellation (AEC) and Noise Suppression (NS) in hardware.
2. **Offline Wake Word**: The ESP-SR neural network scans for *"Hi Harvya"*. No audio leaves the device until the wake word triggers.
3. **Opus Compression**: Clean speech is compressed into 60ms Opus frames and streamed over low-latency WebSockets to the Python server.
4. **Cloud AI Processing**: The server executes Whisper STT, passes the prompt to DeepSeek / Qwen LLM, and synthesizes 24 kHz speech via Edge neural TTS.
5. **Playback & Animation**: Downlink audio is decoded in real time on the ESP32 and played through the MAX98357 amplifier while the ST7735 screen displays the matching emotional avatar.

### 3. Why the Hardware Choices Matter
- **ESP32-S3 with PSRAM**: Required for running the ESP-SR neural network and buffering audio frames.
- **I2S Simplex Separation**: Using separate I2S clocks for mic (16 kHz) and speaker (24 kHz) prevents audio distortion and sampling drift.
- **Model Context Protocol (MCP)**: Transforms the assistant into an IoT hub that can inspect device telemetry and toggle AC/DC appliances via relays.

---

## 📜 License & Acknowledgments

- **License**: Released under the [MIT License](LICENSE).
- **Author**: Shivam Mehra
- **Special Thanks**: Espressif Systems for ESP-IDF, ESP-SR, and LVGL Open Source Community.
