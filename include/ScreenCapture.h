/*
 * Copyright 2018 ARP Network
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ARP_SCREEN_CAPTURE_H_
#define ARP_SCREEN_CAPTURE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    //
    // these constants need to match those
    // in graphics/PixelFormat.java & pixelflinger/format.h
    //
    PIXEL_FORMAT_UNKNOWN    =   0,
    PIXEL_FORMAT_NONE       =   0,

    // logical pixel formats used by the SurfaceFlinger -----------------------
    PIXEL_FORMAT_CUSTOM         = -4,
        // Custom pixel-format described by a PixelFormatInfo structure

    PIXEL_FORMAT_TRANSLUCENT    = -3,
        // System chooses a format that supports translucency (many alpha bits)

    PIXEL_FORMAT_TRANSPARENT    = -2,
        // System chooses a format that supports transparency
        // (at least 1 alpha bit)

    PIXEL_FORMAT_OPAQUE         = -1,
        // System chooses an opaque format (no alpha bits required)

    // real pixel formats supported for rendering -----------------------------

    PIXEL_FORMAT_RGBA_8888    = 1,                             // 4x8-bit RGBA
    PIXEL_FORMAT_RGBX_8888    = 2,                             // 4x8-bit RGB0
    PIXEL_FORMAT_RGB_888      = 3,                             // 3x8-bit RGB
    PIXEL_FORMAT_RGB_565      = 4,                             // 16-bit RGB
    PIXEL_FORMAT_BGRA_8888    = 5,                             // 4x8-bit BGRA
    PIXEL_FORMAT_RGBA_5551    = 6,                             // 16-bit ARGB
    PIXEL_FORMAT_RGBA_4444    = 7,                             // 16-bit ARGB
    PIXEL_FORMAT_RGBA_FP16    = 22,                            // 64-bit RGBA
    PIXEL_FORMAT_RGBA_1010102 = 43,                            // 32-bit RGB
};

/* Display orientations as defined in Surface.java and ISurfaceComposer.h. */
enum {
    DISPLAY_ORIENTATION_0 = 0,
    DISPLAY_ORIENTATION_90 = 1,
    DISPLAY_ORIENTATION_180 = 2,
    DISPLAY_ORIENTATION_270 = 3
};

typedef struct ARPDisplayInfo {
  uint32_t width{0};
  uint32_t height{0};
  uint8_t  orientation{0};
} ARPDisplayInfo;

typedef struct displayinfo {
  uint32_t width{0};
  uint32_t height{0};
  uint8_t  orientation{0};
} displayinfo;

typedef struct ARPFrameBuffer {
    uint8_t    *data;
    uint32_t    width;
    uint32_t    height;
    int32_t     format;
    uint32_t    stride;
    int64_t     timestamp;
    uint64_t    frame_number;
} ARPFrameBuffer;

typedef void (*arp_callback)(uint64_t frame_number, int64_t timestamp);

void arpcap_init();

void arpcap_fini();

int arpcap_get_display_info(ARPDisplayInfo *info);

int arpcap_create(
    uint32_t paddingTop, uint32_t paddingBottom, uint32_t width, uint32_t height, arp_callback cb);

void arpcap_destroy();

int arpcap_acquire_frame_buffer(ARPFrameBuffer *fb);

void arpcap_release_frame_buffer();

#ifdef __cplusplus
}
#endif

#endif // ARP_SCREEN_CAPTURE_H_
