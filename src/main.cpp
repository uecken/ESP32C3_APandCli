#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP32Ping.h>

// If use M5Atom, Please replace below vode in platformio.ini
/*
[env:m5stick-c]
platform = espressif32
board = m5stick-c
framework = arduino
lib_deps = 
	marian-craciunescu/ESP32Ping@^1.7
	adafruit/Adafruit NeoPixel@^1.10.6
	m5stack/M5Atom@^0.1.0
	fastled/FastLED@^3.5.0
	seeed-studio/Grove Ultrasonic Ranger@^1.0.1
monitor_speed = 115200
*/
#define TASK_DEFAULT_CORE_ID 0 //0は無線系が利用するため利用不可.利用するとBluetooth頻繁に送受信停止
#define TASK_STACK_DEPTH 4096UL
#define TASK_NAME_IMU "UltrasonicTask"
#define TASK_SLEEP_ULTRASONIC 10 // = 1000[ms] / 100[Hz]
static void UltrasonicLoop(void* arg);
static SemaphoreHandle_t UltrasonicMutex = NULL;
uint16_t distance;


//#define AP
//#define CLI
#define CLI_M5STAMP
//#define CLI_M5ATOM

#ifdef AP
  const char *ssid = "XIAO_ESP32C3";
  static const char SERVER[] = "192.168.4.2"; 
  static int LED_PIN = D10;
#elif defined CLI
  static const char WIFI_SSID[] = "XIAO_ESP32C3";
  static const char WIFI_PASSPHRASE[] = "";
  static const char SERVER[] = "192.168.4.1";
  static int LED_PIN = D10;
#elif defined CLI_M5STAMP
  #include <Adafruit_NeoPixel.h>
  #define LED_PIN 2
  static int BUZZER_PIN = 20;

  Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);
  #include "Ultrasonic.h"
  portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
  
  Ultrasonic ultrasonic(0);
  static const char WIFI_SSID[] = "XIAO_ESP32C3";
  static const char WIFI_PASSPHRASE[] = "";
  static const char SERVER[] = "192.168.4.1";
#elif defined CLI_M5ATOM
  #include <M5Atom.h>    //Atomのヘッダファイルを準備
  // FastLEDライブラリの設定（CRGB構造体）
  CRGB dispColor(uint8_t r, uint8_t g, uint8_t b) {
    return (CRGB)((r << 16) | (g << 8) | b);
  }
  static const char WIFI_SSID[] = "XIAO_ESP32C3";
  static const char WIFI_PASSPHRASE[] = "";
  static const char SERVER[] = "192.168.4.1";

#else
#endif

static constexpr unsigned long INTERVAL = 1000;	// [msec.]


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring WiFi...");

  #ifdef AP
    pinMode(LED_PIN, OUTPUT);
    WiFi.mode( WIFI_AP );//for AP mode
    int a= esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B );
    int b = esp_wifi_config_80211_tx_rate(WIFI_IF_AP,WIFI_PHY_RATE_1M_L);
    WiFi.softAP(ssid);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  #elif defined(CLI)
    pinMode(LED_PIN, OUTPUT);
    WiFi.mode(WIFI_STA);
    if (WIFI_SSID[0] != '\0')WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
    else WiFi.begin();
  #elif defined(CLI_M5STAMP)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN,LOW);
    pixels.begin();
    WiFi.mode(WIFI_STA);
    if (WIFI_SSID[0] != '\0')WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
    else WiFi.begin();
  #elif defined(CLI_M5ATOM)
    M5.begin(true, false, true);
    M5.dis.drawpix(0, dispColor(0, 0, 0));    
    WiFi.mode(WIFI_STA);
    if (WIFI_SSID[0] != '\0')WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
    else WiFi.begin();
  #else
  #endif
  

  UltrasonicMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(UltrasonicLoop, TASK_NAME_IMU, TASK_STACK_DEPTH, NULL, 2, NULL, TASK_DEFAULT_CORE_ID);

}

void LED_ONOFF(){
  #ifdef CLI_M5STAMP
    digitalWrite(BUZZER_PIN,HIGH);
    pixels.setPixelColor(0, pixels.Color(0, 255, 255));    //Colorメソッド内の引数の順番は赤、緑、青
    pixels.show();delay(100);
    digitalWrite(BUZZER_PIN,LOW);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));    //明るさを3原色全て0にして消灯
    pixels.show();delay(100);
  #elif defined(CLI_M5ATOM)
    M5.dis.drawpix(0, dispColor(0, 0, 255));delay(100); //LED（指定色
    M5.dis.drawpix(0, dispColor(0, 0, 0));delay(100); //LED（指定色）
  #else
    digitalWrite(LED_PIN, HIGH);delay(100);
    digitalWrite(LED_PIN, LOW);delay(100);
  #endif

}

void loop()
{
#if defined(CLI)||defined(CLI_M5STAMP)||defined(CLI_M5ATOM)
	static int count = 0;
	////////////////////////////////////////
	// Check Wi-Fi connection status
	// const bool wifiStatus = WiFi.status() == WL_CONNECTED;
	// const int wifiRssi = WiFi.RSSI();
	////////////////////////////////////////
	// Execute ping
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting to WiFi...");
    WiFi.reconnect();
  }else if(WiFi.status() == WL_CONNECTED){
    const bool wifiStatus = WiFi.status() == WL_CONNECTED;
	  const int wifiRssi = WiFi.RSSI();
    const bool pingResult = Ping.ping(SERVER, 1);
    const float pingTime =  Ping.averageTime();
    if(pingResult){
      if(WiFi.RSSI() > -85) LED_ONOFF();
      else if(WiFi.RSSI() > -90) {LED_ONOFF(); LED_ONOFF();}
      else if(WiFi.RSSI() > -95) {LED_ONOFF(); LED_ONOFF(); LED_ONOFF();}

      Serial.print(count);
      Serial.print('\t');
      Serial.print(wifiStatus ? 1 : 0);
      Serial.print('\t');
      Serial.print(wifiRssi);
      Serial.print('\t');
      Serial.print(pingResult ? 1 : 0);
      Serial.print('\t');
      Serial.println(pingTime);
      count++;
    }
  }
	delay(INTERVAL);
#endif

/*
#if defined(CLI_M5STAMP)
  portENTER_CRITICAL_ISR(&mutex);
  uint16_t Distance = ultrasonic.MeasureInCentimeters()*10;
  Serial.print(Distance);
  portEXIT_CRITICAL_ISR(&mutex);
#endif
*/
}

static void UltrasonicLoop(void* arg){
  while (1) {
    uint32_t entryTime = millis();
    distance = ultrasonic.MeasureInCentimeters()*10;

    uint32_t elapsed_time = millis() - entryTime; 
    Serial.println(String(distance)+","+String(elapsed_time)+","+String(millis()));
    
    int32_t sleep = TASK_SLEEP_ULTRASONIC  - elapsed_time;
    vTaskDelay((sleep > 0) ? sleep : 0);
  }
}