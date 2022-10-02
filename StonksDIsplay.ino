#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RED 14
#define GREEN 12
#define BLUE 15

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char* url = "https://query2.finance.yahoo.com/v8/finance/chart/";
String symbol[14] = { "VEDL", "SBIN", "ALOKINDS", "TRIDENT", "TATACHEM", "TATAPOWER", "STLTECH", "DEEPAKNTR", "RELIANCE", "INFY", "IEX", "UNOMINDA", "IRCTC", "CDSL" };
float price;
float prevPrice;
byte stockNos;
unsigned long previousMillis = 0;
const long period = 10000;

void setup() {

  
  pinMode(RED, HIGH);    // Blue led Pin Connected To D5 Pin
  pinMode(GREEN, HIGH);  // Green Led Pin Connected To D6 Pin
  pinMode(BLUE, HIGH);   // Red Led Connected To D8 Pin

  Serial.begin(115200);
  Serial.println("Connecting...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

 display.clearDisplay();  // boot text
  display.setTextColor(WHITE);
  drawCentreString("THE", 64, 4, 1);
  display.setTextSize(2);
  drawCentreString("Stock", 64, 12, 2);
  drawCentreString("  Stalker", 64, 28, 2);
  display.setCursor(40, 56);
  display.setTextSize(1);
  display.write(3);
  display.print("MYSTIC");
  display.write(3);
  display.display();
  setRGB(255, 0, 0);
  delay(500);
  setRGB(0, 255, 0);
  delay(500);
  setRGB(0, 0, 255);
  delay(500);

  WiFiManager wifiManager;
  wifiManager.autoConnect("StonkDisplay");
  setRGB(0, 0, 0);
  display.clearDisplay();  //connecting to wifi
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  drawCentreString("Connecting to WiFi..", 64, 28, 1);
  display.display();

  
    display.clearDisplay();
    display.setTextColor(WHITE, BLACK);  //connected to wifi
    drawCentreString("Connected\nStarting OTA...", 64, 28, 1);
    display.display();
    Serial.println("Connected");

    ArduinoOTA.setHostname("StockStalker");
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      display.clearDisplay();
      display.setCursor(0, 28);
      display.setTextColor(WHITE, BLACK);
      display.printf("Progress: %u%%\r", (progress / (total / 100)));
      display.display();
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setTextColor(WHITE, BLACK);  //connected to wifi
    drawCentreString("OTA Started :)", 64, 28, 1);
    display.print("\nIP: ");
    display.print(WiFi.localIP());
    display.display();
  
}


void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= period) {
    previousMillis = currentMillis;
    price = 0;
    fetch(stockNos);
    setRGB(0, 0, 255);
    stonkToDisplay(stockNos);
    Serial.print(symbol[stockNos]);
    Serial.print(": ");
    Serial.println(price);
    Serial.println(prevPrice);
    Serial.println(changePer(prevPrice, price));
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    stockNos++;
    Serial.println(stockNos);
    if (stockNos > 14) {
      stockNos = 0;
    }
  }

  ArduinoOTA.handle();
}
void fetch(const byte x) {

  StaticJsonDocument<96> filter;

  JsonObject fltr = filter["chart"]["result"][0].createNestedObject("meta");
  fltr["regularMarketPrice"] = true;
  fltr["previousClose"] = true;

  StaticJsonDocument<192> doc;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String fullurl(url);
  fullurl.reserve(63);
  fullurl += symbol[x];
  fullurl += ".NS";

  while (price == 0) {

    https.begin(client, fullurl);
    int httpCode = https.GET();
    Serial.println("Response code: " + String(httpCode));

    if (httpCode > 0) {

      ReadBufferingStream bufferedStream(https.getStream(), 64);
      DeserializationError error = deserializeJson(doc, bufferedStream, DeserializationOption::Filter(filter));

      if (error) {

        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }
      prevPrice = doc["chart"]["result"][0]["meta"]["previousClose"];
      price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
    }
    https.end();
  }
}
float changePer(float x, float y) {
  float result = (y - x) / x * 100;
  return result;
}
void drawCentreString(const String buf, int x, int y, int z) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(z);
  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);  //calc width of new string
  display.setCursor(x - w / 2, y);
  display.print(buf);
}
void stonkToDisplay(int x) {
  float percentChange = changePer(prevPrice, price);

  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
  drawCentreString(symbol[x], 64, 4, 2);
  display.setCursor(0, 24);
  display.setTextSize(2);
  display.write(125);
  display.printf("Rs.%.1f\r\n", price);
  display.setTextSize(1);
  display.write(16);
  display.printf(" Rs.%.2f ", price - prevPrice);
  if (percentChange > 0) {
    display.write(30);
    display.printf(" %.2f%%\r\n", percentChange);
    setRGB(0, 25, 0);
  } else if (percentChange < 0) {
    display.write(31);
    display.printf(" %.2f%%\r\n", percentChange);
    setRGB(25, 0, 0);
  } else {
    display.write(1);
    display.printf(" %.2f%%\r\n", percentChange);
    setRGB(0, 0, 25);
  }

  display.print("\r\n Frag: ");
  display.print(ESP.getHeapFragmentation());
  display.drawRect(0, 0, 128, 64, WHITE);
  display.drawLine(1, 52, 127, 52, WHITE);
  display.drawLine(1, 19, 127, 19, WHITE);
  display.display();
}
void setRGB(int r, int g, int b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}
