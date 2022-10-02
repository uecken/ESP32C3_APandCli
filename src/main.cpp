#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP32Ping.h>

//#define AP
#ifdef AP
const char *ssid = "XIAO_ESP32C3";
static const char SERVER[] = "192.168.4.2";
#endif

#define CLI
#ifdef CLI
static const char WIFI_SSID[] = "XIAO_ESP32C3";
static const char WIFI_PASSPHRASE[] = "";
static const char SERVER[] = "192.168.4.1";
#endif


static int LED_PIN = D10;
static constexpr unsigned long INTERVAL = 1000;	// [msec.]


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  #ifdef AP
  WiFi.mode( WIFI_AP );//for AP mode
  int a= esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B );
  int b = esp_wifi_config_80211_tx_rate(WIFI_IF_AP,WIFI_PHY_RATE_1M_L);
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  #endif
  #ifdef CLI
	WiFi.mode(WIFI_STA);
	if (WIFI_SSID[0] != '\0'){	
		WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
	}else{
		WiFi.begin();
	}
  #endif
  pinMode(LED_PIN, OUTPUT);
}

void LED_ONOFF(){
  digitalWrite(LED_PIN, HIGH);delay(100);
  digitalWrite(LED_PIN, LOW);delay(100);  
}

void loop()
{
#ifdef CLI
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
    const bool pingResult = Ping.ping(SERVER, 1);
    const float pingTime =  Ping.averageTime();
    if(pingResult){
      if(WiFi.RSSI() > -85){
        LED_ONOFF();
      }else if(WiFi.RSSI() > -90) {
        LED_ONOFF(); LED_ONOFF();
      }else if(WiFi.RSSI() > -95) {
        LED_ONOFF(); LED_ONOFF(); LED_ONOFF();
      }
      Serial.print(count);
      Serial.print('\t');
      //Serial.print(wifiStatus ? 1 : 0);
      //Serial.print('\t');
      //Serial.print(wifiRssi);
      //Serial.print('\t');
      Serial.print(pingResult ? 1 : 0);
      Serial.print('\t');
      Serial.println(pingTime);
      count++;
    }
  }
	delay(INTERVAL);
#endif
}