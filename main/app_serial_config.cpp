/*
 * TLED Serial Configuration
 * Handles serial commands for runtime configuration
 */

#include "app_serial_config.h"
#include "app_nvs_config.h"
#include "app_config.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_system.h"
#include <esp_matter.h>

static const char *TAG = "serial_config";

#define SERIAL_BUF_SIZE 256
#define CMD_BUF_SIZE 128

static bool s_config_active = false;
static app_driver_handle_t s_light_handle = NULL;

// Effect names, indexed by TLED_EFFECT_* (app_config.h)
static const char *effect_names[] = {"off", "rainbow", "breathing", "candle", "chase"};
#define NUM_EFFECTS (sizeof(effect_names) / sizeof(effect_names[0]))
static char s_cmd_buf[CMD_BUF_SIZE];
static int s_cmd_pos = 0;

// Forward declarations
static void process_command(const char *cmd);
static void print_help(void);
static void print_config(void);
static void handle_set_command(const char *param, const char *value);
static void handle_effect_command(const char *name);
static void handle_pattern_command(const char *args);
static const char *white_mode_to_str(uint8_t mode);

static void serial_write(const char *str) {
    usb_serial_jtag_write_bytes((const uint8_t *)str, strlen(str), pdMS_TO_TICKS(100));
}

static void serial_printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    serial_write(buf);
}

static void serial_config_task(void *arg) {
    uint8_t rx_buf[64];

    // Print welcome message
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for USB to stabilize
    serial_write("\r\n");
    serial_write("========================================\r\n");
    serial_write("  TLED Serial Configuration\r\n");
    serial_write("  Type 'help' for available commands\r\n");
    serial_write("========================================\r\n");
    print_config();
    serial_write("\r\n> ");

    s_config_active = true;

    while (1) {
        int len = usb_serial_jtag_read_bytes(rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)rx_buf[i];

                // Handle backspace
                if (c == '\b' || c == 127) {
                    if (s_cmd_pos > 0) {
                        s_cmd_pos--;
                        serial_write("\b \b");
                    }
                    continue;
                }

                // Handle enter
                if (c == '\r' || c == '\n') {
                    serial_write("\r\n");
                    s_cmd_buf[s_cmd_pos] = '\0';

                    if (s_cmd_pos > 0) {
                        process_command(s_cmd_buf);
                    }

                    s_cmd_pos = 0;
                    serial_write("> ");
                    continue;
                }

                // Add character to buffer
                if (s_cmd_pos < CMD_BUF_SIZE - 1 && c >= 32 && c < 127) {
                    s_cmd_buf[s_cmd_pos++] = c;
                    // Echo character
                    char echo[2] = {c, '\0'};
                    serial_write(echo);
                }
            }
        }
    }
}

static void process_command(const char *cmd) {
    // Skip leading whitespace
    while (*cmd == ' ') cmd++;

    if (strlen(cmd) == 0) {
        return;
    }

    // 'pattern' takes more arguments than the generic parser below handles
    if (strncmp(cmd, "pattern", 7) == 0 && (cmd[7] == '\0' || cmd[7] == ' ')) {
        handle_pattern_command(cmd + 7);
        return;
    }

    // Parse command
    char command[32] = {0};
    char param[32] = {0};
    char value[32] = {0};

    int parsed = sscanf(cmd, "%31s %31s %31s", command, param, value);

    if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
        print_help();
    }
    else if (strcmp(command, "config") == 0 || strcmp(command, "show") == 0) {
        print_config();
    }
    else if (strcmp(command, "set") == 0 && parsed >= 3) {
        handle_set_command(param, value);
    }
    else if (strcmp(command, "save") == 0) {
        serial_write("Saving configuration to NVS...\r\n");
        esp_err_t err = tled_config_save();
        if (err == ESP_OK) {
            serial_write("Configuration saved successfully!\r\n");
            serial_write("Rebooting in 2 seconds...\r\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        } else if (err == ESP_ERR_INVALID_ARG) {
            serial_write("Error: current config is invalid, not saved (check 'show')\r\n");
        } else {
            serial_printf("Error saving config: %s\r\n", esp_err_to_name(err));
        }
    }
    else if (strcmp(command, "effect") == 0) {
        handle_effect_command(parsed >= 2 ? param : NULL);
    }
    else if (strcmp(command, "reboot") == 0) {
        serial_write("Rebooting...\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }
    else if (strcmp(command, "factory") == 0) {
        serial_write("Resetting to factory defaults (including Matter fabric)...\r\n");
        tled_config_reset();
        tled_config_save();
        serial_write("Clearing Matter commissioning data...\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        // This clears fabric data and reboots
        esp_matter::factory_reset();
    }
    else {
        serial_printf("Unknown command: %s\r\n", command);
        serial_write("Type 'help' for available commands\r\n");
    }
}

// Start or stop a built-in effect (testing aid; not saved, and any change
// from the Home app stops the effect)
static void handle_effect_command(const char *name) {
    if (s_light_handle == NULL) {
        serial_write("Error: LED driver not available\r\n");
        return;
    }
    if (name == NULL) {
        uint8_t current = app_driver_light_get_effect(s_light_handle);
        serial_printf("effect = %s\r\n", current < NUM_EFFECTS ? effect_names[current] : "unknown");
        return;
    }
    for (size_t i = 0; i < NUM_EFFECTS; i++) {
        if (strcmp(name, effect_names[i]) == 0) {
            app_driver_light_set_effect(s_light_handle, (uint8_t)i);
            serial_printf("Set effect = %s\r\n", name);
            return;
        }
    }
    serial_write("Error: effect must be rainbow, breathing, candle, chase, or off\r\n");
}

// Named colors for the 'pattern' command (RGB 0-255)
static const struct {
    const char *name;
    uint8_t rgb[3];
} named_colors[] = {
    {"red",    {255, 0, 0}},
    {"orange", {255, 80, 0}},
    {"yellow", {255, 180, 0}},
    {"green",  {0, 255, 0}},
    {"cyan",   {0, 255, 255}},
    {"blue",   {0, 0, 255}},
    {"purple", {140, 0, 255}},
    {"pink",   {255, 40, 120}},
    {"white",  {255, 255, 255}},
};
#define NUM_NAMED_COLORS (sizeof(named_colors) / sizeof(named_colors[0]))

// Parse a color name or 6-digit hex value (e.g. ff8800)
static bool parse_color(const char *token, uint8_t rgb[3])
{
    for (size_t i = 0; i < NUM_NAMED_COLORS; i++) {
        if (strcmp(token, named_colors[i].name) == 0) {
            memcpy(rgb, named_colors[i].rgb, 3);
            return true;
        }
    }
    if (strlen(token) == 6 && strspn(token, "0123456789abcdefABCDEF") == 6) {
        unsigned long v = strtoul(token, NULL, 16);
        rgb[0] = (v >> 16) & 0xFF;
        rgb[1] = (v >> 8) & 0xFF;
        rgb[2] = v & 0xFF;
        return true;
    }
    return false;
}

// Show a multi-color pattern (testing aid; not saved):
//   pattern <color> [color] [color] [color] [alt <n> | blocks]
//   pattern off
static void handle_pattern_command(const char *args) {
    if (s_light_handle == NULL) {
        serial_write("Error: LED driver not available\r\n");
        return;
    }

    char buf[CMD_BUF_SIZE];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    uint8_t colors[TLED_PATTERN_MAX_COLORS][3];
    uint8_t count = 0;
    uint8_t layout = PATTERN_LAYOUT_ALTERNATE;
    int group = TLED_PATTERN_DEFAULT_GROUP;

    char *save = NULL;
    char *tok = strtok_r(buf, " ", &save);
    if (tok == NULL) {
        serial_write("Usage: pattern <color> [color] [color] [color] [alt <n> | blocks]\r\n");
        serial_write("       pattern off\r\n");
        serial_write("Colors: red orange yellow green cyan blue purple pink white, or hex like ff8800\r\n");
        return;
    }
    if (strcmp(tok, "off") == 0) {
        app_driver_light_set_pattern(s_light_handle, colors, 0, layout, (uint8_t)group);
        serial_write("Pattern off\r\n");
        return;
    }

    for (; tok != NULL; tok = strtok_r(NULL, " ", &save)) {
        if (strcmp(tok, "blocks") == 0) {
            layout = PATTERN_LAYOUT_BLOCKS;
        } else if (strcmp(tok, "alt") == 0) {
            layout = PATTERN_LAYOUT_ALTERNATE;
            char *n = strtok_r(NULL, " ", &save);
            if (n != NULL) {
                group = atoi(n);
                if (group < 1 || group > 255) {
                    serial_write("Error: alt group must be 1-255 pixels\r\n");
                    return;
                }
            }
        } else if (count >= TLED_PATTERN_MAX_COLORS) {
            serial_write("Error: at most 4 colors\r\n");
            return;
        } else if (!parse_color(tok, colors[count])) {
            serial_printf("Error: unknown color '%s'\r\n", tok);
            return;
        } else {
            count++;
        }
    }

    if (count == 0) {
        serial_write("Error: give at least one color\r\n");
        return;
    }

    app_driver_light_set_pattern(s_light_handle, colors, count, layout, (uint8_t)group);
    if (layout == PATTERN_LAYOUT_BLOCKS) {
        serial_printf("Pattern: %d colors in blocks\r\n", count);
    } else {
        serial_printf("Pattern: %d colors alternating every %d pixels\r\n", count, group);
    }
}

static void print_help(void) {
    serial_write("\r\n");
    serial_write("Available commands:\r\n");
    serial_write("  help              - Show this help\r\n");
    serial_write("  config            - Show current configuration\r\n");
    serial_write("  set leds <n>      - Set number of LEDs (1-1000)\r\n");
    serial_write("  set gpio <n>      - Set data GPIO pin (0-21)\r\n");
    serial_write("  set brightness <n> - Set max brightness (1-255)\r\n");
    serial_write("  set type <t>      - Set LED type: ws2812b, ws2811, sk6812, ws2805\r\n");
    serial_write("  set order <o>     - Set RGB order: grb, rgb, brg, rbg, bgr, gbr\r\n");
    serial_write("                      RGBW: add w where white goes, e.g. wrgb, grbw\r\n");
    serial_write("  set bin <n|off>   - WS2805 BIN backup data GPIO (off = disabled)\r\n");
    serial_write("  set white_order <o> - WS2805 white channels: ww_cw, cw_ww\r\n");
    serial_write("  set name <name>   - Set device name\r\n");
    serial_write("  set poweron <m>   - Power-on behavior: restore, on, off\r\n");
    serial_write("  set white_mode <m> - RGBW white mode: accurate, brighter, none, dual, max\r\n");
    serial_write("  set white <n>     - Manual RGBW white level (0-255)\r\n");
    serial_write("  set gain_r <n>    - Red channel gain (0-255)\r\n");
    serial_write("  set gain_g <n>    - Green channel gain (0-255)\r\n");
    serial_write("  set gain_b <n>    - Blue channel gain (0-255)\r\n");
    serial_write("  set gain_w <n>    - White channel gain (0-255)\r\n");
    serial_write("  effect [name]     - Run effect: rainbow, breathing, candle, chase, off\r\n");
    serial_write("                      (testing aid; not saved, Home app changes stop it)\r\n");
    serial_write("  pattern <colors>  - Show 1-4 colors, e.g. 'pattern red green' or\r\n");
    serial_write("                      'pattern orange purple blocks' or '... alt 5'\r\n");
    serial_write("                      'pattern off' returns to one color (not saved)\r\n");
    serial_write("  save              - Save config and reboot\r\n");
    serial_write("  reboot            - Reboot without saving\r\n");
    serial_write("  factory           - Reset to factory defaults\r\n");
    serial_write("\r\n");
}

// Color order names, indexed by tled_rgb_order_t
static const char *rgb_order_names[] = {"grb", "rgb", "brg", "rbg", "bgr", "gbr"};
#define NUM_RGB_ORDERS (sizeof(rgb_order_names) / sizeof(rgb_order_names[0]))

// Parse a color order: 3 letters ("grb") or 4 letters with one 'w' ("wrgb").
// The 4-letter form is split into an RGB order plus the white byte position;
// the 3-letter form puts white last, as before.
static bool parse_order(const char *value, uint8_t *rgb_order, uint8_t *white_pos)
{
    char rgb[4] = {0};
    size_t pos = TLED_WHITE_POS_LAST;
    size_t len = strlen(value);

    if (len == 4) {
        const char *w = strchr(value, 'w');
        if (w == NULL || strchr(w + 1, 'w') != NULL) {
            return false;
        }
        pos = w - value;
        for (size_t i = 0, j = 0; i < len; i++) {
            if (i != pos) {
                rgb[j++] = value[i];
            }
        }
    } else if (len == 3) {
        memcpy(rgb, value, 3);
    } else {
        return false;
    }

    for (size_t i = 0; i < NUM_RGB_ORDERS; i++) {
        if (strcmp(rgb, rgb_order_names[i]) == 0) {
            *rgb_order = (uint8_t)i;
            *white_pos = (uint8_t)pos;
            return true;
        }
    }
    return false;
}

// Format the color order, inserting 'w' when white isn't in the default last position
static void format_order(const tled_config_t *cfg, char *out, size_t out_size)
{
    if (cfg->rgb_order >= NUM_RGB_ORDERS) {
        snprintf(out, out_size, "unknown");
        return;
    }
    const char *rgb = rgb_order_names[cfg->rgb_order];
    if (cfg->white_pos == TLED_WHITE_POS_LAST) {
        snprintf(out, out_size, "%s", rgb);
        return;
    }
    char order[5] = {0};
    for (int i = 0, j = 0; i < 4; i++) {
        order[i] = (i == cfg->white_pos) ? 'w' : rgb[j++];
    }
    snprintf(out, out_size, "%s", order);
}

static void print_config(void) {
    const tled_config_t *cfg = tled_config_get();

    const char *type_str = "unknown";
    switch (cfg->chipset) {
        case CHIPSET_WS2812B: type_str = "ws2812b"; break;
        case CHIPSET_WS2811: type_str = "ws2811"; break;
        case CHIPSET_SK6812: type_str = "sk6812"; break;
        case CHIPSET_WS2805: type_str = "ws2805"; break;
    }

    char order_str[8];
    format_order(cfg, order_str, sizeof(order_str));

    const char *poweron_str = "unknown";
    switch (cfg->power_on_behavior) {
        case POWER_ON_RESTORE: poweron_str = "restore"; break;
        case POWER_ON_ON: poweron_str = "on"; break;
        case POWER_ON_OFF: poweron_str = "off"; break;
    }

    serial_write("\r\nCurrent configuration:\r\n");
    serial_printf("  leds       = %d\r\n", cfg->num_leds);
    serial_printf("  gpio       = %d\r\n", cfg->gpio_pin);
    serial_printf("  brightness = %d\r\n", cfg->max_brightness);
    serial_printf("  type       = %s\r\n", type_str);
    serial_printf("  order      = %s\r\n", order_str);
    serial_printf("  poweron    = %s\r\n", poweron_str);
    serial_printf("  white_mode = %s\r\n", white_mode_to_str(cfg->white_mode));
    serial_printf("  white      = %d\r\n", cfg->manual_white);
    serial_printf("  gain_r     = %d\r\n", cfg->gain_r);
    serial_printf("  gain_g     = %d\r\n", cfg->gain_g);
    serial_printf("  gain_b     = %d\r\n", cfg->gain_b);
    serial_printf("  gain_w     = %d\r\n", cfg->gain_w);
    if (cfg->bin_gpio == TLED_BIN_GPIO_DISABLED) {
        serial_write("  bin        = off\r\n");
    } else {
        serial_printf("  bin        = %d\r\n", cfg->bin_gpio);
    }
    serial_printf("  white_order = %s\r\n", cfg->white_order == WHITE_ORDER_CW_WW ? "cw_ww" : "ww_cw");
    serial_printf("  name       = %s\r\n", cfg->device_name);
    serial_write("\r\n");
}

static const char *white_mode_to_str(uint8_t mode) {
    switch (mode) {
        case WHITE_MODE_ACCURATE: return "accurate";
        case WHITE_MODE_BRIGHTER: return "brighter";
        case WHITE_MODE_NONE: return "none";
        case WHITE_MODE_DUAL: return "dual";
        case WHITE_MODE_MAX: return "max";
        default: return "unknown";
    }
}

static bool parse_u8_value(const char *value, uint8_t *out) {
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0 || parsed > 255) {
        return false;
    }
    *out = (uint8_t)parsed;
    return true;
}

static void handle_set_command(const char *param, const char *value) {
    tled_config_t *cfg = tled_config_get_mutable();

    if (strcmp(param, "leds") == 0) {
        int n = atoi(value);
        if (n >= 1 && n <= 1000) {
            cfg->num_leds = n;
            serial_printf("Set leds = %d\r\n", n);
        } else {
            serial_write("Error: leds must be 1-1000\r\n");
        }
    }
    else if (strcmp(param, "gpio") == 0) {
        int n = atoi(value);
        if (!tled_config_validate_gpio((uint8_t)n)) {
            serial_write("Error: invalid GPIO pin (avoid 9, 12-13, 15)\r\n");
        } else if (cfg->bin_gpio != TLED_BIN_GPIO_DISABLED && (uint8_t)n == cfg->bin_gpio) {
            serial_printf("Error: GPIO %d is already used as BIN pin (run 'set bin off' first)\r\n", n);
        } else {
            cfg->gpio_pin = n;
            serial_printf("Set gpio = %d\r\n", n);
        }
    }
    else if (strcmp(param, "brightness") == 0) {
        int n = atoi(value);
        if (n >= 1 && n <= 255) {
            cfg->max_brightness = n;
            serial_printf("Set brightness = %d\r\n", n);
        } else {
            serial_write("Error: brightness must be 1-255\r\n");
        }
    }
    else if (strcmp(param, "type") == 0) {
        if (strcmp(value, "ws2812b") == 0) {
            cfg->chipset = CHIPSET_WS2812B;
            serial_write("Set type = ws2812b\r\n");
        } else if (strcmp(value, "ws2811") == 0) {
            cfg->chipset = CHIPSET_WS2811;
            serial_write("Set type = ws2811\r\n");
        } else if (strcmp(value, "sk6812") == 0) {
            cfg->chipset = CHIPSET_SK6812;
            serial_write("Set type = sk6812\r\n");
        } else if (strcmp(value, "ws2805") == 0) {
            cfg->chipset = CHIPSET_WS2805;
            serial_write("Set type = ws2805 (RGB + warm/cool white)\r\n");
            serial_write("Note: switching to/from ws2805 changes the Matter clusters -\r\n");
            serial_write("      remove and re-add the device in your smart home app.\r\n");
        } else {
            serial_write("Error: type must be ws2812b, ws2811, sk6812, or ws2805\r\n");
        }
    }
    else if (strcmp(param, "bin") == 0) {
        if (strcmp(value, "off") == 0) {
            cfg->bin_gpio = TLED_BIN_GPIO_DISABLED;
            serial_write("Set bin = off (BIN backup line disabled)\r\n");
        } else {
            uint8_t n;
            if (!parse_u8_value(value, &n)) {
                serial_write("Error: bin must be a GPIO number or 'off'\r\n");
            } else if (tled_config_validate_gpio(n) && n != cfg->gpio_pin) {
                cfg->bin_gpio = n;
                serial_printf("Set bin = %d\r\n", n);
            } else {
                serial_write("Error: invalid BIN GPIO (avoid 9, 12-13, 15 and the data pin)\r\n");
            }
        }
    }
    else if (strcmp(param, "white_order") == 0) {
        if (strcmp(value, "ww_cw") == 0) {
            cfg->white_order = WHITE_ORDER_WW_CW;
            serial_write("Set white_order = ww_cw (W1 = warm, W2 = cool)\r\n");
        } else if (strcmp(value, "cw_ww") == 0) {
            cfg->white_order = WHITE_ORDER_CW_WW;
            serial_write("Set white_order = cw_ww (W1 = cool, W2 = warm)\r\n");
        } else {
            serial_write("Error: white_order must be ww_cw or cw_ww\r\n");
        }
    }
    else if (strcmp(param, "order") == 0) {
        uint8_t rgb_order, white_pos;
        if (parse_order(value, &rgb_order, &white_pos)) {
            cfg->rgb_order = rgb_order;
            cfg->white_pos = white_pos;
            serial_printf("Set order = %s\r\n", value);
            if (white_pos != TLED_WHITE_POS_LAST && cfg->chipset != CHIPSET_SK6812) {
                serial_write("Note: white position only applies to type sk6812 (RGBW)\r\n");
            }
        } else {
            serial_write("Error: order must be grb, rgb, brg, rbg, bgr, gbr,\r\n");
            serial_write("       or 4 letters with one w for RGBW (e.g. wrgb, grbw)\r\n");
        }
    }
    else if (strcmp(param, "name") == 0) {
        strncpy(cfg->device_name, value, sizeof(cfg->device_name) - 1);
        cfg->device_name[sizeof(cfg->device_name) - 1] = '\0';
        serial_printf("Set name = %s\r\n", cfg->device_name);
    }
    else if (strcmp(param, "poweron") == 0) {
        if (strcmp(value, "restore") == 0) {
            cfg->power_on_behavior = POWER_ON_RESTORE;
            serial_write("Set poweron = restore (restore last state)\r\n");
        } else if (strcmp(value, "on") == 0) {
            cfg->power_on_behavior = POWER_ON_ON;
            serial_write("Set poweron = on (always turn on)\r\n");
        } else if (strcmp(value, "off") == 0) {
            cfg->power_on_behavior = POWER_ON_OFF;
            serial_write("Set poweron = off (always stay off)\r\n");
        } else {
            serial_write("Error: poweron must be restore, on, or off\r\n");
        }
    }
    else if (strcmp(param, "white_mode") == 0) {
        if (strcmp(value, "accurate") == 0) {
            cfg->white_mode = WHITE_MODE_ACCURATE;
            serial_write("Set white_mode = accurate\r\n");
        } else if (strcmp(value, "brighter") == 0) {
            cfg->white_mode = WHITE_MODE_BRIGHTER;
            serial_write("Set white_mode = brighter\r\n");
        } else if (strcmp(value, "none") == 0) {
            cfg->white_mode = WHITE_MODE_NONE;
            serial_write("Set white_mode = none\r\n");
        } else if (strcmp(value, "dual") == 0) {
            cfg->white_mode = WHITE_MODE_DUAL;
            serial_write("Set white_mode = dual\r\n");
        } else if (strcmp(value, "max") == 0) {
            cfg->white_mode = WHITE_MODE_MAX;
            serial_write("Set white_mode = max\r\n");
        } else {
            serial_write("Error: white_mode must be accurate, brighter, none, dual, or max\r\n");
        }
    }
    else if (strcmp(param, "white") == 0 ||
             strcmp(param, "gain_r") == 0 ||
             strcmp(param, "gain_g") == 0 ||
             strcmp(param, "gain_b") == 0 ||
             strcmp(param, "gain_w") == 0) {
        uint8_t n;
        if (!parse_u8_value(value, &n)) {
            serial_printf("Error: %s must be 0-255\r\n", param);
            return;
        }

        if (strcmp(param, "white") == 0) {
            cfg->manual_white = n;
        } else if (strcmp(param, "gain_r") == 0) {
            cfg->gain_r = n;
        } else if (strcmp(param, "gain_g") == 0) {
            cfg->gain_g = n;
        } else if (strcmp(param, "gain_b") == 0) {
            cfg->gain_b = n;
        } else {
            cfg->gain_w = n;
        }
        serial_printf("Set %s = %d\r\n", param, n);
    }
    else {
        serial_printf("Unknown parameter: %s\r\n", param);
        serial_write("Type 'help' for available parameters\r\n");
    }
}

esp_err_t serial_config_init(app_driver_handle_t light_handle) {
    s_light_handle = light_handle;

    // Configure USB Serial JTAG
    usb_serial_jtag_driver_config_t cfg = {
        .tx_buffer_size = SERIAL_BUF_SIZE,
        .rx_buffer_size = SERIAL_BUF_SIZE,
    };

    esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB serial driver: %s", esp_err_to_name(err));
        return err;
    }

    // Create the serial config task
    BaseType_t ret = xTaskCreate(
        serial_config_task,
        "serial_config",
        4096,
        NULL,
        5,
        NULL
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create serial config task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Serial configuration initialized");
    return ESP_OK;
}

bool serial_config_is_active(void) {
    return s_config_active;
}
