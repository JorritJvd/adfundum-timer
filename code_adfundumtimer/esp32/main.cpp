#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_system.h>
#include "config.h"
#include "secret.h"

// RFID reader instance (I2C)
Adafruit_PN532 nfc(-1, -1, &Wire);

// State machine states
enum State { WAIT_TAG, TAG_DELAY, WAIT_EMPTY, MEASURING, DONE };
State state = WAIT_TAG;

// Timing & LED globals
unsigned long lastPulse    = 0;
int           pulseBright  = 50;
bool          pulseInc     = true;
unsigned long tagTime      = 0;
unsigned long measureStart = 0;
unsigned long doneUntil    = 0;

// Hardware interfaces
WiFiClient      espClient;
PubSubClient    mqttClient(espClient);
Adafruit_7segment display;
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Function prototypes
void   connectWiFi();
bool   connectMQTT();
void   checkReset();
String readUID();
String currentUID;  // bewaar hier de UID van de sessie
bool   glassFull();
bool   glassRemoved();
bool   glassEmpty();
void   showTime(unsigned long ms);
void   sendMQTT(const String& uid, unsigned long t);

void setup() {
  Serial.begin(115200);
  // while (!Serial);

  pinMode(RESET_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize display
  display.begin(0x70);

  // Initialize NeoPixel
  pixels.begin();
  pixels.clear();
  pixels.show();

  // Initialize RFID
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("RFID module niet gevonden");
    while (1);
  }
  nfc.SAMConfig();

  // Connect to WiFi and MQTT once
  connectWiFi();
  connectMQTT();
  Serial.println("Setup compleet");
}

void loop() {
  unsigned long now = millis();
  checkReset();
 
static unsigned long lastDebug = 0;
if (mqttClient.connected()) {
  mqttClient.loop();
}


  switch (state) {

    case WAIT_TAG: {
      // White pulsing effect
      if (now - lastPulse >= 100) {
        lastPulse = now;
        pulseBright += pulseInc ? 20 : -20;
        if (pulseBright >= 255) { pulseBright = 255; pulseInc = false; }
        else if (pulseBright <= 50)  { pulseBright = 50; pulseInc = true;  }
        pixels.fill(pixels.Color(pulseBright, pulseBright, pulseBright));
        pixels.show();
      }
      // Detect full glass
      if (glassFull()) {
        Serial.println("Glas vol gedetecteerd, lees RFID...");
        currentUID = readUID();
        if (currentUID.length()) {
          Serial.print("UID: "); Serial.println(currentUID);
          tagTime = now;
          state = TAG_DELAY;
        }

      }
      break;
    }

    case TAG_DELAY: {
    static unsigned long chaseTimer = 0;
    static int chasePos = 0;
    static unsigned long glassLostStart = 0; // debounce timer

    // Check of het glas nog vol is met 200 ms debounce
    if (!glassFull()) {
      if (glassLostStart == 0) {
        glassLostStart = now;
      } else if (now - glassLostStart >= 200) {
        Serial.println("Glas niet meer vol tijdens countdown, knipper rood en reset.");
        unsigned long blinkStart = now;
        while (millis() - blinkStart < 2000) {
          if ((millis() - blinkStart) % 500 < 250) {
            pixels.fill(pixels.Color(255, 0, 0));
          } else {
            pixels.clear();
          }
          pixels.show();
        }
        display.clear(); display.writeDisplay();
        pixels.clear(); pixels.show();
        state = WAIT_TAG;
        glassLostStart = 0;
        break;
      }
    } else {
      glassLostStart = 0;
    }

    // Blue chase animation + 5s countdown
    if (now - chaseTimer >= 50) {
      chaseTimer = now;
      chasePos = (chasePos + 1) % NUMPIXELS;
      pixels.clear();
      pixels.setPixelColor(chasePos, pixels.Color(0, 0, 255));
      pixels.show();
    }
    unsigned long elapsed = now - tagTime;
    if (elapsed >= 5000) {
      Serial.println("GO");
      display.clear();
      display.writeDigitAscii(1, 'G', false);
      display.writeDigitAscii(3, 'O', false);
      display.writeDisplay();
      pixels.fill(pixels.Color(0, 255, 0));
      pixels.show();
      state = WAIT_EMPTY;
      glassLostStart = 0;
    } else {
      int rem = 5 - elapsed / 1000;
      display.clear();
      display.writeDigitNum(1, rem / 10, false);
      display.writeDigitNum(3, rem % 10, false);
      display.drawColon(true);
      display.writeDisplay();
    }
    break;
}


    case WAIT_EMPTY:
      // Wait until glass is removed (FSR below no-glass)
      if (glassRemoved()) {
        Serial.println("Glas verwijderd, start meten...");
        measureStart = now;
        pixels.fill(pixels.Color(255, 255, 0));
        pixels.show();
        state = MEASURING;
      }
      break;
      
    case MEASURING: {
      // Toon de lopende timer
      showTime(now - measureStart);

      // Na 0.2s mag de timer stoppen als het glas terugkomt
      if ((now - measureStart) >= 200 && glassEmpty()) {
        // Lees de UID opnieuw als controle
        String newUID = readUID();
        if (newUID == currentUID) {
          // UID komt overeen, stop de timer
          unsigned long duration = now - measureStart;
          Serial.print("Meting gestopt, drinktijd(ms): ");
          Serial.println(duration);

          // LED-indicatie na drinken
          if (duration < 5000) {
            // < 5s → groen
            pixels.fill(pixels.Color(0, 255, 0));
          } else if (duration < 10000) {
            // 5–10s → oranje
            pixels.fill(pixels.Color(255, 165, 0));
          } else {
            // ≥10s → rood
            pixels.fill(pixels.Color(255, 0, 0));
          }
          pixels.show();

          if (mqttClient.connected()) {
            sendMQTT(currentUID, duration);
          } else {
            Serial.println("Publicatie overgeslagen: MQTT niet verbonden");
          }

          doneUntil = now + 5000;
          state = DONE;
        } else {
          // UID is anders → vals spel. Knipper rood en reset naar WAIT_TAG.
          Serial.println("Andere UID gedetecteerd, vals spel! Knipper rood en reset.");
          unsigned long blinkStart = now;
          while (millis() - blinkStart < 2000) {
            // Knipper rood: 250 ms aan, 250 ms uit
            if ((millis() - blinkStart) % 500 < 250) {
              pixels.fill(pixels.Color(255, 0, 0));
            } else {
              pixels.clear();
            }
            pixels.show();
          }
          // Reset display en LEDs, terug naar WAIT_TAG
          display.clear(); display.writeDisplay();
          pixels.clear(); pixels.show();
          state = WAIT_TAG;
        }
      }
      break;
  }

    

    case DONE:
      if (now >= doneUntil) {
        pixels.clear(); pixels.show();
        display.clear(); display.writeDisplay();
        Serial.println("Klaar, wacht op nieuwe sessie");
        state = WAIT_TAG;
      }
      break;
    }
}

void connectWiFi() {
  Serial.print("Verbinden met WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000);
  Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "FAIL");
}

bool connectMQTT() {
  Serial.print("MQTT verbinden...");
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  if (mqttClient.connect("ESP32Client")) {
    Serial.println("OK");
    return true;
  }
  Serial.print("Mislukt rc=");
  Serial.println(mqttClient.state());
  return false;
}

void checkReset() {
  if (digitalRead(RESET_PIN) == LOW) return;
  Serial.println("Reset ingedrukt, herstart...");
  ESP.restart();
}

String readUID() {
  uint8_t uid[7], len;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 100)) {
    String s;
    for (uint8_t i = 0; i < len; i++) {
      if (uid[i] < 0x10) s += '0';
      s += String(uid[i], HEX);
    }
    s.toUpperCase();
    return s;
  }
  return String();
}

bool glassFull()    { return analogRead(FSR_PIN) >= FSR_FULL_THRESHOLD; }
bool glassRemoved() { return analogRead(FSR_PIN) <= FSR_NOGLASS_THRESHOLD; }
bool glassEmpty()   { return analogRead(FSR_PIN) >= FSR_EMPTY_THRESHOLD; }

void showTime(unsigned long ms) {
  int s  = ms / 1000;
  int cs = (ms % 1000) / 10;
  display.clear();
  display.writeDigitNum(0, s / 10, false);
  display.writeDigitNum(1, s % 10, false);
  display.drawColon(true);
  display.writeDigitNum(3, cs / 10, false);
  display.writeDigitNum(4, cs % 10, false);
  display.writeDisplay();
}

void sendMQTT(const String& uid, unsigned long t) {
  unsigned int secs = t / 1000;
  unsigned int cs  = (t % 1000) / 10;
  char buf[128];
  snprintf(buf, sizeof(buf),
           "{\"uid\":\"%s\",\"drinktijd\":%u.%02u}",
           uid.c_str(), secs, cs);
  Serial.print("MQTT payload: ");
  Serial.println(buf);
  mqttClient.publish(MQTT_TOPIC, buf);
}

