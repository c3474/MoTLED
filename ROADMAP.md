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
- [x] **M-2b WS2814 guidance.** Make it obvious which type and order to pick
      for a WS2814. The type reads "SK6812 / WS2814 (RGBW)" on the web
      page, with a hint pointing to the white-first orders. No firmware type:
      a WS2814 only differs from an SK6812 in byte order, which is a setting.
- [x] **M-3 Bench test.** Flash from the web installer without erasing, so the
      Apple Home pairing survives. Check red, green, blue and white, dimming,
      and all 100 pixels lighting evenly.
- [x] **M-3b Low-end dimming.** At 1% in Apple Home the strip looks off;
      2% is a nice dim glow. Likely the brightness scaling rounds the lowest
      levels to 0. Make sure "on" always lights at least the dimmest step.
- [ ] **M-4 Offer upstream.** Open a PR or issue on `maui1911/TLED` with the
      color-order change.

## Phase 2: features

Priorities come from what permanent-lighting products (Govee, Trimlight,
JellyFish) and WLED put front and center: repeating multi-color patterns
first, then motion on top of them. No hard usage statistics were found.

- [ ] **M-5 Color patterns (top priority).** 2-4 colors along the strip,
      alternating every N pixels (default 3) or split into equal blocks.
  - [x] Step 1: pattern engine + serial `pattern` command; tested on the strip.
  - [x] Step 2: "Color 1-4" lights in Apple Home. The colors that are switched
        on make up the pattern; all off returns to the main light's color.
        Main light keeps power and brightness. Layout and band size become
        settings on the web page. Home scenes give one-tap themes. Requires
        one re-pair.
  - [x] Step 3a: one brightness control. Color lights are color on/off only
        (their sliders are ignored), the main light's brightness dims every
        mode, and turning on a Color light turns the main light on.
  - [ ] Step 3: Home app usability. Apple's light screen always shows a
        brightness slider plus color picker, which feels clunky for picking
        pattern colors. Trying first: grouped tile, favorite color swatches,
        and Home scenes per theme. Fallbacks: a third-party HomeKit dashboard
        app, or fewer Color lights.
- [x] **M-8 Optional features.** One firmware for every strip: checkboxes on
      the web Configure tab decide which extra controls Apple Home shows.
      "Color pattern lights" (Color 1-4) is off by default, so a new device
      is one simple light. Devices upgrading from the first Color 1-4 release
      keep them. Changing a checkbox needs a re-pair.
  - [ ] "Effect switches" checkbox, added with M-6.
- [ ] **M-9 White-only mode for WS2805 (kitchen).** Plan: a WS2805 RGBCCT
      strip (already supported, with a color temperature slider). Add a
      "white only" option so Home shows just brightness and color
      temperature, with no color wheel. Plain warm/cool-only addressable
      strips (3-channel chips) would need a new strip type; not planned yet.
- [ ] **M-6 Animations.** Effects ranked by usefulness for porch/eave lights:
  - [x] Step 1: serial `effect` command; all four upstream effects work.
  - [ ] **Chase for patterns:** the color pattern slides along the strip;
        covers "rainbow chase" and moving holiday patterns.
  - [ ] **Twinkle:** random LEDs sparkle over the pattern or a base color.
  - [ ] **Smooth breathing/fades:** breathing steps through only ~100
        brightness levels with no gamma correction, so it looks stuttery,
        especially when dim. Compute at full 0-255 resolution with a gamma
        curve.
  - [ ] Expose Chase and Twinkle in Apple Home as on/off switches (small
        tiles, no sliders), behind an "Effect switches" checkbox; rainbow is
        a pattern rather than its own switch.
  - Lower priority: single-dot chase, candle (possible Halloween use).
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
- [x] **H-4 Configure tab reads the device.** Fixed upstream's parser (it
      never matched lines ending in \r\n), Save stays disabled until the
      device's settings are loaded, added device name and power-on fields,
      and the page reconnects after a reboot.
- [ ] **H-3 Rebrand and restyle the web installer.** Give it its own look
      (colors, fonts, layout, logo) so it doesn't copy the original TLED design.
      Change its title and links to MoTLED and credit upstream TLED.
