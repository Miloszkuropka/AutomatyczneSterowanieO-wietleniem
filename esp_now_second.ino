/*
  ESP-NOW Multi Unit Demo
  esp-now-multi.ino
  Broadcasts control messages to all devices in network
  Load script on multiple devices

  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>

// Define LED and pushbutton state booleans

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

int light = 0;

int lastState = HIGH; // the previous state from the input pin
int currentState;     // the current reading from the input pin

// Define LED and pushbutton pins
//#define STATUS_LED 15
//#define STATUS_BUTTON 5
#define BUTTON_PIN 32


void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}


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
  if (strcmp("onon", buffer) == 0)
  {
    light = 2;
  }
  else if ((strcmp("onoff", buffer) == 0)){
    light = 1;
  }
  else if ((strcmp("offoff", buffer) == 0)){
    light = 0;
  }
  else
  {
    light = 3;
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

int block = 0;
int lastblock = 0;
int lastlight = 0;

void display()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  if(light == 0){
    lcd.print("Lazienka wolna");
  }
  else if (light == 2){
  lcd.print("Lazienka zajeta");
  lcd.setCursor(0, 1);
  lcd.print("Umywalka zajeta");
  }
  else if (light == 1){
    lcd.print("Lazienka zajeta");
    lcd.setCursor(0, 1);
    lcd.print("Umywalka wolna");
  }
  if (block == 1){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MANUAL ON");
  }
  else if (block == 2){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MANUAL OFF");
  }
}

void setup()
{

  // Set up Serial Monitor
  Serial.begin(115200);
  lcd.init();                      
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AUTOMATYCZNE");
  lcd.setCursor(0, 1);
  lcd.print("OSWIETLENIE");
  
  delay(2000);

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

  pinMode(BUTTON_PIN, INPUT_PULLUP);
   
  display();
}

void loop()
{
  Serial.println(light);

currentState = digitalRead(BUTTON_PIN);

  if(lastState == LOW && currentState == HIGH)
  {
    Serial.println("The state changed from LOW to HIGH");
    if (block == 0) { block = 1;
    broadcast("blockoff");
    }
    else if (block == 1){ block = 2;
    broadcast("blockon");
    }
    else { block = 0;
    broadcast("noblock");
    }
  }
  // save the last state
  lastState = currentState;
  if (lastblock != block || lastlight != light)
  {
     display();
  }

    lastblock = block;
    lastlight = light;
    
    // Delay to avoid bouncing
    delay(400);
  }
  
