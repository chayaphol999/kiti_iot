#define BLYNK_TEMPLATE_NAME "flame alert"
#define BLYNK_AUTH_TOKEN "EDTH98xY92vlZApnwtvMFWDE_VoU3uMb"
#define BLYNK_TEMPLATE_ID "TMPL6qDI_hiVe"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleWiFi.h> 
#include <DHT.h>

char ssid[] = "iPhone";
char pass[] = "1234567890";

// --- PIN DEFINITIONS ---
#define FLAME_PIN 26
#define DHT_PIN 17
#define BUZZER_PIN 18

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer;

// --- CONFIG ---
#define FLAME_THRESHOLD 750 // Catch fire earlier based on your logs (starting ~695)
#define TEMP_LV1 35.0
#define TEMP_LV2 40.0
#define TEMP_LV3 80.0

float currentTemp = 0;
bool isFireDetected = false;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// ---------------- ALARM ----------------
int getAlarmLevel() {
    if (isFireDetected) return 4;
    if (currentTemp >= TEMP_LV3) return 3;
    if (currentTemp >= TEMP_LV2) return 2;
    if (currentTemp >= TEMP_LV1) return 1;
    return 0;
}

void updateBuzzer() {
    int level = getAlarmLevel();
    unsigned long now = millis();
    unsigned long interval = 0;
    unsigned long onTime = 200; // Duration of beep

    switch (level) {
        case 0: 
            digitalWrite(BUZZER_PIN, LOW);
            digitalWrite(LED_BUILTIN, LOW);
            buzzerState = false;
            return;
        case 1: interval = 2000; break; // Slow (1 beep/2s)
        case 2: interval = 1000; break; // Medium (1 beep/1s)
        case 3: interval = 500; break;  // Fast (1 beep/0.5s)
        case 4: // Fire - Continuous
            digitalWrite(BUZZER_PIN, HIGH);
            digitalWrite(LED_BUILTIN, (now % 200 < 100) ? HIGH : LOW); // Blink LED if fire
            return;
    }

    // Non-blocking beep pattern
    unsigned long currentInterval = buzzerState ? onTime : (interval - onTime);
    if (now - lastBuzzerToggle >= currentInterval) {
        lastBuzzerToggle = now;
        buzzerState = !buzzerState;
        digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        digitalWrite(LED_BUILTIN, buzzerState ? HIGH : LOW);
    }
}

// ---------------- SENSOR ----------------
void sendSensorData() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("DHT read failed!");
        return;
    }

    Serial.printf("Temp: %.1f C, Hum: %.1f %%\n", t, h);
    currentTemp = t; // Update global temp for alarm assessment

    Blynk.virtualWrite(V1, t);
    Blynk.virtualWrite(V2, h);
    Blynk.virtualWrite(V4, getAlarmLevel()); // Send current alarm level to Blynk
}

// ---------------- FLAME ----------------
void checkFlame() {
    int flameValue = analogRead(FLAME_PIN);

    Serial.printf("Flame: %d\n", flameValue);

    // 🔥 Detect (Active-LOW: Fire = 35, Idle = 1023)
    if (flameValue < FLAME_THRESHOLD) {
        if (!isFireDetected) {
            Serial.println("!!! FIRE DETECTED !!!");
            Blynk.virtualWrite(V3, 1);
            isFireDetected = true;
        }
    } 
    // 🟢 Clear
    else if (flameValue >= FLAME_THRESHOLD) {
        if (isFireDetected) {
            Serial.println("Fire cleared");
            Blynk.virtualWrite(V3, 0);
            isFireDetected = false;
        }
    }
}

// ---------------- SETUP ----------------
void setup() {
    Serial.begin(115200);

    pinMode(FLAME_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    dht.begin();

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    timer.setInterval(5000L, sendSensorData);
    timer.setInterval(500L, checkFlame);

    Serial.println("--- System Ready (Active-LOW / Enhanced Alarm) ---");
}

// ---------------- LOOP ----------------
void loop() {
    Blynk.run();
    timer.run();
    updateBuzzer(); // Continuously handle alarm patterns
}