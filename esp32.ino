//Alarm Pendeteksi hujan
//TEAM 6
//Sistem Komputer 19.2 Jember

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

AsyncWebServer server(80);


/* 2. Define the API Key */
#define API_KEY "AIzaSyB6tHJpz3NY6XNVCxMFT3tC2V19KOSujm4"

/* 3. Define the RTDB URL */
#define DATABASE_URL "raindetector-81030-default-rtdb.asia-southeast1.firebasedatabase.app" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

#define USER_EMAIL "superadmin@admin.com"
#define USER_PASSWORD "superadmin"

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

LiquidCrystal_I2C lcd(0x27, 16,2);

const int sensorPin = 34;    //inisialisasi pin button
const int buzzer = 4;      //inisialisasi pin buzzer
const int ledGreen = 33;   //inisialisasi pin LED
const int ledRed = 32;    //inisialisasi pin LED

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

// server firebase. gantien berdasarkan firebasemu. lahh? iki ngerun ga seh?
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

//  WiFi.mode(WIFI_STA);
//  localIP.fromString(ip.c_str());
//  localGateway.fromString(gateway.c_str());
//
//  if (!WiFi.config(localIP, localGateway, subnet)){
//    Serial.println("Gagal konfigurasi STA");
//    return false;
//  }
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

  Serial.println("Berhasil terhubung");
  Serial.println(WiFi.localIP());
  return true;
}

void initFirebase() {
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 /, 1024 / Tx buffer size in bytes from 512 - 16384 */);

  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;
}

void setup()
{
  Serial.begin (115200);

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
    Serial.print("tes");
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
  }

  initFirebase();

  pinMode(buzzer, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(sensorPin, INPUT);
  
  lcd.begin (16,2);
  lcd.setBacklight(255); //menghidupkan lampu latar LCD
  lcd.setCursor (0, 0);
  lcd.print("*SENSOR HUJAN*");
  lcd.setCursor (0, 1);
  lcd.print("Ardha Pria");

  delay(5000);
  lcd.clear();
}

void loop() {
  //membaca nilai dari pin button
  int nilaiSensor = analogRead(sensorPin);
  int nilai = nilaiSensor / 40.95;

  nilai = nilai - 100;

  if (nilai < 0) {
    nilai = nilai * -1;
  }

  Serial.println(nilai);
  
  //mengecek jika sensor terkena air
  Serial.println (nilaiSensor);
  delay (1000);
  //jika nilaiSensor < 500 (range sensor 0-1023)/sensor terkena air.
  //nyalakan alarm dan LED merah

  if (nilai > 50){
    digitalWrite (ledRed, HIGH);
    digitalWrite (buzzer, HIGH);
    digitalWrite (ledGreen, LOW);
    lcd.setCursor(0,0); //Mapping LCD nya
    lcd.print("*SENSOR HUJAN*"); //(Ganti Nama Anda, Karakter Sebanyak 16 termasuk spasi)
    lcd.setCursor(0,1);
    lcd.print("KONDISI  HUJAN");
    delay(2000);
    lcd.clear(); //Bersihkan LCD dari karakter yg ada
  }

  else {
    lcd.setCursor(0,0); //Mapping LCD nya
    lcd.print("*SENSOR HUJAN*"); //(Ganti Nama Anda, Karakter Sebanyak 16 termasuk spasi)
    lcd.setCursor(0,1);
    lcd.print(" CUACA  CERAH ");
    digitalWrite (ledRed, LOW);
    digitalWrite (buzzer, LOW);
    digitalWrite (ledGreen, HIGH);   
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (Firebase.ready()) {
      Serial.printf("Set intensitas %s\n", Firebase.RTDB.setInt(&fbdo, F("/tetesan"), nilai) ? "ok" : fbdo.errorReason().c_str());
    }
  }
}