#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ServoLED_ESP32_GrupoK";
const char* password = "";

WebServer server(80);

constexpr int LED_PIN = 2;
constexpr int SERVO_PIN = 7;
constexpr int LED_FREQ = 5000;
constexpr int LED_RESOLUTION = 8;
constexpr int SERVO_FREQ = 50;
constexpr int SERVO_RESOLUTION = 12;

int ledBrightnessPercent = 0;
int currentServoAngle    = 90;
int lastStationCount     = 0;

void setupWiFi() {
    WiFi.softAP(ssid, password);
    Serial.println("Wi-Fi Iniciado com Sucesso!");
    Serial.print("IP do Servidor: ");
    Serial.println(WiFi.softAPIP());
}

void setServoAngle(int angle) {
    angle = constrain(angle, 0, 180);
    int duty = map(angle, 0, 180, 102, 512);
    ledcWrite(SERVO_PIN, duty);
    currentServoAngle = angle;
    Serial.printf("[SERVO] angulo=%d duty=%d\n", angle, duty);
}

void setupPWM() {
    ledcAttach(LED_PIN, LED_FREQ, LED_RESOLUTION);
    ledBrightnessPercent = 0;
    ledcWrite(LED_PIN, 0);
}

void setupServo() {
    ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION);
    setServoAngle(90);
    Serial.println("[SERVO] Inicializado em 90 graus");
}

void handleStatus() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "OK");
}

int gammaCorrectDuty(int brightness) {
    const float normalized = brightness / 100.0f;
    const float corrected  = pow(normalized, 2.2f);
    const int maxDuty      = (1 << LED_RESOLUTION) - 1;
    const int duty         = round(corrected * maxDuty);
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
    Serial.printf("[LED] brightness=%d%% duty=%d latency=%luus\n", brightness, duty, latency);

    String payload = "{";
    payload += "\"brightness\":" + String(brightness) + ",";
    payload += "\"duty\":"       + String(duty)       + ",";
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
    setServoAngle(angle);

    unsigned long latency = micros() - startMicros;
    Serial.printf("[SERVO] target=%d latency=%luus\n", angle, latency);

    String payload = "{";
    payload += "\"angle\":"      + String(angle)             + ",";
    payload += "\"current\":"    + String(currentServoAngle) + ",";
    payload += "\"latency_us\":" + String(latency);
    payload += "}";
    server.send(200, "application/json", payload);
}

void setupRoutes() {
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/led",    HTTP_GET, handleLed);
    server.on("/servo",  HTTP_GET, handleServo);
}

void checkWiFi() {
    int stationCount = WiFi.softAPgetStationNum();
    if (stationCount != lastStationCount) {
        if (lastStationCount > 0 && stationCount == 0)
            Serial.println("[ERROR] WiFi disconnected");
        else if (lastStationCount == 0 && stationCount > 0)
            Serial.println("[INFO] Cliente conectado ao SoftAP");
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
    checkWiFi();
}