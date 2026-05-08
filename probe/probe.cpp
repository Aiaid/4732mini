// HiWin 4732 Pro hardware probe — I2C bus scan with SI4732 reset cycling.
//
// Background: prior probes confirmed BL=GPIO38, PIN_POWER_ON=GPIO15 and
// TFT_eSPI Setup206 drives the LCD fine. The main sketch's "black screen"
// was traced to setup() blocking at while(1) when getDeviceI2CAddress()
// can't find the SI4735/SI4732 — and at that point the backlight is still
// gated LOW, so the on-screen "Si4735 not detected" red text is invisible.
//
// This probe forces BL ON (so we can see the screen), tries the standard
// SI4735 reset sequence on candidate GPIOs, and scans I2C for any responder.
// On a healthy sunnygold-spec board it should print:
//   reset_pin=16  ->  found device at 0x11  (or 0x63)
//
// I2C addresses to expect:
//   0x11 — SI4735 / SI4732 with SEN tied LOW
//   0x63 — SI4735 / SI4732 with SEN tied HIGH

#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>

static const int PIN_POWER_ON = 15;
static const int PIN_LCD_BL   = 38;
static const int I2C_SDA      = 18;
static const int I2C_SCL      = 17;

// RESET_PIN candidates — sunnygold uses 16, but HiWin may have remapped it.
static const int reset_candidates[] = {16, 5, 11, 12, 13, 14};
static const size_t N = sizeof(reset_candidates) / sizeof(reset_candidates[0]);

TFT_eSPI tft;

static void scanI2C(int line_y) {
  tft.fillRect(0, line_y, 320, 60, TFT_BLACK);
  tft.setCursor(0, line_y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  I2C device at 0x%02X\n", addr);
      tft.printf("0x%02X ", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("  (no I2C devices)");
    tft.print("(none)");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);  // backlight ON so we can see output

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("I2C SCAN PROBE");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  Serial.println();
  Serial.println("=== HiWin 4732 Pro: I2C scan probe ===");
  Serial.printf("SDA=GPIO%d  SCL=GPIO%d  power_on=GPIO%d\n",
                I2C_SDA, I2C_SCL, PIN_POWER_ON);

  // First scan with no reset asserted at all (whatever the chip's current state).
  delay(200);
  Serial.println("Initial scan (no reset cycle):");
  tft.setCursor(0, 30);
  tft.println("no-reset:");
  scanI2C(50);
  delay(2000);

  // Now try each reset candidate: pulse it LOW for 10ms, release HIGH, scan.
  for (size_t i = 0; i < N; i++) {
    int rpin = reset_candidates[i];
    Serial.printf("Trying reset on GPIO%d:\n", rpin);

    pinMode(rpin, OUTPUT);
    digitalWrite(rpin, LOW);
    delay(10);
    digitalWrite(rpin, HIGH);
    delay(50);

    tft.fillRect(0, 30, 320, 90, TFT_BLACK);
    tft.setCursor(0, 30);
    tft.printf("reset=GPIO%d:\n", rpin);
    scanI2C(50);
    delay(2500);
  }
}

void loop() {
  // Re-scan once a second to detect any device that might come/go.
  Serial.println("loop scan:");
  tft.fillRect(0, 100, 320, 30, TFT_BLACK);
  tft.setCursor(0, 100);
  tft.setTextSize(2);
  tft.print("now: ");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X\n", addr);
      tft.printf("0x%02X ", addr);
    }
  }
  Serial.println();
  delay(1000);
}
