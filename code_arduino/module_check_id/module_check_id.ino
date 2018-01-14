#include <SPI.h>
#include <MFRC522.h>

#include <LiquidCrystal.h>

#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT.h>

#include <ESP8266WiFi.h>

//#define SECURE_CO

#ifdef SECURE_CO
  #include <WiFiClientSecure.h>
  WiFiClientSecure wifi_client;
#else
  #include <WiFiClient.h>
  WiFiClient wifi_client;
#endif

const char* ssid     = "SFR-5ba8";
const char* password = "DLTYQGFM4XFE";

const char* mqtt_broker_host = "mottet.xyz";
const uint16_t mqtt_broker_port = 8883;

// mottet.xyz SHA1 fingerprint
const char* fingerprint = "B0 00 9C 58 D8 01 CF D2 6D F3 A7 53 BD E3 D8 6B F9 FD E3 0F";

#define MODULE_ID "AUTH_1337"

// Variables to handle connection to MQTT server
Adafruit_MQTT_Client mqtt(&wifi_client, mqtt_broker_host, mqtt_broker_port, "Module " MODULE_ID);

Adafruit_MQTT_Publish pub = Adafruit_MQTT_Publish(&mqtt, "employee_badging_information");

Adafruit_MQTT_Subscribe authorised = Adafruit_MQTT_Subscribe(&mqtt, MODULE_ID);

// LCD Screen init
LiquidCrystal lcd(15, 0, 16, 2, 5, 4);

// RFID reader
#define RST_PIN   3     // Configurable, see typical pin layout above
#define SS_PIN    1   // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

void write_lcd(String line1 = "", String line2 = "") {
  if (line1 != "") {
    lcd.setCursor(0, 0);
    lcd.print(line1 + "                ");
  }
  if (line2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(line2 + "                ");
  }
}

void wifi_connect() {
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  write_lcd("", "WiFi connection ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

#ifdef SECURE_CO
void verifyFingerprint() {

  //Serial.print("Secure connection to ");
  //Serial.println(mqtt_broker_host);
  while (! wifi_client.connect(mqtt_broker_host, mqtt_broker_port, MODULE_ID)) {
    //Serial.print(".");
    delay(2000);
  }

  if (wifi_client.verify(fingerprint, mqtt_broker_host)) {
    //Serial.println("Connection secure.");
  } else {
    //Serial.println("Connection insecure! Halting execution.");
    while(1);
  }

}
#endif

void mqtt_connect() {
  write_lcd("", "MQTT connection ");
  while(!mqtt.connected()) {
    if (!mqtt.connect()) {
      delay(3000);
    }
  }
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.print(MODULE_ID);

  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card

  wifi_connect();
  
#ifdef SECURE_CO
  verifyFingerprint();
#endif

  mqtt.subscribe(&authorised);
  mqtt_connect();

  write_lcd("Present your", "ID card...");
}

String decToHex(char _byte)
{
  const char* tab = "0123456789ABCDEF";
  return String(tab[_byte >> 4]) + String(tab[_byte & 0x0F]);
}

void loop() {
  if (!mqtt.connected()) {
    mqtt_connect();
  }

    // Look for new cards, and select one if present
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(50);
    return;
  }
  
  
  String cur_str = "";
  // Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cur_str += (i ? ' ' + decToHex(mfrc522.uid.uidByte[i]) : decToHex(mfrc522.uid.uidByte[i]));
  } 

  write_lcd("Sending:", cur_str);
  
  Adafruit_MQTT_Subscribe *subscription;
  
  // Empty the queue
  while(subscription = mqtt.readSubscription(0));
  
  if (pub.publish(String("{\"moduleID\":\"" + String(MODULE_ID) + "\",\"cardID\":\"" + cur_str + "\"}").c_str())) {
    subscription = mqtt.readSubscription(5000);
    if (subscription == &authorised && (strcmp((char *)authorised.lastread, "OK") == 0))
      write_lcd("ID check OK");
    else
      write_lcd("ID check FAIL");
  } else {
    write_lcd("Sending", "Failed...");
  }
  delay(2000);
  write_lcd("Present your", "ID card...");
}
