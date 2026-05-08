# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository overview

This repo is firmware for an SI4732/SI4735-based all-band (FM / MW / SW / LW / SSB) radio receiver built on a **Lilygo T-Display-S3** (ESP32-S3 + ST7789 320x170 TFT). The whole receiver UI, menu system, EEPROM persistence, battery monitoring and rotary-encoder input live in one Arduino sketch: `ALL_IN_ONE_T-Display_S3.ino` (~2k lines).

The repository also contains an unrelated standalone HTML app `勾平叉.html` (a "勾平叉" scorekeeper web page) that ships through GitHub Pages — `CNAME` points the site at `gpc.srem.cn`. It has no build step and no relationship to the firmware.

`README.md` only states: stable open-source release for study/non-commercial use, contributions welcome.

## Build / flash

There is no Makefile, PlatformIO config, or CI. The sketch is meant to be opened in the **Arduino IDE** with these settings:

- Board: **ESP32S3 Dev Module** (or "LilyGo T-Display-S3").
- USB CDC On Boot: **Enabled**.
- Flash size: 16MB; PSRAM: OPI PSRAM (typical for T-Display-S3).
- All required libraries are vendored under `libraries/` — copy that directory into `~/Documents/Arduino/libraries/` (or symlink it) before compiling. Bundled libs include a customized `TFT_eSPI_DynamicSpeed` (must be used in place of stock TFT_eSPI — pin map and config are tuned for T-Display-S3), `PU2CLR_SI4735`, `Battery18650Stats`, `ESP32Time`, `GFX_Library_for_Arduino`, `Adafruit_BusIO`, `Adafruit_MPR121`.
- EEPROM_SIZE is 2048; the sketch verifies `app_id = 47` at offset 0 before trusting persisted state — bump `app_id` whenever the on-flash layout changes or stored settings will be silently misread.

There are no automated tests; verification is manual on hardware.

### Hardware variants and rollback

This Aiaid fork was opened against a **HiWin 4732 Pro** unit — a derivative of sunnygold's open-hardware 4732mini. That unit ships with a private zooc firmware **v3.4.0** that adds WiFi, internet-radio streaming (ESP32-audioI2S), and a web config / OTA panel — none of which are in this repo's source. Flashing this repo onto a HiWin board is a feature **downgrade**, not an upgrade. Always dump the factory image first.

Dump the factory firmware before flashing anything custom:

```bash
# Find port (usually /dev/cu.usbmodem* on macOS, after BOOT+RESET into download mode)
ls /dev/cu.usbmodem*

# Full 16MB flash dump (takes ~3 min at 921600 baud)
~/Library/Python/3.9/bin/esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodemNNNNNNN --baud 921600 \
  read_flash 0 0x1000000 hiwin_4732_factory.bin
```

Roll back to the factory image after a failed experimental flash:

```bash
~/Library/Python/3.9/bin/esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodemNNNNNNN --baud 921600 \
  write_flash 0 hiwin_4732_factory.bin
```

`*.bin` is gitignored. The dump contains NVS regions including WiFi credentials, so never commit or share it.

Failure modes after flashing this fork's sketch on a 4732mini-derivative board are non-destructive and diagnose by symptom: black screen → `PIN_LCD_BL` (38) or `PIN_POWER_ON` (15) wrong; no audio → `AMP_EN` (10) / `AUDIO_MUTE` (3) / I2C (18/17) wrong; dead encoder → encoder pins (2/1/21) wrong. The board can always be re-entered into download mode (BOOT held + RESET tapped) and rolled back.

## Firmware architecture

The sketch is procedural Arduino — `setup()` initializes hardware, `loop()` is a single dispatch loop. Understanding these cross-cutting structures matters more than reading top-to-bottom:

- **Mode/menu state machine**: a family of `bool cmd*` flags (`cmdBand`, `cmdVolume`, `cmdAgc`, `cmdBandwidth`, `cmdStep`, `cmdMode`, `cmdMenu`, `cmdSoftMuteMaxAtt`, `cmdbright`, `cmdselstation`, `cmdsave`, `cmdseek`, `cmdautolcdoff`, `cmdabout`) plus `menuIdx` define what the encoder rotation currently controls. `disableCommands()` clears them all; `isMenuMode()` tests the group. The menu order (`menu[]` near line 143) is index-coupled to the `#define VOLUME 0 … autolcdoff 13` constants — keep them in sync.

- **Input model**: a single push button (GPIO21) is decoded by `getButtonEvent()` (line ~402) into events 1=single, 2=double, 3=triple, 4=quad, 5=short-long, 6=long-press. The rotary encoder uses an ISR (`rotaryEncoder` attached in `setup()`) that updates `volatile int encoderCount`. `loop()` consumes `event` and `encoderCount` and routes them through the `cmd*` flags.

- **Band table**: `Band band[]` (line ~225) is the source of truth for tuning — name, type (FM/MW/SW/LW), min/max, default freq, step index, bandwidth index. `useBand()` applies a row to the SI473x. `bandwidthFM/AM/SSB[]` and `tabFmStep/tabAmStep/tabSwStep[]` are parallel lookup tables indexed by per-mode `bwIdx*` / `idx*Step` variables.

- **SSB**: SSB requires uploading `ssb_patch_content` from `patch_init.h` to the SI473x at runtime (`loadSSB()`); `ssbLoaded` gates this so the ~16KB patch is only re-sent when needed. SSB modes (LSB/USB) disable seek and enable BFO control via the encoder when `bfoOn` is true.

- **EEPROM layout** (see `saveAllReceiverInformation()` / `readAllReceiverInformation()` near lines 588/624):
  - `+0` app_id sentinel, then volume/band/mode/bandwidth/AGC/softmute/brightness/lcdofftime in low bytes;
  - `+10..+11` battery `CONV_FACTOR * 1000` (high/low byte) — calibrated by event 4 (quad-click) which scales the factor so a measured USB voltage maps to 4.20V;
  - `+12 onward` (or similar) holds the 50-slot `station[]` preset array. When changing any of these offsets, also bump `app_id`.

- **Display pipeline**: drawing uses a single 320x170 `TFT_eSprite spr` as a back-buffer (`drawSprite()` is the canonical "render whole screen" call). Custom font headers (`HanYiZhongYuanJian16.h` for Chinese, `orbitronbold16.h` / `FREQFONT.h` / `NotoMono16.h` / `EnvyCodeR16.h` for digits/labels, `icon.h` for bitmap icons) are large generated arrays — do not hand-edit. The menu strings in `menu[]` are Chinese (UTF-8 source); preserve encoding when editing.

- **Power management**: when idle past `lcdofftime` minutes (`checkScreenTimeout()`), `lcdoffed` is set, the loop sends `0x10` (display sleep), drops CPU to 80MHz, configures GPIO1 (encoder pin B) as the ext0 wake source and calls `esp_light_sleep_start()`. On wake it issues `0x11` and returns to 240MHz. Any new long-running work in `loop()` must respect this — don't add code that runs while `lcdoffed` is true unless you've thought about wake latency.

- **Battery**: `Battery.cpp` reads ADC GPIO4, averages 20 samples, applies the calibrated `CONV_FACTOR`, and uses a piecewise table when `useConversionTable` is true. `MIN_USB_VOLTAGE = 4.35` distinguishes "charging" from running on cell. `batteryLevel` is sampled on a timer in `loop()`, not every frame.

## Conventions worth knowing

- Comments and UI strings are mostly Chinese; keep that consistent when adding user-facing text.
- The sketch deliberately avoids `delay()` in `loop()` (only ~5 ms in a couple of places) — use `millis()` deltas like the existing `elapsedRSSI`, `elapsedCommand`, `lastRDSCheck`, `storeTime` pattern rather than blocking waits.
- `itIsTimeToSave` + `STORE_TIME` (10 s) debounces EEPROM writes — set the flag from anywhere a setting changes and let the loop commit. Don't call `saveAllReceiverInformation()` synchronously from a UI handler.
- `TFT_eSPI_DynamicSpeed` under `libraries/` is a fork — pin definitions and `User_Setup` are baked in. Don't replace it with upstream TFT_eSPI.
