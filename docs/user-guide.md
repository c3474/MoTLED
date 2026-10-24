# MoTLED User Guide

MoTLED turns an ESP32-C6 board and an addressable LED strip into a smart light
for Apple Home (and other Matter apps such as Home Assistant and Google Home).
It talks to your home over **Thread**, the same low-power network HomePods and
Apple TVs already run, so it needs no Wi-Fi and no cloud account.

This guide is for everyday users. You don't need to know any programming.

- [What you need](#what-you-need)
- [1. Flash the firmware](#1-flash-the-firmware)
- [2. Set up your strip](#2-set-up-your-strip)
- [3. Add it to Apple Home](#3-add-it-to-apple-home)
- [4. Your lights in the Home app](#4-your-lights-in-the-home-app)
- [5. Color patterns (Color 1-4)](#5-color-patterns-color-1-4)
- [6. Effects (Rainbow, Chase, Twinkle)](#6-effects-rainbow-chase-twinkle)
- [7. Themes with Home scenes](#7-themes-with-home-scenes)
- [8. Turning features on or off (re-pairing)](#8-turning-features-on-or-off-re-pairing)
- [9. Updating the firmware](#9-updating-the-firmware)
- [10. Troubleshooting](#10-troubleshooting)

---

## What you need

| Item | Notes |
|---|---|
| **ESP32-C6 board** | For example a Seeed XIAO ESP32C6 or DFRobot Beetle ESP32-C6. |
| **Addressable LED strip** | WS2812B, WS2811, SK6812, WS2814 (RGBW) or WS2805 (RGB + warm/cool white). The strip has a **data** wire, not just + and −. |
| **Power supply** | Matches the strip's voltage (5 V, 12 V or 24 V) with enough watts for its length. A small 5 V converter can power the board from a 12/24 V supply. |
| **Thread border router** | A HomePod mini, HomePod (2nd gen) or Apple TV 4K that supports Thread. |
| **A computer with Chrome or Edge** | For flashing and setup. Safari can't talk to USB devices. |

**Wiring:** connect the strip's data wire to the board's data pin, and connect
the **ground (GND) of the board, the strip and the power supply together**. A
missing shared ground is the most common cause of flickering or wrong colors.
When the board is plugged into your computer's USB, disconnect its 5 V power
lead but **keep the ground connected**.

---

## 1. Flash the firmware

1. Open **<https://c3474.github.io/MoTLED/>** in Chrome or Edge.
2. Plug the board into your computer with a USB cable that carries data.
3. Click **Install MoTLED** and pick the board from the list.
   - If it doesn't show up, put the board in download mode: hold **BOOT**, tap
     **RESET**, release **BOOT**, then try again.
4. **"Erase device"**: tick it only for a brand-new board. On a board that's
   already set up, **leave it unticked**. Erasing wipes your settings and
   removes the light from Apple Home.

---

## 2. Set up your strip

1. Switch to the **Configure** tab and click **Connect**, then pick the board.
2. The form fills in with the board's current settings, and the board's
   firmware version appears under the connection status. The **Save** buttons
   unlock once the settings have loaded.
3. Fill in:

| Setting | What to enter |
|---|---|
| **Number of LEDs** | The number of *pixels* (LED chips) on the strip. On strips where one chip drives a group of LEDs (common on 12 V/24 V strips), count chips, not LEDs: a 5 m strip with 20 chips per meter has 100. |
| **GPIO Pin** | The board pin your data wire is soldered to (the GPIO number, not the printed label: D9 on a XIAO ESP32C6 is GPIO 20). |
| **LED Type** | Your strip's chip. For **WS2814** choose **SK6812 / WS2814 (RGBW)**. |
| **Color Order** | See [Finding the color order](#finding-the-color-order) below. |
| **Max Brightness** | A cap (1-255) to limit power draw and heat. Keep it low enough that your power supply isn't overloaded. |
| **Device Name** | One word, no spaces. Only used while pairing; you name the light in the Home app. |
| **After a Power Cut** | Restore the last state, always turn on, or always stay off. |
| **Color pattern lights** | Adds the Color 1-4 lights to Home ([section 5](#5-color-patterns-color-1-4)). Leave off for one simple light. |
| **Effect switches** | Adds Rainbow, Chase and Twinkle to Home ([section 6](#6-effects-rainbow-chase-twinkle)). Leave off for one simple light. |
| **White only** | **WS2805 strips only.** The light shows in Home as a tunable white light: brightness plus warm-to-cool white (3000-6500 K), no color wheel. Good for kitchens and under-cabinet lights. Turns the color and effect extras off. |
| **Color Pattern Layout / Band Size** | How Color 1-4 share the strip: alternating bands of N pixels (like classic holiday bulbs), or equal blocks (halves, thirds, quarters). |

4. Click **Save and reboot**. The board restarts and the page reconnects by itself.

### Finding the color order

If red shows as green (or similar), the color order is wrong. For RGBW strips:

1. Set **Color Order** to **GRB**, Save and reboot, and add the light to Home.
2. In Home, set the light to **green**, then **red**, then **blue**, then
   **white**, and note what the strip actually shows each time.
3. Whatever shows for green is the strip's 1st color, red its 2nd, blue its
   3rd, and white its 4th. Example: green→white, red→red, blue→green,
   white→blue means the order is **WRGB** (common on WS2814 strips).

---

## 3. Add it to Apple Home

1. On the **Configure** tab, while connected, a **QR code** appears whenever
   the board isn't paired yet (after a reboot the page reconnects by itself;
   otherwise click **Connect**).
2. In the Home app: **+ → Add Accessory**, and scan the QR code (or tap **More
   options** and type the number shown under it).
3. Choose a room and a name, for example "Porch".

---

## 4. Your lights in the Home app

What you see depends on the two feature checkboxes:

| Features on | Tiles in Home |
|---|---|
| Neither | **One light**: on/off, brightness, color. That's all. |
| White only (WS2805) | **One white light**: on/off, brightness and white temperature. |
| Color pattern lights | The main light **plus 4 Color lights**. |
| Effect switches | The main light **plus 3 on/off switches** (Rainbow, Chase, Twinkle). |

**Home names the extra tiles "Light 2", "Light 3" and so on, in no
particular order. Rename them first**, or nothing else will make sense:

1. Press and hold a tile, then tap the **gear** (settings) and edit the name.
2. To tell them apart, turn each one on by itself:
   - **The main light** is the one that turns the whole strip on and off with
     your normal color. Name it after the place, for example "Porch".
   - **Color lights** start out red, green, blue and white (Color 1-4).
     Name them "Porch Color 1" to "Porch Color 4".
   - **Effect switches**: one fills the strip with a rainbow (Rainbow), one
     makes the colors move (Chase), one makes pixels sparkle (Twinkle).

**Tips:**
- **One tile or separate tiles:** in the accessory's settings, **Show as
  Separate Tiles** controls whether all the controls sit in one tile.
- **Favorite colors:** in a color picker, edit the row of swatches to hold your
  go-to colors (orange, purple, red, green, warm white…). Picking a color then
  takes one tap.
- **A separate room for the extras:** put the Color lights and effect switches
  in their own room (for example "Porch extras"), so "Hey Siri, turn on the
  porch lights" only turns on the main light instead of every color and effect.

---

## 5. Color patterns (Color 1-4)

Use these to show 2-4 colors at once, for example red and green for Christmas.

**The rules:**

- **Turn on the Color lights you want.** One on means the whole strip is that
  color; two on means a two-color pattern, and so on, in the order Color 1, 2,
  3, 4.
- **All Color lights off** means the strip goes back to the main light's
  single color.
- **The main light is the master.** Its power turns everything on or off, and
  its **brightness slider dims everything**.
- **A Color light's own brightness slider does nothing.** Apple always shows
  it, but MoTLED ignores it: any position above zero means "on, full color".
- **Turning on a Color light turns the main light on** if it was off.
- **While a pattern is showing, the main light's color picker has no visible
  effect.** Turn the Color lights off to go back to one color.

**Layout** (on the web page's Configure tab): *Alternating bands* repeats the
colors every few pixels (Band Size, default 3), like classic holiday bulbs.
*Blocks* splits the strip into halves, thirds or quarters.

---

## 6. Effects (Rainbow, Chase, Twinkle)

The effect switches add motion to whatever is showing. They can be combined.

| Switch | What it does |
|---|---|
| **Rainbow** | Fills the strip with a full rainbow (replaces the Color lights while on). |
| **Chase** | Makes the colors march along the strip. With a single color, it chases color and gaps. |
| **Twinkle** | Random pixels sparkle and fade; the rest of the strip dims slightly so the sparkles stand out. |

Combinations: **Rainbow + Chase** is a moving rainbow, **Color lights + Chase**
moves your pattern, **Chase + Twinkle** does both.

Like the Color lights, the main light's slider dims effects, and turning on an
effect switch turns the main light on.

---

## 7. Themes with Home scenes

Scenes are the easiest way to use MoTLED day to day: set them up once, then
use them from Siri, Control Center, Lock Screen widgets or automations.

In Home: **+ → Add Scene → Custom**, then add the main light, all Color lights
and all effect switches, and set each one. **Set the ones you don't want to
"off" explicitly**, so the scene always gives the same result.

| Scene | Main light | Color 1 | Color 2 | Color 3 | Color 4 | Rainbow | Chase | Twinkle |
|---|---|---|---|---|---|---|---|---|
| Normal | on, warm white | off | off | off | off | off | off | off |
| Christmas | on | red | green | off | off | off | on | off |
| Halloween | on | orange | purple | off | off | off | on | on |
| July 4th | on | red | white | blue | off | off | off | on |
| Party | on | off | off | off | off | on | on | off |

Ideas: an automation that runs "Halloween" at sunset from Oct 25-31, or a
Home Screen icon (Shortcuts app → Run scene).

---

## 8. Turning features on or off (re-pairing)

Ticking or unticking **Color pattern lights**, **Effect switches** or **White only** changes
which tiles the light shows in Home. Apple Home doesn't pick that up on its
own, so the light must be removed and added again:

1. Change the checkbox on the **Configure** tab and click **Save and reboot**.
2. **With the board powered on**, remove the light in Home (press and hold →
   gear → **Remove Accessory**). Removing it while the board is online lets the
   board know it's free to pair again.
3. Add it again with the QR code ([section 3](#3-add-it-to-apple-home)).
   - **No QR code?** Your iPhone keeps a pairing of its own that **Remove
     Accessory** doesn't clear. Either remove the light under iPhone **Settings
     → General → Matter Accessories**, or use the Configure tab's buttons:
     **Show pairing code** opens pairing for 15 minutes even though the board
     is still paired; **Forget pairings** clears every pairing on the board
     (LED settings are kept) and restarts ready to pair.
4. Rename the tiles and rebuild your scenes. They were deleted with the old
   accessory.

**Tip:** decide which features you want first, so you only re-pair once.

---

## 9. Updating the firmware

When you connect on the **Configure** tab, the page compares your board's
firmware with the version it installs. If they differ, it shows **Firmware
update available**, with a button that takes you to the Flash tab.

1. **Hard-refresh the web page** first (Cmd+Shift+R on a Mac, Ctrl+Shift+R on
   Windows) so you get the newest version. The version badge next to the logo
   shows which firmware the page will install.
2. Click **Install MoTLED** and **leave "Erase device" unticked**. Your
   settings, pairing and scenes are kept.
3. Connect on the **Configure** tab and check that **Device firmware** matches
   the badge.

Updates never require re-pairing unless the release notes say they do.

---

## 10. Troubleshooting

| Problem | Fix |
|---|---|
| **Colors are wrong** (red shows green, white shows blue…) | The color order is wrong; see [Finding the color order](#finding-the-color-order). |
| **Flickering, random colors, or only the first few pixels work** | Check the shared ground between board, strip and power supply. Check that Number of LEDs and LED Type are right. |
| **The far end is dim or yellowish** | Long strips lose voltage. Feed power into both ends (power injection), or lower Max Brightness. |
| **The Configure form shows wrong values (10 LEDs, GPIO 5…)** | That's the page's placeholder, not your board. Hard-refresh the page and click Connect again. The Save buttons stay locked until the board's real settings load, so you can't save placeholders by accident. |
| **The board isn't listed when connecting or installing** | Use a USB data cable (not charge-only), use Chrome or Edge, close other tabs or apps using the board, or use download mode (hold BOOT, tap RESET, release BOOT). |
| **Turning on a Color light or effect seems to do nothing** | Make sure you're using the right tiles: rename them ([section 4](#4-your-lights-in-the-home-app)). The pattern only shows while the main light is on; turning on a Color light or effect turns it on automatically. |
| **New tiles didn't appear after ticking a feature** | Re-pair ([section 8](#8-turning-features-on-or-off-re-pairing)). |
| **Home says "No Response"** | Check the board has power, and that a Thread border router (HomePod mini or Apple TV) is on and nearby. It can take a minute to reconnect after a reboot. |
| **No QR code on the Configure tab** | The board is still paired: the Serial Console says "Already paired". Click **Show pairing code** to pair anyway (open for 15 minutes). |
| **Can't add it to Home again after removing it** | The board still holds a pairing: your iPhone keeps one of its own (**Settings → General → Matter Accessories**), or it was removed while offline. Remove it there, or click **Forget pairings** on the Configure tab: it clears every pairing, **keeps your LED settings**, and restarts ready to pair. |
| **Start completely fresh** | In the Serial Console, run `factory`. This clears pairings **and** all LED settings. |
| **Lights come on after a power cut when you don't want them to** | Change **After a Power Cut** on the Configure tab. |

For advanced users, the **Docs** tab on the web page lists every serial
console command, including test commands such as `pattern red green` and
`motion chase twinkle`.
