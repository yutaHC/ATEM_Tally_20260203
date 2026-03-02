#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ESPmDNS.h>

// --- 設定保存用 ---
Preferences preferences;
String ssid = "";
String password = "";
int brightness = 255;
bool enablePreview = true;
String deviceName = ""; // mDNSホスト名 (例: tally-a1b2c3)

// --- ネットワーク設定 ---
WebServer server(80);
WiFiUDP udp;
const unsigned int localUdpPort = 8888;
bool isAPMode = false;

// --- LED設定 ---
const int NUM_EXTERNAL_LEDS = 5;
const int externalLedPins[NUM_EXTERNAL_LEDS] = {19, 21, 22, 23, 25};
// PWM設定
const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmChannels[NUM_EXTERNAL_LEDS] = {0, 1, 2, 3, 4};

// 現在の状態
String currentState = "off";

// HTML テンプレート (設定画面)
const char* htmlHeader = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Tally Setup</title><style>body{font-family:sans-serif;margin:20px;}input,button,select{font-size:16px;margin:5px 0;padding:5px;}label{display:block;margin-top:10px;}</style></head><body><h2>ATEM Tally Setup</h2>";
const char* htmlFooter = "</body></html>";

void setupPWM() {
    for (int i = 0; i < NUM_EXTERNAL_LEDS; i++) {
        ledcSetup(pwmChannels[i], pwmFreq, pwmResolution);
        ledcAttachPin(externalLedPins[i], pwmChannels[i]);
        ledcWrite(pwmChannels[i], 0);
    }
}

void setExternalLEDs(int val) {
    for (int i = 0; i < NUM_EXTERNAL_LEDS; i++) {
        ledcWrite(pwmChannels[i], val);
    }
}

void updateTallyDisplay() {
    if (currentState == "pgm") {
        M5.dis.drawpix(0, 0xff0000); // 内蔵LEDも赤
        setExternalLEDs(brightness);
    } else if (currentState == "pvw") {
        setExternalLEDs(0);
        if (enablePreview) {
            M5.dis.drawpix(0, 0x00ff00); // 内蔵緑
        } else {
            M5.dis.drawpix(0, 0x000000); // 消灯
        }
    } else { // off
        M5.dis.drawpix(0, 0x000000);
        setExternalLEDs(0);
    }
}

void handleRoot() {
    String html = htmlHeader;
    html += "<form action='/save' method='POST'>";
    html += "<label>Device Name (hostname):</label><input type='text' name='devicename' value='" + deviceName + "'><br>";
    html += "<small style='color:#888'>Companionに表示されるID。変更する場合は半角英数字のみ。例: cam1, cam2</small><br><br>";
    html += "<label>SSID:</label><input type='text' name='ssid' value='" + ssid + "'><br>";
    html += "<label>Password:</label><input type='password' name='password' value='" + password + "'><br>";
    html += "<label>Brightness (0-255):</label><input type='number' name='brightness' min='0' max='255' value='" + String(brightness) + "'><br>";
    html += "<label>Preview LED (Green):</label><select name='preview'>";
    html += "<option value='1'" + String(enablePreview ? " selected" : "") + ">ON</option>";
    html += "<option value='0'" + String(!enablePreview ? " selected" : "") + ">OFF</option>";
    html += "</select><br><br>";
    html += "<button type='submit'>Save & Restart</button>";
    html += "</form>";
    html += "<hr><h3>OTA Firmware Update</h3><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><button type='submit'>Upload & Update</button></form>";
    html += htmlFooter;
    server.send(200, "text/html", html);
}

void handleSave() {
    if (server.hasArg("devicename") && server.arg("devicename").length() > 0)
        deviceName = server.arg("devicename");
    if (server.hasArg("ssid")) ssid = server.arg("ssid");
    if (server.hasArg("password")) password = server.arg("password");
    if (server.hasArg("brightness")) brightness = server.arg("brightness").toInt();
    if (server.hasArg("preview")) enablePreview = (server.arg("preview") == "1");

    preferences.begin("tally", false);
    preferences.putString("devicename", deviceName);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putInt("brightness", brightness);
    preferences.putBool("preview", enablePreview);
    preferences.end();

    server.send(200, "text/html", htmlHeader + String("<p>Saved! Restarting...</p>") + htmlFooter);
    delay(1000);
    ESP.restart();
}

void handleUpdate() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
}

void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %uB\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

void setup() {
    M5.begin(true, false, true);
    Serial.begin(115200);
    setupPWM();

    preferences.begin("tally", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    brightness = preferences.getInt("brightness", 255);
    enablePreview = preferences.getBool("preview", true);
    deviceName = preferences.getString("devicename", "");
    preferences.end();

    // deviceNameが未設定ならMACアドレスから自動生成
    if (deviceName == "") {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char macSuffix[7];
        snprintf(macSuffix, sizeof(macSuffix), "%02x%02x%02x", mac[3], mac[4], mac[5]);
        deviceName = String("tally-") + macSuffix;
        // NVSに保存しておく
        preferences.begin("tally", false);
        preferences.putString("devicename", deviceName);
        preferences.end();
    }
    Serial.println("Device Name: " + deviceName);

    // ボタンが押されているか、SSIDが空ならAPモード
    M5.update();
    if (ssid == "" || M5.Btn.isPressed()) {
        isAPMode = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Tally-Setup");
        Serial.println("AP Mode: Tally-Setup");
        M5.dis.drawpix(0, 0xffff00); // APモードは黄色
    } else {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.print("Connecting to WiFi");
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20) { // 10秒待機
            delay(500);
            Serial.print(".");
            M5.dis.drawpix(0, (timeout % 2 == 0) ? 0x0000ff : 0x000000); // 接続中 青点滅
            timeout++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            // 接続成功: 緑一瞬
            M5.dis.drawpix(0, 0x00ff00);
            Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

            // mDNS起動: deviceName.local でアクセス可能に
            if (MDNS.begin(deviceName.c_str())) {
                // UDPタリーサービスを広告する
                MDNS.addService("tally", "udp", localUdpPort);
                MDNS.addServiceTxt("tally", "udp", "name", deviceName.c_str());
                MDNS.addServiceTxt("tally", "udp", "port", String(localUdpPort).c_str());
                Serial.println("mDNS started: " + deviceName + ".local");
            } else {
                Serial.println("mDNS failed");
            }

            delay(1000);
            M5.dis.drawpix(0, 0x000000);
            
            udp.begin(localUdpPort);
            Serial.println("UDP Listening on port " + String(localUdpPort));
        } else {
            // 失敗: 赤点灯後、APモードへフォールバック
            isAPMode = true;
            WiFi.mode(WIFI_AP);
            WiFi.softAP("Tally-Setup");
            M5.dis.drawpix(0, 0xff0000);
            Serial.println("\nWiFi Connect Failed. Fallback to AP Mode.");
        }
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/update", HTTP_POST, handleUpdate, handleUpdateUpload);
    server.begin();
}

void loop() {
    M5.update();
    server.handleClient();

    if (!isAPMode) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            char packetBuffer[255];
            int len = udp.read(packetBuffer, 255);
            if (len > 0) packetBuffer[len] = 0;
            
            String command = String(packetBuffer);
            command.trim();

            if (command == "pgm") {
                currentState = "pgm";
                updateTallyDisplay();
            } else if (command == "pvw") {
                currentState = "pvw";
                updateTallyDisplay();
            } else if (command == "off") {
                currentState = "off";
                updateTallyDisplay();
            } else if (command == "identify") {
                // Identity（全体青色とLEDが高速点滅）
                for (int i=0; i<10; i++) {
                    M5.dis.drawpix(0, 0x0000ff);
                    setExternalLEDs(brightness);
                    delay(100);
                    M5.dis.drawpix(0, 0x000000);
                    setExternalLEDs(0);
                    delay(100);
                }
                updateTallyDisplay(); // 元の状態に戻す
            } else if (command.startsWith("dim:")) {
                // 例: "dim:128" -> 128
                int newBright = command.substring(4).toInt();
                if (newBright >= 0 && newBright <= 255) {
                    brightness = newBright;
                    // 保存する
                    preferences.begin("tally", false);
                    preferences.putInt("brightness", brightness);
                    preferences.end();
                    // 設定反映
                    updateTallyDisplay();
                }
            }
        }
    }
}