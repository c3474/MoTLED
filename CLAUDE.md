# MoTLED: Matter-over-Thread LED firmware

Fork of [TLED](https://github.com/maui1911/TLED) for ESP32-C6, tuned for Apple Home.

- **Plans and status:** `ROADMAP.md`
- **How it behaves for users:** `docs/user-guide.md` (keep it and the web page's Docs tab in step with every user-visible change)
- **Web flasher:** <https://c3474.github.io/MoTLED/>

## Build and publish

There is no local toolchain. GitHub Actions (`.github/workflows/build-and-deploy.yml`) builds every push in the
`espressif/esp-matter:release-v1.5_idf_v5.4.1` container. Pushes to `main` also publish the web installer, with
that firmware, to GitHub Pages. Work on a feature branch, let CI build it, then fast-forward `main`.

## Layout

```
main/
├── app_main.cpp           # Matter node and endpoints (main light, Color 1-4, effect switches, temperature)
├── app_driver.cpp/.h      # LED output: transitions, patterns, motion, state persistence
├── ws2805_strip.c/.h      # WS2805 5-channel (RGB + warm/cool white) RMT driver
├── app_nvs_config.cpp/.h  # Saved settings and their migrations
├── app_serial_config.cpp  # USB serial commands used by the web page's Configure tab
├── app_monitoring.cpp     # Heap/temperature logging, watchdog
└── app_device_info.cpp    # Matter device identification
web-installer/index.html   # Flasher, Configure tab and Docs tab (single file)
test/                      # Unit tests for the pure color/pattern helpers
```

## Rules that are easy to break

- **Settings layout:** `tled_config_t` is stored as one blob and the loader picks the migration by blob size.
  Any layout change needs a `TLED_CONFIG_VERSION` bump, a `tled_config_vN_t` copy, a `migrate_vN_config`, and a
  size that differs from every older layout (static_asserts check this).
- **Matter endpoints:** changing which endpoints exist forces users to remove and re-add the device in Apple Home.
  New features default to off, and migrations keep an existing device's endpoints.
- **Serial output** ends lines with `\r\n`; the web page splits on `\r?\n`.
