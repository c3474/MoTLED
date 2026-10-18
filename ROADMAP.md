# MoTLED Roadmap

MoTLED (Matter-over-Thread LED) is a fork of [TLED](https://github.com/maui1911/TLED)
by maui1911. It is tuned for a Seeed XIAO ESP32C6 driving a BTF-Lighting WS2814
RGBW strip, controlled from Apple Home.

Items are listed in order. Each one gets its own branch, and the box is ticked
once it is merged and tested on the strip.

## Phase 1: get the WS2814 strip working

- [x] **M-1 WRGB color order.** Allow `set order wrgb` (and any other 4-letter
      order that includes W, such as `rgbw` or `grbw`) for SK6812-type strips.
      The setting persists in NVS, older saved configs migrate without being
      wiped, and the 3-letter orders behave as before.
- [x] **M-2 Web config UI.** Add the white-first orders to the installer's
      color-order dropdown, plus help on how to find a strip's order.
- [ ] **M-2b WS2814 guidance.** Make it obvious which type and order to pick
      for a WS2814. Options: relabel the type as "SK6812 / WS2814 (RGBW)" (web
      only), or add a `ws2814` firmware type that behaves like `sk6812` and
      defaults to `wrgb`.
- [ ] **M-3 Bench test.** Flash from the web installer without erasing, so the
      Apple Home pairing survives. Check red, green, blue and white, dimming,
      and all 100 pixels lighting evenly.
- [ ] **M-4 Offer upstream.** Open a PR or issue on `maui1911/TLED` with the
      color-order change.

## Phase 2: features

- [ ] **M-5 Split colors (segments).** Divide the strip into segments, each
      with its own color. Likely approach: one Matter light endpoint per
      segment, which Apple Home shows as separate lights that can be grouped.
      *Needs research:* the endpoint count, NVS layout, and how re-pairing
      behaves when segments change.
- [ ] **M-6 Animations.** The firmware already has rainbow, breathing,
      candle and chase, but they cannot be reached from a controller.
      Matter has no "effect" control that Apple Home shows. *Needs research:*
      candidate triggers are extra on/off endpoints (one per effect), Matter
      Scenes, or a serial/BLE command.
- [ ] **M-7 Firmware updates without USB (OTA).** The partition table already
      has two OTA slots and the Matter OTA requestor is enabled. Upstream
      reports that Matter OTA crashes partway through the download on the
      ESP32-C6 (`docs/ota-update-guide.md`). *Needs research:* whether a newer
      esp-matter fixes that crash, and what can act as the OTA provider for
      a non-certified device (Apple Home is not expected to). The fallback is
      BLE OTA from a browser page.

## Housekeeping

- [x] **H-1 Rename to MoTLED.** Rename the GitHub repo and the local folder.
      Firmware identifiers (`tled` binary name, Matter device name "TLED") stay
      unchanged until Phase 1 is upstreamed, which keeps those diffs small.
- [ ] **H-2 CI build + web flasher.** GitHub Actions builds the firmware in the
      `espressif/esp-matter` Docker image on every push. Pushes to `main`
      publish the web installer, with that firmware, to
      <https://c3474.github.io/MoTLED/>. No local toolchain and no committed
      binaries. Flash from Chrome or Edge and leave "Erase device" unticked to
      keep the Apple Home pairing.
- [ ] **H-3 Rebrand and restyle the web installer.** Give it its own look
      (colors, fonts, layout, logo) so it doesn't copy the original TLED design.
      Change its title and links to MoTLED and credit upstream TLED.
