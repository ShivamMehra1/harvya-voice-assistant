# Harvya AI -- Main CMakeLists.txt
# Hardware: ESP32-S3 + INMP441 (I2S MEMS Mic) + MAX98357 (I2S Amp) + ST7735 1.8" 128x160 SPI LCD
# Created by Shivam Mehra

# Define core source files
set(SOURCES "audio/audio_codec.cc"
            "audio/audio_service.cc"
            "audio/demuxer/ogg_demuxer.cc"
            "audio/codecs/no_audio_codec.cc"
            "audio/codecs/dummy_audio_codec.cc"
            "audio/processors/audio_debugger.cc"
            "led/single_led.cc"
            "led/circular_strip.cc"
            "led/gpio_led.cc"
            "display/display.cc"
            "display/lcd_display.cc"
            "display/lvgl_display/lvgl_display.cc"
            "display/emote_display.cc"
            "display/lvgl_display/emoji_collection.cc"
            "display/lvgl_display/lvgl_theme.cc"
            "display/lvgl_display/lvgl_font.cc"
            "display/lvgl_display/lvgl_image.cc"
            "display/lvgl_display/gif/lvgl_gif.cc"
            "display/lvgl_display/gif/gifdec.c"
            "display/lvgl_display/jpg/image_to_jpeg.cpp"
            "display/lvgl_display/jpg/jpeg_to_image.c"
            "protocols/protocol.cc"
            "protocols/mqtt_protocol.cc"
            "protocols/websocket_protocol.cc"
            "mcp_server.cc"
            "system_info.cc"
            "application.cc"
            "ota.cc"
            "settings.cc"
            "device_state_machine.cc"
            "assets.cc"
            "main.cc"
            )

set(INCLUDE_DIRS "." "display" "display/lvgl_display" "display/lvgl_display/jpg" "audio" "audio/demuxer" "protocols")

# Board common sources
list(APPEND SOURCES
    "boards/common/board.cc"
    "boards/common/wifi_board.cc"
    "boards/common/adc_battery_monitor.cc"
    "boards/common/backlight.cc"
    "boards/common/button.cc"
    "boards/common/i2c_device.cc"
    "boards/common/power_save_timer.cc"
    "boards/common/sleep_timer.cc"
    "boards/common/system_reset.cc"
)
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards/common)

idf_build_get_property(build_components BUILD_COMPONENTS)

# Helper function to find component dynamically by pattern
function(find_component_by_pattern PATTERN COMPONENT_VAR PATH_VAR)
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${PATTERN}")
            set(${COMPONENT_VAR} ${COMPONENT} PARENT_SCOPE)
            idf_component_get_property(COMPONENT_PATH ${COMPONENT} COMPONENT_DIR)
            set(${PATH_VAR} "${COMPONENT_PATH}" PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

# Default fonts and layout for ST7735 128x160 display
set(BUILTIN_TEXT_FONT font_puhui_basic_16_4)
set(BUILTIN_ICON_FONT font_awesome_16_4)
set(DEFAULT_EMOJI_COLLECTION twemoji_32)
set(EMOTE_RESOLUTION "128_160")

# Board Type definition: bread-compact-wifi-lcd
if(CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD)
    set(BOARD_TYPE "bread-compact-wifi-lcd")
    set(BUILTIN_TEXT_FONT font_puhui_basic_16_4)
    set(BUILTIN_ICON_FONT font_awesome_16_4)
    set(DEFAULT_EMOJI_COLLECTION twemoji_32)
else()
    set(BOARD_TYPE "bread-compact-wifi-lcd")
endif()

file(GLOB BOARD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD_TYPE}/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD_TYPE}/*.c
)
list(APPEND SOURCES ${BOARD_SOURCES})

# Audio processor configuration
if(CONFIG_USE_AUDIO_PROCESSOR)
    list(APPEND SOURCES "audio/processors/afe_audio_processor.cc")
else()
    list(APPEND SOURCES "audio/processors/no_audio_processor.cc")
endif()

# Wake word support for ESP32-S3
list(APPEND SOURCES "audio/wake_words/afe_wake_word.cc")
list(APPEND SOURCES "audio/wake_words/custom_wake_word.cc")

# Wi-Fi Provisioning
if(CONFIG_USE_ESP_BLUFI_WIFI_PROVISIONING)
    list(APPEND SOURCES "boards/common/blufi.cpp")
endif()

# Language selection (Harvya AI: English and Hindi)
if(CONFIG_LANGUAGE_HI_IN)
    set(LANG_DIR "hi-IN")
else()
    set(LANG_DIR "en-US")
endif()

set(LANG_JSON "${CMAKE_CURRENT_SOURCE_DIR}/assets/locales/${LANG_DIR}/language.json")
set(LANG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/assets/lang_config.h")

# Collect current language audio files
file(GLOB LANG_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/locales/${LANG_DIR}/*.ogg)

# If Hindi selected, fallback to en-US for any missing audio prompts
if(NOT LANG_DIR STREQUAL "en-US")
    file(GLOB EN_US_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/locales/en-US/*.ogg)
    set(EXISTING_NAMES "")
    foreach(SOUND_FILE ${LANG_SOUNDS})
        get_filename_component(FILENAME ${SOUND_FILE} NAME)
        list(APPEND EXISTING_NAMES ${FILENAME})
    endforeach()

    foreach(FALLBACK_FILE ${EN_US_SOUNDS})
        get_filename_component(FILENAME ${FALLBACK_FILE} NAME)
        list(FIND EXISTING_NAMES ${FILENAME} INDEX)
        if(INDEX EQUAL -1)
            list(APPEND LANG_SOUNDS ${FALLBACK_FILE})
        endif()
    endforeach()
endif()

file(GLOB COMMON_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/common/*.ogg)

idf_component_register(SRCS ${SOURCES}
                    EMBED_FILES ${LANG_SOUNDS} ${COMMON_SOUNDS}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    WHOLE_ARCHIVE
                    PRIV_REQUIRES
                        esp_pm
                        esp_psram
                        esp_netif
                        esp_driver_gpio
                        esp_driver_uart
                        esp_driver_spi
                        esp_driver_i2c
                        esp_driver_i2s
                        esp_driver_jpeg
                        esp_app_format
                        app_update
                        spi_flash
                        console
                        efuse
                        bt
                        fatfs
                    )

if(NOT BOARD_NAME)
    set(BOARD_NAME ${BOARD_TYPE})
endif()
target_compile_definitions(${COMPONENT_LIB}
                    PRIVATE BOARD_TYPE=\"${BOARD_TYPE}\" BOARD_NAME=\"${BOARD_NAME}\"
                    PRIVATE BUILTIN_TEXT_FONT=${BUILTIN_TEXT_FONT} BUILTIN_ICON_FONT=${BUILTIN_ICON_FONT}
                    )

# Generation rule for language config
add_custom_command(
    OUTPUT ${LANG_HEADER}
    COMMAND python ${PROJECT_DIR}/scripts/gen_lang.py
            --language "${LANG_DIR}"
            --output "${LANG_HEADER}"
    DEPENDS
        ${LANG_JSON}
        ${PROJECT_DIR}/scripts/gen_lang.py
    COMMENT "Generating ${LANG_DIR} language config"
)

add_custom_target(lang_header ALL
    DEPENDS ${LANG_HEADER}
)

# ESP-SR component model path
find_component_by_pattern("espressif__esp-sr" ESP_SR_COMPONENT ESP_SR_COMPONENT_PATH)
if(ESP_SR_COMPONENT_PATH)
    set(ESP_SR_MODEL_PATH "${ESP_SR_COMPONENT_PATH}/model")
endif()

# Fonts path
find_component_by_pattern("harvya-ai-fonts" HARVYA_AI_FONTS_COMPONENT HARVYA_AI_FONTS_COMPONENT_PATH)
if(HARVYA_AI_FONTS_COMPONENT_PATH)
    set(HARVYA_AI_FONTS_PATH "${HARVYA_AI_FONTS_COMPONENT_PATH}")
endif()

# Assets builder function
function(build_default_assets_bin)
    set(GENERATED_ASSETS_BIN "${CMAKE_BINARY_DIR}/generated_assets.bin")
    set(BUILD_ARGS
        "--sdkconfig" "${SDKCONFIG}"
        "--output" "${GENERATED_ASSETS_BIN}"
    )

    if(BUILTIN_TEXT_FONT)
        list(APPEND BUILD_ARGS "--builtin_text_font" "${BUILTIN_TEXT_FONT}")
    endif()

    if(DEFAULT_EMOJI_COLLECTION)
        list(APPEND BUILD_ARGS "--emoji_collection" "${DEFAULT_EMOJI_COLLECTION}")
    endif()

    if(DEFAULT_ASSETS_EXTRA_FILES)
        list(APPEND BUILD_ARGS "--extra_files" "${DEFAULT_ASSETS_EXTRA_FILES}")
    endif()

    list(APPEND BUILD_ARGS "--esp_sr_model_path" "${ESP_SR_MODEL_PATH}")
    list(APPEND BUILD_ARGS "--HARVYA_AI_FONTS_path" "${HARVYA_AI_FONTS_PATH}")

    add_custom_command(
        OUTPUT ${GENERATED_ASSETS_BIN}
        COMMAND python ${PROJECT_DIR}/scripts/build_default_assets.py ${BUILD_ARGS}
        DEPENDS
            ${SDKCONFIG}
            ${PROJECT_DIR}/scripts/build_default_assets.py
        COMMENT "Building default assets.bin for Harvya AI"
        VERBATIM
    )

    add_custom_target(generated_default_assets ALL
        DEPENDS ${GENERATED_ASSETS_BIN}
    )

    set(GENERATED_ASSETS_LOCAL_FILE ${GENERATED_ASSETS_BIN} PARENT_SCOPE)
    message(STATUS "Default assets build configured: ${GENERATED_ASSETS_BIN}")
endfunction()

# Function to get local assets file path
function(get_assets_local_file assets_source assets_local_file_var)
    if(assets_source MATCHES "^https?://")
        get_filename_component(ASSETS_FILENAME "${assets_source}" NAME)
        set(ASSETS_LOCAL_FILE "${CMAKE_BINARY_DIR}/${ASSETS_FILENAME}")
        set(ASSETS_TEMP_FILE "${CMAKE_BINARY_DIR}/${ASSETS_FILENAME}.tmp")

        if(EXISTS ${ASSETS_LOCAL_FILE})
            message(STATUS "Assets file ${ASSETS_FILENAME} already exists, skipping download")
        else()
            message(STATUS "Downloading ${ASSETS_FILENAME}")
            if(EXISTS ${ASSETS_TEMP_FILE})
                file(REMOVE ${ASSETS_TEMP_FILE})
            endif()

            file(DOWNLOAD ${assets_source} ${ASSETS_TEMP_FILE} STATUS DOWNLOAD_STATUS)
            list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
            if(NOT STATUS_CODE EQUAL 0)
                if(EXISTS ${ASSETS_TEMP_FILE})
                    file(REMOVE ${ASSETS_TEMP_FILE})
                endif()
                message(FATAL_ERROR "Failed to download ${ASSETS_FILENAME} from ${assets_source}")
            endif()

            file(RENAME ${ASSETS_TEMP_FILE} ${ASSETS_LOCAL_FILE})
            message(STATUS "Successfully downloaded ${ASSETS_FILENAME}")
        endif()
    else()
        if(IS_ABSOLUTE "${assets_source}")
            set(ASSETS_LOCAL_FILE "${assets_source}")
        else()
            set(ASSETS_LOCAL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${assets_source}")
        endif()

        if(NOT EXISTS ${ASSETS_LOCAL_FILE})
            message(FATAL_ERROR "Assets file not found: ${ASSETS_LOCAL_FILE}")
        endif()
        message(STATUS "Using assets file: ${ASSETS_LOCAL_FILE}")
    endif()

    set(${assets_local_file_var} ${ASSETS_LOCAL_FILE} PARENT_SCOPE)
endfunction()

partition_table_get_partition_info(size "--partition-name assets" "size")
partition_table_get_partition_info(offset "--partition-name assets" "offset")
if ("${size}" AND "${offset}")
    if(CONFIG_FLASH_DEFAULT_ASSETS)
        build_default_assets_bin()
        esptool_py_flash_to_partition(flash "assets" "${GENERATED_ASSETS_LOCAL_FILE}")
        message(STATUS "Generated default assets flash configured: ${GENERATED_ASSETS_LOCAL_FILE} -> assets partition")
    elseif(CONFIG_FLASH_CUSTOM_ASSETS)
        get_assets_local_file("${CONFIG_CUSTOM_ASSETS_FILE}" ASSETS_LOCAL_FILE)
        esptool_py_flash_to_partition(flash "assets" "${ASSETS_LOCAL_FILE}")
        message(STATUS "Custom assets flash configured: ${ASSETS_LOCAL_FILE} -> assets partition")
    elseif(CONFIG_FLASH_NONE_ASSETS)
        message(STATUS "Assets flashing disabled (FLASH_NONE_ASSETS)")
    endif()
else()
    message(STATUS "Assets partition not found, using v1 partition table")
endif()
