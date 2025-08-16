#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include<SoftwareSerial.h>
#include<Servo.h>

// 手机热点配置 
const char* PHONE_HOTSPOT_SSID = "lch";
const char* PHONE_HOTSPOT_PASSWORD = "18563263050";

//获取天气信息的网站
const char* host = "api.52vmy.cn";
const int httpsPort = 443;
const char*path = "/api/query/tian/three?city=北京市";

// Web服务器在端口80上运行
ESP8266WebServer server(80);

//和主板通信的串口
SoftwareSerial mySerial(D3,D4);

//控制竖直方向的舵机
Servo myservo;
int shangxia=0;

//手机控制的信息
bool isFinding = false;
bool isLocking = false;

//网络获取的信息
String temperature = "";  
String weatherCondition = "";
String currentTime = "";

//服务器处理函数
void handleRoot() {
  server.send(200, "text/plain", "ESP8266");
}

//或取天气的函数
void getWeatherInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client; // 因为是 https，使用 WiFiClientSecure
    client.setInsecure(); // 忽略证书验证，仅用于测试环境
    HTTPClient http;
    String url = "https://" + String(host) + path ;
    http.begin(client, url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();


        const size_t capacity = 1024; 
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          weatherCondition = doc["data"]["data"][1]["weather"].as<String>();
          temperature = doc["data"]["data"][1]["temperature"].as<String>();
          Serial.println(weatherCondition);
          mySerial.println("weather"+weatherCondition);
          Serial.println(temperature);
          mySerial.println("temp"+temperature);
        } else {
          Serial.print("JSON parse error: ");
          Serial.println(error.c_str());
        }
      }
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
}

//获取时间的函数
void getTimeInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client; // 因为是 https，使用 WiFiClientSecure
    client.setInsecure(); // 忽略证书验证，仅用于测试环境
    HTTPClient http;
    String url = "https://api.52vmy.cn/api/chat/glm?msg=你好";
    http.begin(client, url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        const size_t capacity = 512; 
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          currentTime = doc["data"]["time"].as<String>();
          Serial.println(currentTime);
          if (currentTime.length() != 19) {
            return; // 格式错误
          }
          
          if (currentTime[4] != '-' || currentTime[7] != '-' || 
              currentTime[10] != ' ' ||currentTime[13] != ':' || currentTime[16] != ':') {
            return ; 
          }
          
          String month =currentTime.substring(5, 7);
          mySerial.println("month:"+month);
          String date = currentTime.substring(8, 10);
          mySerial.println("data:"+date);
          String hour = currentTime.substring(11, 13);
          mySerial.println("hour:"+hour);
          String minute = currentTime.substring(14, 16);
          mySerial.println("minute:"+minute);
        } else {
          Serial.print("JSON parse error: ");
          Serial.println(error.c_str());
        }
      }
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
}

//处理手机收到的数据
void handleData() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, server.arg(0));

  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  if (doc.containsKey("isFinding")) isFinding = doc["isFinding"];
  if (doc.containsKey("isLocking")) isLocking = doc["isLocking"];
  Serial.printf("控制状态: finding=%d, locking=%d\n", isFinding, isLocking);
  mySerial.println(isLocking?"lock":"unlock");
  //digitalWrite(D6,isFinding);
  if(isFinding){
    tone(D6,500,10000);
    //digitalWrite(D6,HIGH);
  }else{
    digitalWrite(D6,LOW);
  }
  
  // 发送响应
  server.send(200, "text/plain", "Data received");
}

void setup() {
  mySerial.begin(9600);                   //通信串口
  Serial.begin(9600);                     //调试串口

  //通过STA模式连接手机热点
  WiFi.mode(WIFI_STA);
  WiFi.begin(PHONE_HOTSPOT_SSID, PHONE_HOTSPOT_PASSWORD);
  Serial.print("连接热点中");
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("连接失败，请检查热点信息");
    while (1); 
  }
  Serial.println("连接成功");
  Serial.printf("ESP8266 IP地址: %s\n", WiFi.localIP().toString().c_str());
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web服务器已启动");

  //获取网络数据
  delay(500);
  getWeatherInfo();
  delay(500);
  getTimeInfo();
  
  //竖直方向舵机控制
  myservo.attach(D5);

  //蜂鸣器报警
  pinMode(D6,OUTPUT);
}

void loop() {
  //确保数据得到
  if(!temperature)
    getWeatherInfo();
  if(!currentTime)
    getTimeInfo();

  //处理手机数据
  server.handleClient();
  
  //控制竖直旋转电机
  int x=analogRead(A0);
  if(x<20&&shangxia>0)
    shangxia-=5;
  if(x>1000&&shangxia<180)
    shangxia+=5;
  myservo.write(shangxia);
  delay(10);
}