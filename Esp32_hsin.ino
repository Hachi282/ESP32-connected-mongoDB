#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <WiFi.h>
// #include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// SSID and Password
const char *ssid = "your wifi name";
const char *password = "your wifi password";
// int port = 80;
// WebServer server(port);

// WiFiClient client;
WiFiClientSecure client;
HTTPClient http;

// Read interval
unsigned long previousMillis = 0;
const long readInterval = 600000;

const int led = LED_BUILTIN;

// DHT11 things
#include "DHT.h"
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Other things
float asoilmoist = analogRead(32); // global variable to store exponential smoothed soil moisture reading

// Size of the JSON document. Use the ArduinoJSON JSONAssistant
const int JSON_DOC_SIZE = 384;

void handleRoot()
{
    digitalWrite(led, 1);
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float temp = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(temp))
    {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);

    // Send DHT22 sensor readings using HTTP POST
    sendDHT22Readings(temp, hum);
}

// Send DHT22 readings using HTTP PUT
void sendDHT22Readings(float temp, float hum)
{
    // Constructing the API endpoint with credentials
    const char *rest_api_url = "your api";
    Serial.print("API Endpoint: ");
    Serial.println(rest_api_url);

    // Prepare our JSON data
    String jsondata = "";
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    // 將 DHT22 讀數的溫度、濕度、當前時間和土壤濕度添加到 JSON 文檔中
    doc["temperature"] = temp;
    doc["humidity"] = hum;

    doc["asoilmoist"] = asoilmoist; // 將土壤濕度添加到 JSON 文檔中

    doc["sensor_sn"] = "SR01";

    serializeJson(doc, jsondata);
    Serial.println("JSON Data:");
    Serial.println(jsondata);

    client.setInsecure();
    http.begin(client, rest_api_url);
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");

    // Send the POST request
    int httpResponseCode = http.POST(jsondata);
    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response Code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.println("Response:");
        Serial.println(response);
    }
    else
    {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}

void connectToWiFi()
{
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    int attemptCount = 0;
    while (WiFi.status() != WL_CONNECTED && attemptCount < 20)
    {
        delay(500);
        Serial.print(".");
        attemptCount++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nFailed to connect to WiFi. Please check your credentials.");
    }
}

void setup()
{
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
    Serial.begin(115200);
    delay(500);
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);
    connectToWiFi();
    delay(500);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    dht.begin();
    delay(2000);
}

void loop()
{
    // 定期檢查 WiFi 連接狀態，並在需要時重新連接
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection lost. Reconnecting...");
        connectToWiFi();
    }

    asoilmoist = 0.95 * asoilmoist + 0.05 * analogRead(32); // exponential smoothing of soil moisture
    handleRoot();
    delay(5000);
}
