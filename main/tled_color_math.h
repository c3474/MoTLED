/*
 * TLED - Matter-over-Thread LED Controller
 * Pure color/pattern math (no ESP headers, so it also builds on a host PC)
 */

#pragma once

#include <stdint.h>
#include "app_nvs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} tled_rgbw_color_t;

static inline uint8_t tled_apply_channel_gain(uint8_t value, uint8_t gain)
{
    return (uint8_t)(((uint16_t)value * gain) / 255);
}

/**
 * @brief Convert logical RGB to RGBW using the configured SK6812 white mode.
 *
 * Pure helper for the output pipeline and unit tests. Input and output channels
 * are 0-255. Gains are applied after white mixing.
 */
static inline tled_rgbw_color_t tled_rgb_to_rgbw(uint8_t r, uint8_t g, uint8_t b,
                                                 uint8_t white_mode, uint8_t manual_white,
                                                 uint8_t gain_r, uint8_t gain_g,
                                                 uint8_t gain_b, uint8_t gain_w)
{
    uint8_t min_rgb = r < g ? r : g;
    min_rgb = min_rgb < b ? min_rgb : b;
    uint8_t max_rgb = r > g ? r : g;
    max_rgb = max_rgb > b ? max_rgb : b;

    uint8_t ro = r;
    uint8_t go = g;
    uint8_t bo = b;
    uint8_t wo = 0;

    switch (white_mode) {
        case WHITE_MODE_BRIGHTER:
            wo = min_rgb;
            break;
        case WHITE_MODE_NONE:
            wo = manual_white;
            break;
        case WHITE_MODE_DUAL:
            wo = manual_white > 0 ? manual_white : min_rgb;
            break;
        case WHITE_MODE_MAX:
            wo = max_rgb;
            break;
        case WHITE_MODE_ACCURATE:
        default:
            ro = r - min_rgb;
            go = g - min_rgb;
            bo = b - min_rgb;
            wo = min_rgb;
            break;
    }

    tled_rgbw_color_t out;
    out.r = tled_apply_channel_gain(ro, gain_r);
    out.g = tled_apply_channel_gain(go, gain_g);
    out.b = tled_apply_channel_gain(bo, gain_b);
    out.w = tled_apply_channel_gain(wo, gain_w);
    return out;
}

/**
 * @brief Position the white byte for strips that don't send W last (e.g. WRGB).
 *
 * led_strip in GRBW mode always transmits [green, red, blue, white]. The r/g/b
 * inputs must already be remapped for the strip's RGB order, so the color bytes
 * go out as [g, r, b]. This inserts w at white_pos (0 = first byte, 3 = last)
 * and returns the values to hand to led_strip_set_pixel_rgbw() so the bytes on
 * the wire come out in the strip's order.
 */
static inline tled_rgbw_color_t tled_place_white(uint8_t r, uint8_t g, uint8_t b,
                                                 uint8_t w, uint8_t white_pos)
{
    if (white_pos > TLED_WHITE_POS_LAST) {
        white_pos = TLED_WHITE_POS_LAST;
    }

    const uint8_t color[3] = {g, r, b};  // Color bytes in transmit order
    uint8_t wire[4];
    for (int i = 0, c = 0; i < 4; i++) {
        wire[i] = (i == white_pos) ? w : color[c++];
    }

    tled_rgbw_color_t out;
    out.g = wire[0];
    out.r = wire[1];
    out.b = wire[2];
    out.w = wire[3];
    return out;
}

// Multi-color patterns: up to 4 colors laid out along the strip
#define TLED_PATTERN_MAX_COLORS     4
#define TLED_PATTERN_DEFAULT_GROUP  3

typedef enum {
    PATTERN_LAYOUT_ALTERNATE = 0,   // Colors repeat every `group` pixels: A A A B B B A A A ...
    PATTERN_LAYOUT_BLOCKS = 1,      // Strip split into equal blocks: halves, thirds, quarters
} tled_pattern_layout_t;

// Motion layered on top of the light's colors (bit flags, combinable)
#define TLED_MOTION_RAINBOW     0x01    // Full rainbow along the strip (replaces the colors)
#define TLED_MOTION_CHASE       0x02    // Colors march along the strip
#define TLED_MOTION_TWINKLE     0x04    // Random pixels sparkle brighter and fade
#define TLED_MOTION_ALL         (TLED_MOTION_RAINBOW | TLED_MOTION_CHASE | TLED_MOTION_TWINKLE)
#define TLED_NUM_EFFECT_SWITCHES 3      // Rainbow, Chase, Twinkle switches in Home

/**
 * @brief Which pattern color (0..count-1) a pixel shows.
 *
 * Pure helper for the output pipeline and unit tests.
 */
static inline uint8_t tled_pattern_color_index(uint16_t pixel, uint16_t num_leds, uint8_t count,
                                               uint8_t layout, uint8_t group)
{
    if (count <= 1 || num_leds == 0) {
        return 0;
    }
    if (layout == PATTERN_LAYOUT_BLOCKS) {
        return (uint8_t)(((uint32_t)pixel * count) / num_leds);
    }
    if (group == 0) {
        group = 1;
    }
    return (uint8_t)((pixel / group) % count);
}

typedef struct {
    uint8_t ww;
    uint8_t cw;
} tled_white_channels_t;

/**
 * @brief Convert a color temperature to warm/cool white channel levels (WS2805).
 *
 * Pure helper for the output pipeline and unit tests. Linear mix between the
 * warm and cool white channels: at max_mireds (warmest) only WW is driven,
 * at min_mireds (coolest) only CW. The result is scaled by level (0-255
 * overall brightness) and gain_w. Out-of-range mireds are clamped.
 */
static inline tled_white_channels_t tled_ct_to_whites(uint16_t mireds,
                                                      uint16_t min_mireds, uint16_t max_mireds,
                                                      uint8_t level, uint8_t gain_w)
{
    if (mireds < min_mireds) {
        mireds = min_mireds;
    }
    if (mireds > max_mireds) {
        mireds = max_mireds;
    }

    // Warm fraction 0-255: 0 = coolest (min mireds), 255 = warmest (max mireds)
    uint32_t warm = ((uint32_t)(mireds - min_mireds) * 255) / (max_mireds - min_mireds);

    tled_white_channels_t out;
    out.ww = tled_apply_channel_gain((uint8_t)(((uint32_t)level * warm) / 255), gain_w);
    out.cw = tled_apply_channel_gain((uint8_t)(((uint32_t)level * (255 - warm)) / 255), gain_w);
    return out;
}

#ifdef __cplusplus
}
#endif
