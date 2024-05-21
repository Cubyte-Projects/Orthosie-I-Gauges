#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

// You will need to enter your router data in <data/wifimanager.html>
IPAddress localIP;
//IPAddress localIP(192, 168, xxx, xxx);      // hardcoded in <data/wifimanager.html>

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, xxx, xxx); // hardcoded in <data/wifimanager.html>
IPAddress subnet(255, 255, 255, 0);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

// Timer variables (wifimanager)
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

//**********************************************************************************************
//**********************************************************************************************
void projectInfo(){
Serial.print("Project Name = ");Serial.println("Wurth HIDS Guages_LED + WifiManager & OTA ");
return;
}
//***************************************************************************************
//***************************************************************************************
#include <Wire.h>
    #define I2C_SCL  3
    #define I2C_SDA 10
//***************************************************************************************
//***************************************************************************************

#include "WSEN_HIDS.h"


Sensor_HIDS sensor;

int sensor_ID;
//*************************************************************************************** 
//********************************************************************************************** 

///==============================================================================================
///==============================================================================================
// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){

  readings["temperature"]   =  String(sensor.get_Temperature());
  //Print the Temperature value on the serial monitor
  Serial.print("The Temperature is: ");
  Serial.print(readings["temperature"]);
  Serial.println(" degC"); 

  readings["humidity"]      =  String(sensor.get_Humidity());
  Serial.print("The Humidity is: ");
  //Print the Humidity value on the serial monitor
  Serial.print(readings["humidity"]);
  Serial.println(" %");
 
  String jsonString = JSON.stringify(readings);
  Serial.print("jsonString = ");
  Serial.println(jsonString);
  return jsonString;

}

void setup() {

  // Serial port for debugging purposes
  Serial.begin(115200);
  projectInfo();
  initSPIFFS();
  initWiFi();  

//***************************************************************************************
//***************************************************************************************
    Wire.begin(I2C_SDA, I2C_SCL);
  //sensor.set_continuous_mode(ODR); DOES NOT ADJUST CTRL_1 ?
  ////// Configure HIDS CTRL_1 to OneShot mode /////
    Wire.beginTransmission(HIDS_ADDRESS_I2C_0);  //     HIDS(I2CADDR) 0x5F
    Wire.write(0x20);  // write to HIDS register  CTRL-1 REG    0x20
    Wire.write(0x84);  // write to HIDS CTRL-1 REG (10000100b)(PD=1,XXXX,BDU=1,ODR=0) One Shot Mode
    Wire.endTransmission();
//***************************************************************************************
// NOTE, ArduinoPlatform.cpp has also been altered to use  I2C_SCL  3, I2C_SDA 10   
//***************************************************************************************

  // Initialize the I2C interface for HIDS
  sensor.init(HIDS_ADDRESS_I2C_0);    

  // Get the device ID for this sensor
  sensor_ID = sensor.get_DeviceID();
  Serial.print("Sensor ID: ");

  // Print the device ID in hexadecimal on the serial monitor
  Serial.print(sensor_ID, HEX);
  Serial.println();

  // Check if the determined device ID matches the correct device ID (->0xBC) of this sensor
  if (sensor_ID == HIDS_DEVICE_ID_VALUE)
  {
    Serial.println("Communication successful !");
  } else
  {
    Serial.println("No communication with the device !");
  }  
//***************************************************************************************
//***************************************************************************************

// Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);

  //comment-out the next four lines when done
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  AsyncElegantOTA.begin(&server);      // Start AsyncElegantOTA
  //ElegantOTA.begin(&server);         // Start ElegantOTA

  // Start server
 server.begin();
 Serial.println("HTTP Wurth-Environment Server started");
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
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
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });

  }

}

void loop(){ 
   AsyncElegantOTA.loop();            

//***************************************************************************************
//***************************************************************************************

/*
int i = 0;
while(i < 3){
  Serial.print("The Humidity is: ");
  //Print the Humidity value on the serial monitor
  Serial.print(sensor.get_Humidity());
  Serial.println(" %");
  //Print the Temperature value on the serial monitor
  Serial.print("The Temperature is: ");
  Serial.print(sensor.get_Temperature());
  Serial.println(" degC");
   i++;
}
*/  
 UpdateESP:
        Serial.println("Updating ESP Environment Server");
        events.send(getSensorReadings().c_str(),"new_readings");  
        
delay(2000); // Waste 10 Seconds

// Reset ONE-SHOT ... MODE
    Wire.beginTransmission(HIDS_ADDRESS_I2C_0);  //     HIDS(I2CADDR) 0x5F
    Wire.write(0x21);  // write to HIDS register  CTRL-2 REG    0x21
    Wire.write(0x01);  // write to HIDS CTRL-2 REG (0000001b)(BOOT=0,XXXXX,HEATER=0,ONESHOT=1-start new measurment)
    Wire.endTransmission();
//***************************************************************************************
//***************************************************************************************
}            
