//Alarm Pendeteksi hujan
//Imam Maulana Al'arief
//Sistem Komputer 19.2 Siliwangi

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

AsyncWebServer server(80);

LiquidCrystal_I2C lcd(0x27, 16,2);

const int sensorPin = A0;    //inisialisasi pin button
const int buzzer = 7;      //inisialisasi pin buzzer
const int ledGreen = 8;   //inisialisasi pin LED
const int ledRed = 9;    //inisialisasi pin LED

String ssid;
String pass;
String ip;
String gateway;

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
IPAddress localGateway;

IPAddress subnet(255, 255, 0, 0);

unsigned long previousMillis = 0;
const long interval = 10000; 

const char* firebaseUrl = "https://aigreen-default-rtdb.asia-southeast1.firebasedatabase.app/tetesan.json";

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Error mounting SPIFFS");
  }
  Serial.println("Mounting SPIFFS berhasil");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- gagal membaca file");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- Gagal generate file");
    return;
  }
  if(file.print(message)){
    Serial.println("- File berhasil tergenerate");
  } else {
    Serial.println("- Gagal generate file");
  }
}

bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("SSID atau IP address tidak diketahui.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("Gagal konfigurasi STA");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Menghubungkan...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Gagal terhubung.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void setup()
{
  Serial.begin (9600);

  initSPIFFS();

  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(!initWiFi()) {
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == "ssid") {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == "pass") {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == "ip") {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == "gateway") {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }

      request->send(200, "text/plain", "Berhasil. ESP akan direstart.");

      delay(3000);
      ESP.restart();
    });
    server.begin();
  } else {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Wifi sudah terhubung.");
    });
  }

  pinMode(buzzer, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(sensorPin, INPUT);
  
  lcd.begin (16,2);
  lcd.setBacklight(255); //menghidupkan lampu latar LCD
  lcd.setCursor (0, 0);
  lcd.print("**SENSOR HUJAN**");
  lcd.setCursor (0, 1);
  lcd.print("Ardha Pria");

  delay(5000);
  lcd.clear();
}

void loop() {
  //membaca nilai dari pin button
  int nilaiSensor = analogRead(sensorPin);
  //mengecek jika sensor terkena air
  Serial.println (nilaiSensor);
  delay (1000);
  //jika nilaiSensor < 500 (range sensor 0-1023)/sensor terkena air.
  //nyalakan alarm dan LED merah

  if (nilaiSensor <= 500){
    digitalWrite (ledRed, HIGH);
    digitalWrite (buzzer, HIGH);
    digitalWrite (ledGreen, LOW);
    lcd.setCursor(0,0); //Mapping LCD nya
    lcd.print("**SENSOR HUJAN**"); //(Ganti Nama Anda, Karakter Sebanyak 16 termasuk spasi)
    lcd.setCursor(0,1);
    lcd.print("*KONDISI  HUJAN*");
    delay(2000);
    lcd.clear(); //Bersihkan LCD dari karakter yg ada
  }

  else {
    lcd.setCursor(0,0); //Mapping LCD nya
    lcd.print("**SENSOR HUJAN**"); //(Ganti Nama Anda, Karakter Sebanyak 16 termasuk spasi)
    lcd.setCursor(0,1);
    lcd.print(" *CUACA  CERAH* ");
    digitalWrite (ledRed, LOW);
    digitalWrite (buzzer, LOW);
    digitalWrite (ledGreen, HIGH);   
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, firebaseUrl);

    http.addHeader("Content-Type", "application/json");

    String httpRequestData = String(nilaiSensor);

    int httpResponseCode = http.PUT(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
      
    http.end();
  }
}
