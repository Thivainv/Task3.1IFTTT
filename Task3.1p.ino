#include <Wire.h>
#include <BH1750.h>            // Christopher Laws’s BH1750 library
#include <WiFiNINA.h>

// ─── WIFI & IFTTT SETUP ───────────────────────────────────────────────────────
const char* WIFI_SSID   = "SITHUM's S24 FE";
const char* WIFI_PASS   = "12345678";

const char* IFTTT_HOST  = "maker.ifttt.com";
const String IFTTT_KEY  = "bXm4pbWwfJ-3Sgxd4oPMjK";        // from https://ifttt.com/maker_webhooks/settings
const String EVENT_ON   = "sunlight_started";     // match your Applet’s event name
const String EVENT_OFF  = "Sent me an email";

// ─── LIGHT SENSOR SETUP ───────────────────────────────────────────────────────
BH1750 lightMeter;           // creates the sensor object
const float LUX_THRESHOLD = 100.0;   // adjust after calibration
bool inSunlight = false;

// ─── SETUP ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // start I²C and BH1750
  Wire.begin();
  if (! lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 init failed");
    while (1);
  }
  Serial.println("BH1750 initialized");

  // connect Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
}

// ─── MAIN LOOP ────────────────────────────────────────────────────────────────
void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Ambient Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  // detect sunrise → sunset transitions
  if (!inSunlight && lux >= LUX_THRESHOLD) {
    triggerIFTTT(EVENT_ON);
    inSunlight = true;
  }
  else if (inSunlight && lux < LUX_THRESHOLD) {
    triggerIFTTT(EVENT_OFF);
    inSunlight = false;
  }

  delay(1000);  // sample every 1 s
}

// ─── IFTTT TRIGGER FUNCTION ─────────────────────────────────────────────────
void triggerIFTTT(const String &eventName) {
  WiFiClient client;
  if (!client.connect(IFTTT_HOST, 80)) {
    Serial.println("IFTTT connection failed");
    return;
  }

  // build and send GET request
  String url = "/trigger/" + eventName + "/with/key/" + IFTTT_KEY;
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + IFTTT_HOST + "\r\n" +
               "Connection: close\r\n\r\n");

  // read & discard response headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }
  client.stop();

  Serial.println("IFTTT event sent: " + eventName);
}
