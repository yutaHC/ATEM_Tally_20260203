#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- Wi-Fi設定 ---
const char* networkName = "HAIRCAMP_Studio";
const char* networkPswd = "haircampstudio";

// --- UDP設定 ---
WiFiUDP udp;
const unsigned int localUdpPort = 8888; // 受信ポート番号

// --- 外部LEDピン設定 ---
const int NUM_EXTERNAL_LEDS = 5;
const int externalLedPins[NUM_EXTERNAL_LEDS] = {19, 21, 22, 23, 25}; 

void updateTally(uint8_t r, uint8_t g, uint8_t b, bool externalOn) {
    for (int i = 0; i < 25; i++) {
        M5.dis.drawpix(i, (r << 16) | (g << 8) | b);
    }
    for (int i = 0; i < NUM_EXTERNAL_LEDS; i++) {
        digitalWrite(externalLedPins[i], externalOn ? HIGH : LOW);
    }
}

void setup() {
    M5.begin(true, false, true);
    Serial.begin(115200);
    WiFi.setSleep(false); // スリープ禁止

    for (int i = 0; i < NUM_EXTERNAL_LEDS; i++) {
        pinMode(externalLedPins[i], OUTPUT);
        digitalWrite(externalLedPins[i], LOW);
    }

    WiFi.begin(networkName, networkPswd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.dis.drawpix(0, 0xaaaa00); 
    }

    udp.begin(localUdpPort); // UDP開始
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    // 起動サイン
    M5.dis.clear();
    M5.dis.drawpix(12, 0x0000ff); 
}

void loop() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        char packetBuffer[255];
        int len = udp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;
        
        String command = String(packetBuffer);

        if (command == "pgm") {
            updateTally(255, 0, 0, true);
        } else if (command == "pvw" || command == "off") {
            updateTally(0, 0, 0, false);
        }
    }
    M5.update();
}