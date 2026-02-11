#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(); // TFT instance

#define TFT_BL 3  // Backlight pin

void setup() {
  tft.init();
  tft.setRotation(0); // adjust if display is rotated
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // turn on backlight

  // Fill display with black first
  tft.fillScreen(TFT_BLACK);

  // Draw a rectangle in the center
  int rectSize = 100;
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;

  // Draw filled rectangle
  tft.fillRect(centerX - rectSize/2, centerY - rectSize/2, rectSize, rectSize, TFT_RED);

  // Draw rectangle border
  tft.drawRect(centerX - rectSize/2, centerY - rectSize/2, rectSize, rectSize, TFT_WHITE);
}

void loop() {
  // Change background colors to check display
  tft.fillScreen(TFT_BLUE);
  delay(500);
  tft.fillScreen(TFT_GREEN);
  delay(500);
  tft.fillScreen(TFT_YELLOW);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  delay(500);
}
