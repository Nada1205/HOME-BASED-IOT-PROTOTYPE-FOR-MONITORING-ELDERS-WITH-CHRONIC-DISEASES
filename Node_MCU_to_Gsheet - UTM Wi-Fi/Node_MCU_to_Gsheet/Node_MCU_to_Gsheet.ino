#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <String.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
#include "c_types.h"
}

// SSID to connect to
char ssid[] = "UTMWiFi"; // Wi-Fi Name
char username[] = "ahmed2000"; // Username
char identity[] = "";
char password[] = "Adam_H_1412"; // Password

uint8_t target_esp_mac[6] = {0x24, 0x0a, 0xc4, 0x9a, 0x58, 0x28};


//D6 = Rx & D5 = Tx
SoftwareSerial nodemcu(D6, D5);

#define No_ECG  1000

uint8_t MODE;
int ECG_i = 0;
String ECG[No_ECG];

char BP_Flag = 0;

#define ON_Board_LED 2  //--> Defining an On Board LED, used for indicators when the process of connecting to a wifi router


int A=0, B=100, C=200, D=300;

//---------------------------------------- Host & https Port 
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------

WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID_BP = "AKfycbypjkOTLH0p6X8-gCjGneUv_poM7vb4t6LQDj0VVlTXMV1G__55d4oup4M0hjBV1TRsDg";  //--> spreadsheet script ID
String GAS_ID_ECG = "AKfycbzbAQvFR2UVSyT-SHZDdV1UJBePw7vi0CvBMnQshP9JwpTMRmxvONZE0Rm0hr1wnayYjA"; //

void setup() {
  // put your setup code here, to run once:

  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(1000);
  Serial.setDebugOutput(true);
  Serial.printf("SDK version: %s\n", system_get_sdk_version());
  Serial.printf("Free Heap: %4d\n",ESP.getFreeHeap());
  
  // Setting ESP into STATION mode only (no AP mode or dual mode)
  wifi_set_opmode(STATION_MODE);

  struct station_config wifi_config;

  memset(&wifi_config, 0, sizeof(wifi_config));
  strcpy((char*)wifi_config.ssid, ssid);
  strcpy((char*)wifi_config.password, password);

  wifi_station_set_config(&wifi_config);
  wifi_set_macaddr(STATION_IF,target_esp_mac);
  

  wifi_station_set_wpa2_enterprise_auth(1);

  // Clean up to be sure no old data is still inside
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_clear_enterprise_identity();
  wifi_station_clear_enterprise_username();
  wifi_station_clear_enterprise_password();
  wifi_station_clear_enterprise_new_password();
  
  wifi_station_set_enterprise_identity((uint8*)identity, strlen(identity));
  wifi_station_set_enterprise_username((uint8*)username, strlen(username));
  wifi_station_set_enterprise_password((uint8*)password, strlen((char*)password));

  wifi_station_connect();
    
  pinMode(ON_Board_LED,OUTPUT); //--> On Board LED port Direction output
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off Led On Board

  //----------------------------------------Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //----------------------------------------Make the On Board Flashing LED on the process of connecting to the wifi router.
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
    //----------------------------------------
  }
  //----------------------------------------

  digitalWrite(ON_Board_LED, HIGH); //--> Turn off the On Board LED when it is connected to the wifi router.
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------

  Serial.begin(9600);
  nodemcu.begin(9600);
  delay(500);

  client.setInsecure();
}

void loop() {
  digitalWrite(ON_Board_LED, LOW);
  StaticJsonDocument<1000> DATA;

  if (nodemcu.available()) DeserializationError error = deserializeJson(DATA, nodemcu);  

  MODE = DATA["mode"];
  if( DATA["mode"] == 1 && BP_Flag == 0){
    sendDataBPT(String(DATA["sys"]), String(DATA["dia"]), String(DATA["hr"]), String(DATA["spo2"]));
    BP_Flag = 1;
  }

  if( DATA["mode"] == 2){
//    sendDataECG(String(DATA["ecg"]));
    ECG[ECG_i] = String(DATA["ecg"]);
    Serial.print(ECG_i);
    Serial.print(" ");
    Serial.print(ECG[ECG_i]);
    Serial.print("\n");
  ECG_i++;
  }

  if(DATA["mode"] == 0){
      sendDataECG(ECG, 0);
      delay(50);
      sendDataECG(ECG, 20);
      delay(50);
      sendDataECG(ECG, 40);
      delay(50);
      sendDataECG(ECG, 60);
      delay(50);
      sendDataECG(ECG, 80);
      delay(50);
      sendDataECG(ECG, 100);
      delay(50);
      sendDataECG(ECG, 120);
      delay(50);
      sendDataECG(ECG, 140);
    ECG_i = 0;
  }
}

//=======================================================================//
//=======================================================================//

void sendDataBPT(String sys, String dia, String hr, String spo2) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  
  //----------------------------------------
  
  //----------------------------------------Processing data and sending data
  String string_sys =  String(sys);
  String string_dia =  String(dia); 
  String string_hr = String(hr);
  String string_spo2 = String(spo2);
  
  String url = "/macros/s/" + GAS_ID_BP + "/exec?sys=" + string_sys + "&dia=" + string_dia + "&hr=" + string_hr + "&spo2=" + string_spo2;
  
  Serial.print("requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");
  
  Serial.println("request sent");
  //----------------------------------------
  
  //----------------------------------------Checking whether the data was sent successfully or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
} 

//=======================================================================//
//=======================================================================//


void sendDataECG(String ecg[200], int x) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------
  
  //----------------------------------------Processing data and sending data
  
//  String string_ecg =  String(ecg);
  for(int i=x ; i<x+20 ; i++){
    Serial.print(ecg[i]);
    Serial.print("\n");
  }
  String url = "/macros/s/" + GAS_ID_ECG + "/exec?aa=" + ecg[x]  + "&bb=" + ecg[x+1] + "&cc=" + ecg[x+2]  + "&dd=" + ecg[x+3] 
   + "&ee=" + ecg[x+4]  + "&ff=" + ecg[x+5]  + "&gg=" + ecg[x+6]  + "&hh=" + ecg[x]+7  + "&ii=" + ecg[x+8]  + "&jj=" + ecg[x+9] 
    + "&kk=" + ecg[x+10]  + "&ll=" + ecg[x+11]  + "&mm=" + ecg[x+12]  + "&nn=" + ecg[x+13]  + "&oo=" + ecg[x+14]  + "&pp=" + ecg[x+15]
      + "&qq=" + ecg[x+16]  + "&rr=" + ecg[x+17]  + "&ss=" + ecg[x+18]  + "&tt=" + ecg[x+19];
  
  Serial.print("requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
} 
//==============================================================================