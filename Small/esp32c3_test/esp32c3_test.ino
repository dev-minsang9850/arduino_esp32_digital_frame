#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting TFT Test...");
  
  tft.init();
  tft.setRotation(0);
}

void loop() {
  Serial.println("Filling RED");
  tft.fillScreen(TFT_RED);
  delay(1000);
  
  Serial.println("Filling GREEN");
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  
  Serial.println("Filling BLUE");
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  
  Serial.println("Filling BLACK with WHITE text");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Hello World!", 10, 64, 2);
  delay(2000);
}
