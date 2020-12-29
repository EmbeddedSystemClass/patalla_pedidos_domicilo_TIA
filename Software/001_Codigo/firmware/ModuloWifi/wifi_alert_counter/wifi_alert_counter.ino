#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#include <ESP8266FtpServer.h>
#include <FS.h>
#include <MusicEngine.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "ArduinoJson.h"

#define BUZ_PIN 5
#define DisplaySerial Serial
#define LED_BUILTIN 2 
#define BTN_DEFAULT 13

const long interval_1 = 10000;  
const long interval_2 = 1000;     

unsigned long previousTimer1 = 0;
unsigned long previousTimer2 = 0;

char num = 0x00;
String get_data_url;

int configuration_default = 1;
String configuration_uri_end_point;
String configuration_store;

String wifi_ip, wifi_mask, wifi_gateway, wifi_dns1, wifi_dns2;
String wifi_ssid;
String wifi_pass;
int wifi_dhcp;

int En_Proceso = 0; 
bool success = true;

String HTMLpage = "";
String HostNameOTA = "";
char *client_id = "OTA_Name"; 

MusicEngine music(BUZ_PIN);

ESP8266WebServer  Server;          
AutoConnect      Portal(Server);
AutoConnectConfig Config; 

FtpServer ftpSrv;

HTTPClient http;

ESP8266WebServer Webserver(80);

void http_request(){
  http.begin(get_data_url);  
  int httpCode = http.GET(); 
    
  if (httpCode > 0){ 
    String payload = http.getString();  
    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(8) + 2230;
    DynamicJsonDocument root(capacity);
    deserializeJson(root, payload);
    En_Proceso = root["En Proceso"];  
    //Serial.println(En_Proceso); 
  }
    
  http.end();
}

bool loadConfig() {
  File configFile = SPIFFS.open("/data_config.json", "r");
  if (!configFile) {
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(8) + 464;
  DynamicJsonDocument  doc(capacity);
  deserializeJson(doc, buf.get());
  
  JsonObject configuration = doc["configuration"];
  JsonObject wifi = doc["wifi"];
  const char* uri_end_point = configuration["uri_end_point"];
  configuration_uri_end_point = uri_end_point;
  const char* store = configuration["store"]; 
  configuration_store = store;
  configuration_default = configuration["default"]; 
  
  const char* _ssid = wifi["ssid"];  
  wifi_ssid = _ssid;
  const char* _pass = wifi["pass"]; 
  wifi_pass = _pass;
  const char* _ip = wifi["ip"]; 
  wifi_ip = _ip;
  const char* _mask = wifi["mask"];
  wifi_mask = _mask;
  const char* _gateway = wifi["gateway"]; 
  wifi_gateway = _gateway;
  const char* _dns1 = wifi["dns1"]; 
  wifi_dns1 = _dns1;
  const char* _dns2 = wifi["dns2"]; 
  wifi_dns2 = _dns2;
  wifi_dhcp = wifi["dhcp"]; 
  delay(20);
  
  configFile.close();  
  return true;
}

bool saveConfig(){
  String write_data;
  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(8)+ 464;
  DynamicJsonDocument  doc(capacity);
  JsonObject configuration = doc.createNestedObject("configuration");
  configuration["uri_end_point"] = configuration_uri_end_point;
  configuration["store"] = configuration_store;
  configuration["default"] = configuration_default;
 
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ssid"] = wifi_ssid; 
  wifi["pass"]= wifi_pass; 
  wifi["ip"] = wifi_ip; 
  wifi["mask"] = wifi_mask;
  wifi["gateway"] = wifi_gateway;
  wifi["dns1"] = wifi_dns1;
  wifi["dns2"] = wifi_dns2; 
  wifi["dhcp"] = wifi_dhcp;

  File configFile = SPIFFS.open("/data_config.json", "w");
  if (!configFile) {
    return false;
  }
  serializeJson(doc, write_data);
  int bytesWritten = configFile.print(write_data);
  configFile.close();
  return true;
}

void deleteAllCredentials(){
  AutoConnectCredential credential;
  station_config_t config;
  uint8_t ent = credential.entries();

  while (ent--) {
    credential.load(ent, &config);
    credential.del((const char*)&config.ssid[ent]);
  }
}

int getIpBlock(int index, String str){
  char separator = '.';
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = str.length()-1;
  
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(str.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  
  return found>index ? str.substring(strIndex[0], strIndex[1]).toInt() : 0;
}

IPAddress str2IP(String str){

  IPAddress ret( getIpBlock(0,str),getIpBlock(1,str),getIpBlock(2,str),getIpBlock(3,str));
  return ret;
}

void setup(){
  delay(5000);
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(BTN_DEFAULT, INPUT); 
  pinMode(BUZ_PIN, OUTPUT);
  digitalWrite(BUZ_PIN, HIGH);
  DisplaySerial.begin(9600);
  DisplaySerial.write(0x00);
  HTMLpage += "<head><title>Alerta de Pedidos</title></head><h3>Display Alerta de Pedidos</h3><p><a href=\"Reboot\"><button> REBOOT </button></a></p>";
  if (!SPIFFS.begin()) {
    DisplaySerial.write('R'); //Reset Display
    ESP.restart();
  }
  delay(1000);

  if (!loadConfig()) {
    DisplaySerial.write('R'); //Reset Display
    ESP.restart();
  } else{
    get_data_url = configuration_uri_end_point+configuration_store; 
  }

  if(!digitalRead(BTN_DEFAULT)){
    int reset_cont = 0;  
    while(!digitalRead(BTN_DEFAULT)){
      delay(100);
      reset_cont++;
      if(reset_cont >100){
        configuration_uri_end_point = "http://mis.tia.com.ec/consultas/uplink/?sucursal=";
        configuration_store = "320";
        configuration_default = 1;
        
        wifi_ssid = "NETLIFE-QUINDE";        // SSID
        wifi_pass = "0931017271";            // Password
        wifi_dhcp = 0;                       // DHCP == 1 -->Enable; DHCP == 0 -->Disable     
        wifi_ip = " ";                       // Ip
        wifi_mask = " ";                     // Mask
        wifi_gateway = " ";                  // Gateway
        wifi_dns1 = " ";                     // DNS1
        wifi_dns2 = " ";                     // DNS2
        saveConfig();
        delay(200);
        DisplaySerial.write('R'); //Reset Display
        ESP.restart();
      }
    }
  }
  
  if(configuration_default == 0){
    WiFi.disconnect();
    //Serial.print(str2IP(wifi_ip));
    if(wifi_dhcp){
      WiFi.config(str2IP(wifi_ip), str2IP(wifi_gateway), str2IP(wifi_mask), str2IP(wifi_dns1), str2IP(wifi_dns2)); 
    }
    delay(1000);
    
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    WiFi.mode(WIFI_STA);
    int t_cont = 0;
    while(WiFi.status()  != WL_CONNECTED){ 
      music.play("T240");  
      delay(2000);
      t_cont++;
      if(t_cont > 5){
        delay(2000);
        DisplaySerial.write('R'); //Reset Display
        ESP.restart();
      }
    }
    
    ftpSrv.begin("tia","tia");
    delay(100);
    http_request();
  }else{
    for(int j=0;j<=16;j++){
      DisplaySerial.write('Q'); //Display Config Mode
      delay(500);
    }
    deleteAllCredentials();
    Config.apid = "Pedidos-Online";
    Config.psk = "Tia12345";
    Config.immediateStart = true;
    Config.autoRise = true;
    Config.autoReconnect = false;
    Config.hostName = "esp32-01";
    Config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_RESET;
    Portal.config(Config);
    
    if(Portal.begin()){
      delay(10000);
      AutoConnectCredential current_credential;
      station_config_t wifi_config;
      uint8_t n_ent = current_credential.entries();
      current_credential.load(n_ent - 1, &wifi_config);
      wifi_ssid = (char *)wifi_config.ssid;        
      wifi_pass = (char *)wifi_config.password;            
      wifi_dhcp = (int)wifi_config.dhcp;     
      wifi_ip = WiFi.localIP().toString();                                    
      
      if((int)wifi_config.dhcp){                    
        wifi_mask = WiFi.subnetMask().toString();                     
        wifi_gateway = WiFi.gatewayIP().toString();                
        wifi_dns1 = WiFi.dnsIP(0).toString();                   
        wifi_dns2 = WiFi.dnsIP(1).toString();    
      }
      
      configuration_default = 0;
      saveConfig();
      delay(1000);
      DisplaySerial.write('R'); //Reset Display
      ESP.restart();
    }
  }
  for(int i=0;i<=5;i++){
    success = Ping.ping(WiFi.gatewayIP(),2);
    delay(500);
  }
  DisplaySerial.write(num);
  
  Webserver.on("/", [](){
    Webserver.send(200, "text/html", HTMLpage);
  });
  Webserver.on("/Reboot", [](){
    Webserver.send(200, "text/html", HTMLpage+"<p>Rebooting...</p>");
      delay(1000);
      DisplaySerial.write('R'); //Reset Display
      ESP.restart();
  });
 
  Webserver.begin();
  HostNameOTA = "Dispaly(Suc-"+configuration_store+")";
  ArduinoOTA.onStart([]() {
    for(int j=0;j<=16;j++){
      DisplaySerial.write('Q'); //Display Config Mode
      delay(100);
    }
  });
  ArduinoOTA.onEnd([](){
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error){
//    Serial.printf("Error[%u]: ", error);
//    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname(HostNameOTA.c_str());
  ArduinoOTA.begin();
}

void loop(){
  if(configuration_default == 0){
    //********************************************
    //********** Get Data from EndPoint **********
    //********************************************
    unsigned long timer1 = millis();                                                
    if ((timer1 - previousTimer1 >= interval_1)){
      previousTimer1 = timer1;
      http_request();
      DisplaySerial.write((char)En_Proceso);
    }
    //**********************************
    //********** Active Alarm **********
    //**********************************
    unsigned long timer2 = millis();                                                
    if (En_Proceso > 0 && (timer2 - previousTimer2 >= interval_2)){
      previousTimer2 = timer2;
      music.play("G");
      num++;
    }else if(En_Proceso == 0){
      digitalWrite(BUZ_PIN, HIGH);
    }

    if (WiFi.status() != WL_CONNECTED || !success){
      DisplaySerial.write('R'); //Reset Display
      ESP.restart();
    }
    Webserver.handleClient();
    ftpSrv.handleFTP();
    ArduinoOTA.handle();
  }else{
    Portal.handleClient();
  }
}

