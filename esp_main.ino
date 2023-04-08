/*
  ESP-NOW Multi Unit Demo
  esp-now-multi.ino
  Broadcasts control messages to all devices in network
  Load script on multiple devices

  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include "DFRobot_Microwave_Radar_Module.h"
#include <WiFi.h>
#include <esp_now.h>


// Define LED and pushbutton state booleans
//bool blockON = false;
//bool blockOFF = false;

// Define LED and pushbutton pins
#define SOUND_SPEED 0.034
#define LED_GREEN 32
#define LED_YELLOW 15
#define SOUND_TRIG 14
#define SOUND_RECIVE 27
DFRobot_Microwave_Radar_Module Sensor(/*hardSerial =*/&Serial1, /*rx =*/12, /*tx =*/13);


void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

// Define data structure
typedef struct struct_message {
  int mirror;
  int main;
} struct_message;
long duration;
int distance;

//pir detector
int calibrationTime = 15;
int pirPin = 25; 

int present = 0;
int distanceCm = 500;
 
int iMirror = 10;
int iPresent = 10;
int light = 0;
int block = 0;

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
// Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  // Make sure we are null terminated
  buffer[msgLen] = 0;

  // Format the MAC address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);

  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);

  // Check switch status
  if (strcmp("blockoff", buffer) == 0)
  {
    block = 1;
  }
  else if (strcmp("blockon", buffer) == 0){
    block = 2;
  }
  else if (strcmp("noblock", buffer) == 0){
    block = 0;
  }
  else
  {
    light = 0;
  }
}


void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// Called when data is sent
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message)
// Emulates a broadcast
{
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  // Print results to serial monitor
  if (result == ESP_OK)
  {
    Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESP-NOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}

void setup()
{

  // Set up Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Set ESP32 in STA mode to begin with
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");

  // Print MAC address
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Disconnect from WiFi
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }

  pinMode(SOUND_TRIG, OUTPUT); // Sets the trigPin as an Output
  pinMode(SOUND_RECIVE, INPUT); // Sets the echoPin as an Input

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  pinMode(pirPin, INPUT);

  Serial.print("calibrating sensor ");
    for(int i = 0; i < calibrationTime; i++){
      Serial.print(".");
      delay(1000);
      }
    Serial.println(" done");

  digitalWrite(pirPin, LOW);

  while ( !( Sensor.begin() ) ) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000); 
  }
  Serial.println("Begin ok!");
  Sensor.factoryReset();
  Sensor.detRangeCfg(4);
  Sensor.setSensitivity(4);
  Sensor.outputLatency(1, 3);
  Sensor.setGpioMode(1);

  // Serial port for debugging purposes
  Serial.println();
  
}

int buffor1 = 0;
int buffor2 = 0;

void loop()
{
  digitalWrite(SOUND_TRIG, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(SOUND_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(SOUND_TRIG, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(SOUND_RECIVE, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  int present1 = Sensor.readPresenceDetection();
  Serial.println(present1);
  Serial.println(distanceCm);
  int ruch = digitalRead(pirPin); 
  if (block != 0)
  {
    if (block == 1){
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, LOW);
      buffor2 = 3;
      present = 0;
      iMirror = 10;
      iPresent = 10;
    }
    else if (block == 2) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_YELLOW, HIGH);
    }
  }
  else{
    if(ruch == HIGH)                      //wyświetlenie informacji na monitorze szeregowym
    {                                     //stan wysoki oznacza wykrycie ruchu, stan niski - brak ruchu
      iPresent = 0;
      Serial.println("RUCH WYKRYTY!");
    }
    else  {
      iPresent++;
      Serial.println("brak ruchu");}
    
    
    if (distanceCm > 80 && distanceCm < 1197) iMirror++;
    else iMirror = 0; 
    if ((present1 == 1 || iPresent < 7 || iMirror < 7) && buffor1 == 0) {
      present = 1;
      Serial.println("green led on");
      digitalWrite(LED_GREEN, LOW);
      buffor2 = 3;
    }
    else {
      present = 0;
      Serial.println("green led off");
      digitalWrite(LED_GREEN, HIGH);
        if (buffor2 > 0) { 
        buffor1 = 1;
        buffor2--;
        }
        else{
          Serial.println("buffor zero");
          buffor1 = 0;
          buffor2 = 0;
        }

      }

    
    if (iMirror < 7 && present == 1) {
      Serial.println("yellow led on");
    digitalWrite(LED_YELLOW, LOW);
    }
    else {
      Serial.println("yellow led off");
    digitalWrite(LED_YELLOW, HIGH);
    }
  }
        if (present == 1 && iMirror < 7)
        {
          broadcast("onon");
        }
        else if (present == 1 && iMirror >= 7){
          broadcast("onoff");
        }
        else
        {
          broadcast("offoff");
        }
        // Delay to avoid bouncing
      delay(500);
  }  
  
    
  
