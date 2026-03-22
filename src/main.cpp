#define BLYNK_TEMPLATE_ID   "TMPL6cGpWvyX1"
#define BLYNK_TEMPLATE_NAME "Pico2W"
#define BLYNK_AUTH_TOKEN    "VA66Ef3otKKd7JJrfDT-HQFDbqoPcTLN"

#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleWiFi.h> 
#include <DHT.h>

char ssid[] = "HayBro_CH01 999G";
char pass[] = "Fuckyoubro";

// --- PIN DEFINITIONS ---
#define FLAME_PIN 26
#define DHT_PIN 17
#define BUZZER_PIN 18

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer;

// --- CONFIG ---
#define FLAME_THRESHOLD 100 // Trigger when value drops BELOW 100 (Active-LOW)

bool flameState = false;

// ---------------- SENSOR ----------------
void sendSensorData() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("DHT read failed!");
        return;
    }

    Serial.printf("Temp: %.1f C, Hum: %.1f %%\n", t, h);

    Blynk.virtualWrite(V1, t);
    Blynk.virtualWrite(V2, h);
}

// ---------------- FLAME ----------------
void checkFlame() {
    int flameValue = analogRead(FLAME_PIN);

    Serial.printf("Flame: %d\n", flameValue);

    // 🔥 Detect (Active-LOW: Fire = 35, Idle = 1023)
    if (flameValue < FLAME_THRESHOLD && !flameState) {
        Serial.println("!!! FIRE DETECTED !!!");
        digitalWrite(BUZZER_PIN, HIGH); // กลับมาใช้ digitalWrite แบบเดิมตามที่คุณเจ้านายต้องการค่ะ
        digitalWrite(LED_BUILTIN, HIGH);
        Blynk.virtualWrite(V3, 1);
        flameState = true;
    } 
    // 🟢 Clear
    else if (flameValue >= FLAME_THRESHOLD && flameState) {
        Serial.println("Fire cleared");
        digitalWrite(BUZZER_PIN, LOW); // ปิดเสียงแบบเดิม
        digitalWrite(LED_BUILTIN, LOW);
        Blynk.virtualWrite(V3, 0);
        flameState = false;
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

    Serial.println("--- System Ready (Active-LOW / Original Sound) ---");
}

// ---------------- LOOP ----------------
void loop() {
    Blynk.run();
    timer.run();
}