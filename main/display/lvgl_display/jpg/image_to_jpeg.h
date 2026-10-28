// image_to_jpeg.h - High-efficiency JPEG encoder interface for Harvya AI
#pragma once
#include "sdkconfig.h"
#ifndef CONFIG_IDF_TARGET_ESP32

#include <stdint.h>
#include <stddef.h>

// V4L2 Pixel Formats
#ifndef V4L2_PIX_FMT_RGB565
#define V4L2_PIX_FMT_RGB565 0x50424752  // 'RGBP'
#define V4L2_PIX_FMT_RGB565X 0x52474250 // 'PRGB'
#define V4L2_PIX_FMT_RGB24 0x33424752   // 'RGB3'
#define V4L2_PIX_FMT_YUYV 0x56595559    // 'YUYV'
#define V4L2_PIX_FMT_YUV422P 0x36315559 // 'YU16'
#define V4L2_PIX_FMT_YUV420 0x32315559  // 'YU12'
#define V4L2_PIX_FMT_GREY 0x59455247    // 'GREY'
#define V4L2_PIX_FMT_UYVY 0x59565955    // 'UYVY'
#define V4L2_PIX_FMT_JPEG 0x4745504A    // 'JPEG'
#endif

typedef uint32_t v4l2_pix_fmt_t;

#ifdef __cplusplus
extern "C"
{
#endif

    // JPEG output callback type
    typedef size_t (*jpg_out_cb)(void *arg, size_t index, const void *data, size_t len);

    /**
     * @brief Convert image buffer to JPEG
     *
     * @param src       Source image data
     * @param src_len   Source image byte length
     * @param width     Image width
     * @param height    Image height
     * @param format    Image format (e.g. V4L2_PIX_FMT_RGB565)
     * @param quality   JPEG quality (1-100)
     * @param out       Output JPEG buffer pointer (must be freed by caller)
     * @param out_len   Output JPEG length
     *
     * @return true on success, false on failure
     */
    bool image_to_jpeg(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                       v4l2_pix_fmt_t format, uint8_t quality, uint8_t **out, size_t *out_len);

    /**
     * @brief Convert image buffer to JPEG using streaming callback
     *
     * @param src       Source image data
     * @param src_len   Source image byte length
     * @param width     Image width
     * @param height    Image height
     * @param format    Image format
     * @param quality   JPEG quality (1-100)
     * @param cb        Output chunk callback
     * @param arg       User argument passed to callback
     *
     * @return true on success, false on failure
     */
    bool image_to_jpeg_cb(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                          v4l2_pix_fmt_t format, uint8_t quality, jpg_out_cb cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif // !CONFIG_IDF_TARGET_ESP32
