/*
 * ESPNowCam Receiver with TFT (Compatible with Arduino Library Manager versions)
 * Uses only basic TJpg_Decoder features that always work.
 poulet
 */

#include <ESPNowCam.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

#define MAX_FRAME_SIZE 30000

uint8_t frameBuffer[MAX_FRAME_SIZE];
ESPNowCam radio;
TFT_eSPI tft = TFT_eSPI();

// ✅ CORRECT CALLBACK: returns 'bool', not uint16_t
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  tft.pushImage(x, y, w, h, bitmap);
  return true; // Must return true to continue decoding
}

void onDataReady(uint32_t length) {
  // Optional: clear screen
  // tft.fillScreen(TFT_BLACK);

  // Set callback
  TJpgDec.setCallback(tft_output);

  // Draw JPEG at top-left (0,0)
  // This works in ALL versions of TJpg_Decoder
  TJpgDec.drawJpg(0, 0, frameBuffer, length);
}

void setup() {
  Serial.begin(115200);
  Serial.println("📡 ESPNowCam + TFT Receiver (Simple Mode)");

  tft.init();
  tft.setRotation(1); // Adjust as needed
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Waiting for image...");

  radio.setRecvBuffer(frameBuffer);
  radio.setRecvCallback(onDataReady);
  radio.init();

  Serial.println("Ready!");
}

void loop() {
  delay(10);
}