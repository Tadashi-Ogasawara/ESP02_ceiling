#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>


const int    MODE_PIN = 0; // GPIO0// モード切り替えピン
const char*  settings = "/wifi_settings.txt";// Wi-Fi設定保存ファイル
const String pass     = "password";// サーバモードでのパスワード
const string AP_TITLE = "ESP02_ceiling";

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);



/*** WiFi設定 ***/
void handleRootGet() {
  String html = ""
    "<!DOCTYPE html>"
    "<html><head>"
    "<title>CaptivePortal</title>"
    "</head><body>"
    "<h1>WiFi Settings</h1>"
    "<form method='post'>"
    "  <input type='text' name='ssid' placeholder='ssid'><br>"
    "  <input type='text' name='pass' placeholder='pass'><br>"
    "  <input type='submit'><br>"
    "</form>";
  server.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  File f = SPIFFS.open(settings, "w");
  f.println(ssid);
  f.println(pass);
  f.close();

  String html = "";
    html += "<!DOCTYPE html>"
    html += "<html><head>"
    html += "<title>CaptivePortal</title>"
    html += "</head><body>"
    html += "<h1>WiFi Settings</h1>";
    html += ssid + "<br>";
    html += pass + "<br>";
    html += "</body></html>";
  server.send(200, "text/html", html);
}

/*** 初期化(クライアントモード) ***/
void setup_client() {

  File f = SPIFFS.open(settings, "r");
  String ssid = f.readStringUntil('\n');
  String pass = f.readStringUntil('\n');
  f.close();

  ssid.trim();
  pass.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/*** 初期化(サーバモード) ***/
void setup_server() {
  byte mac[6];
  WiFi.macAddress(mac);
  String ssid = "";
  for (int i = 0; i < 6; i++) {
    ssid += String(mac[i], HEX);
  }
  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid.c_str(), pass.c_str());

  dnsServer.start(DNS_PORT, "*", apIP);

  server.onNotFound([]() {
    server.send(200, "text/html", responseHTML);
    server.on("/", HTTP_GET, handleRootGet);
    server.on("/", HTTP_POST, handleRootPost);
  }
  server.begin();
  Serial.println("HTTP server started.");
}

/*** 初期化 ***/
void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(2000);    // 2秒以内にMODEを切り替える 0:Server/1:Client
  SPIFFS.begin(); // ファイルシステム初期化

  pinMode(MODE_PIN, INPUT);
  if (digitalRead(MODE_PIN) == 0) {
    setup_server();    // サーバモード初期化
  } else {
    setup_client();    // クライアントモード初期化
  }
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}