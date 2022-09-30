#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <WiFiManager.h>


char* url = "https://query2.finance.yahoo.com/v8/finance/chart/";
String symbol[14] = { "VEDL", "SBIN", "ALOKINDS", "TRIDENT", "TATACHEM", "TATAPOWER", "STLTECH", "DEEPAKNTR", "RELIANCE", "INFY", "IEX", "UNOMINDA", "IRCTC", "CDSL" };
float price;
float prevPrice;

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting...");

  WiFiManager wifiManager;
  bool state;
  state = wifiManager.autoConnect("StonksDisplay");

  if (!state) {
    Serial.println("Failed to connect, Restarting...");
    delay(500);
    ESP.restart();
  } else {
    Serial.println("Connected");
  }
}


void loop() {
  for (int i = 0; i < 14; i++) {

    price = 0;
    fetch(i);
    Serial.print(symbol[i]);
    Serial.print(": ");
    Serial.println(price);
    Serial.println(prevPrice);
    Serial.println(changePer(prevPrice, price));
Serial.print("Free Heap: ");
Serial.println(ESP.getFreeHeap());
    delay(5000);
  
  }
}
void fetch(const int x) {

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
    delay(50);
    int httpCode = https.GET();
    delay(50);
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
