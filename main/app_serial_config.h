/*
 * TLED Serial Configuration
 * Allows runtime configuration via USB serial commands
 */

#pragma once

#include "esp_err.h"
#include "app_driver.h"

/**
 * Initialize the serial configuration handler
 * Starts a task that listens for commands on UART
 *
 * @param light_handle LED driver handle, used by the 'effect' command
 */
esp_err_t serial_config_init(app_driver_handle_t light_handle);

/**
 * Check if currently in config mode
 */
bool serial_config_is_active(void);
