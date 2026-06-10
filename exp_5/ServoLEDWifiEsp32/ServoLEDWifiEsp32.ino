#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ServoLED_ESP32_GrupoK";
const char* password = "";

WebServer server(80);

constexpr int LED_PIN = 2;
constexpr int SERVO_PIN = 7;
constexpr int LED_FREQ = 5000;
constexpr int LED_RESOLUTION = 8;
constexpr int SERVO_RESOLUTION = 16;
constexpr int SERVO_UPDATE_INTERVAL_MS = 20;
constexpr int SERVO_MIN_PULSE_US = 500;
constexpr int SERVO_MAX_PULSE_US = 2500;

int ledBrightnessPercent = 0;
int currentServoAngle = 90;
int targetServoAngle = 90;
int lastStationCount = 0;
unsigned long lastServoUpdateMs = 0;
unsigned long lastWiFiCheckMs = 0;

void setupWiFi() {
    // Inicialização da Rede SoftAP do ESP32
    WiFi.softAP(ssid, password);
    Serial.println("Wi-Fi Iniciado com Sucesso!");
    Serial.print("IP do Servidor da Calculadora: ");
    Serial.println(WiFi.softAPIP());
}

void setupPWM() {
    ledcAttach(LED_PIN, LED_FREQ, LED_RESOLUTION);
    ledBrightnessPercent = 0;
    ledcWrite(LED_PIN, 0);
}

int angleToServoDuty(int angle) {
    const int maxDuty = (1 << SERVO_RESOLUTION) - 1;
    const int minDuty = (SERVO_MIN_PULSE_US * maxDuty) / 20000;
    const int maxPulseDuty = (SERVO_MAX_PULSE_US * maxDuty) / 20000;
    return map(angle, 0, 180, minDuty, maxPulseDuty);
}

void setServoAngle(int angle) {
    int duty = angleToServoDuty(angle);
    ledcWrite(SERVO_PIN, duty);
}

void setupServo() {
    ledcAttach(SERVO_PIN, 50, SERVO_RESOLUTION);
    currentServoAngle = 90;
    targetServoAngle = 90;
    setServoAngle(currentServoAngle);
}

void handleStatus() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "OK");
}

int gammaCorrectDuty(int brightness) {
    const float normalized = brightness / 100.0f;
    const float corrected = pow(normalized, 2.2f);
    const int maxDuty = (1 << LED_RESOLUTION) - 1;
    const int duty = round(corrected * maxDuty);
    return constrain(duty, 0, maxDuty);
}

void handleLed() {
    unsigned long startMicros = micros();
    server.sendHeader("Access-Control-Allow-Origin", "*");

    if (!server.hasArg("brightness")) {
        server.send(400, "application/json", "{\"error\":\"Parâmetro brightness ausente\"}");
        Serial.println("[ERROR] /led sem parâmetro brightness");
        return;
    }

    int brightness = server.arg("brightness").toInt();
    brightness = constrain(brightness, 0, 100);
    ledBrightnessPercent = brightness;

    const int duty = gammaCorrectDuty(brightness);
    ledcWrite(LED_PIN, duty);

    unsigned long latency = micros() - startMicros;
    Serial.printf("[LED] brightness=%d%% duty=%d latency=%lums\n", brightness, duty, latency);

    String payload = "{";
    payload += "\"brightness\":" + String(brightness) + ",";
    payload += "\"duty\":" + String(duty) + ",";
    payload += "\"latency_us\":" + String(latency);
    payload += "}";
    server.send(200, "application/json", payload);
}

void handleServo() {
    unsigned long startMicros = micros();
    server.sendHeader("Access-Control-Allow-Origin", "*");

    if (!server.hasArg("angle")) {
        server.send(400, "application/json", "{\"error\":\"Parâmetro angle ausente\"}");
        Serial.println("[ERROR] /servo sem parâmetro angle");
        return;
    }

    int angle = server.arg("angle").toInt();
    angle = constrain(angle, 0, 180);
    targetServoAngle = angle;

    unsigned long latency = micros() - startMicros;
    Serial.printf("[SERVO] target=%d° latency=%lums\n", angle, latency);

    String payload = "{";
    payload += "\"angle\":" + String(angle) + ",";
    payload += "\"current\":" + String(currentServoAngle) + ",";
    payload += "\"latency_us\":" + String(latency);
    payload += "}";
    server.send(200, "application/json", payload);
}

void setupRoutes() {
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/led", HTTP_GET, handleLed);
    server.on("/servo", HTTP_GET, handleServo);
}

void updateServoPosition() {
    unsigned long now = millis();
    if (now - lastServoUpdateMs < SERVO_UPDATE_INTERVAL_MS) {
        return;
    }

    if (currentServoAngle == targetServoAngle) {
        return;
    }

    if (targetServoAngle > currentServoAngle) {
        currentServoAngle++;
    } else if (targetServoAngle < currentServoAngle) {
        currentServoAngle--;
    }

    setServoAngle(currentServoAngle);
    lastServoUpdateMs = now;
}

void checkWiFi() {
    int stationCount = WiFi.softAPgetStationNum();

    if (stationCount != lastStationCount) {
        if (lastStationCount > 0 && stationCount == 0) {
            Serial.println("[ERROR] WiFi disconnected");
        } else if (lastStationCount == 0 && stationCount > 0) {
            Serial.println("[INFO] Cliente conectado ao SoftAP");
        }
        lastStationCount = stationCount;
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    setupWiFi();
    setupPWM();
    setupServo();
    setupRoutes();

    server.begin();
    Serial.println("Servidor HTTP ativo na porta 80.");
}

void loop() {
    server.handleClient();
    updateServoPosition();
    checkWiFi();
}
