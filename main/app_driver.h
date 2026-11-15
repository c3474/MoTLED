/*
 * TLED - Matter-over-Thread LED Controller
 * LED Driver Interface
 */

#pragma once

#include <esp_err.h>
#include <esp_matter.h>
#include "app_nvs_config.h"
#include "tled_color_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle for the driver
 */
typedef void *app_driver_handle_t;

/**
 * @brief Initialize the LED driver (onboard LED for Phase 1)
 *
 * @return Driver handle, or NULL on failure
 */
app_driver_handle_t app_driver_light_init(void);

/**
 * @brief Initialize the button driver
 *
 * @return Driver handle, or NULL on failure
 */
app_driver_handle_t app_driver_button_init(void);

/**
 * @brief Set the LED power state
 *
 * @param handle Driver handle
 * @param power true = on, false = off
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_power(app_driver_handle_t handle, bool power);

/**
 * @brief Get the current LED power state
 *
 * @param handle Driver handle
 * @return true if LED is on, false if off
 */
bool app_driver_light_get_power(app_driver_handle_t handle);

/**
 * @brief Set brightness level
 *
 * @param handle Driver handle
 * @param brightness Brightness level (0-254 Matter range)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_brightness(app_driver_handle_t handle, uint8_t brightness);

/**
 * @brief Set color using HSV
 *
 * @param handle Driver handle
 * @param hue Hue (0-254 Matter range)
 * @param saturation Saturation (0-254 Matter range)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_hsv(app_driver_handle_t handle, uint8_t hue, uint8_t saturation);

/**
 * @brief Handle Matter attribute updates
 *
 * Called by the Matter stack when attributes change.
 *
 * @param driver_handle Driver handle
 * @param endpoint_id Matter endpoint ID
 * @param cluster_id Matter cluster ID
 * @param attribute_id Matter attribute ID
 * @param val New attribute value
 * @return ESP_OK on success
 */
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle,
                                       uint16_t endpoint_id,
                                       uint32_t cluster_id,
                                       uint32_t attribute_id,
                                       esp_matter_attr_val_t *val);

/**
 * @brief Apply default settings to the light
 *
 * Reads current attribute values and applies them to hardware.
 *
 * @param endpoint_id Matter endpoint ID
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_defaults(uint16_t endpoint_id);

/**
 * @brief Set brightness with transition time
 *
 * @param handle Driver handle
 * @param brightness Target brightness (0-254 Matter range)
 * @param transition_ms Transition time in milliseconds (0 = instant)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_brightness_with_transition(app_driver_handle_t handle,
                                                           uint8_t brightness,
                                                           uint32_t transition_ms);

/**
 * @brief Set HSV color with transition time
 *
 * @param handle Driver handle
 * @param hue Target hue (0-254 Matter range)
 * @param saturation Target saturation (0-254 Matter range)
 * @param transition_ms Transition time in milliseconds (0 = instant)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_hsv_with_transition(app_driver_handle_t handle,
                                                    uint8_t hue,
                                                    uint8_t saturation,
                                                    uint32_t transition_ms);

/**
 * @brief Set color temperature (WS2805 white channels)
 *
 * Switches the light to color temperature mode: warm/cool white channels are
 * driven and RGB is turned off. Only effective on WS2805 strips.
 *
 * @param handle Driver handle
 * @param mireds Color temperature in mireds (clamped to TLED_CT_MIN/MAX_MIREDS)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_color_temp(app_driver_handle_t handle, uint16_t mireds);

/**
 * @brief Set color temperature with transition time
 *
 * @param handle Driver handle
 * @param mireds Color temperature in mireds
 * @param transition_ms Transition time in milliseconds (0 = instant)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_color_temp_with_transition(app_driver_handle_t handle,
                                                          uint16_t mireds,
                                                          uint32_t transition_ms);

/**
 * @brief Show a multi-color pattern instead of the single light color
 *
 * The pattern follows the light's power and brightness. Not saved across reboots.
 *
 * @param handle Driver handle
 * @param colors RGB colors (0-255 each); ignored when count is 0
 * @param count Number of colors (1-4), or 0 to turn the pattern off
 * @param layout Pattern layout (tled_pattern_layout_t)
 * @param group Pixels per color band for PATTERN_LAYOUT_ALTERNATE (1-255)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on bad arguments
 */
esp_err_t app_driver_light_set_pattern(app_driver_handle_t handle,
                                       const uint8_t colors[][3], uint8_t count,
                                       uint8_t layout, uint8_t group);

/**
 * @brief Load the "Color 1-4" lights' saved state and show their pattern
 *
 * Call after Matter has started, alongside app_driver_light_set_defaults().
 *
 * @param handle Driver handle
 * @return ESP_OK on success
 */
esp_err_t app_driver_color_slots_set_defaults(app_driver_handle_t handle);

/**
 * @brief Set the motion layered on top of the light's colors
 *
 * Follows the light's power and brightness, and works with the single
 * color, Color 1-4 patterns and rainbow.
 *
 * @param handle Driver handle
 * @param motion TLED_MOTION_* flags (0 = still)
 * @return ESP_OK on success
 */
esp_err_t app_driver_light_set_motion(app_driver_handle_t handle, uint8_t motion);

/**
 * @brief Get the current TLED_MOTION_* flags
 */
uint8_t app_driver_light_get_motion(app_driver_handle_t handle);

/**
 * @brief Load the effect switches' saved state and start their motion
 *
 * Call after Matter has started, alongside app_driver_light_set_defaults().
 */
esp_err_t app_driver_effect_switches_set_defaults(app_driver_handle_t handle);

#ifdef __cplusplus
}
#endif
