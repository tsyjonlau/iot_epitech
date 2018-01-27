#include <SPI.h>
#include <MFRC522.h>

#include <LiquidCrystal.h>

#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT.h>

#include <ESP8266WiFi.h>

#define SECURE_CO 1

#ifdef SECURE_CO
  #include <WiFiClientSecure.h>
  WiFiClientSecure wifi_client;
  // mottet.xyz SHA1 fingerprint
  const char* fingerprint = "CC:A1:0D:77:3B:BE:EC:02:9E:9F:6C:D2:EB:A1:F1:9A:C1:FA:46:1E";
#else
  #include <WiFiClient.h>
  WiFiClient wifi_client;
#endif

const char* ssid     = "IOT_PROJECT";
const char* password = "iot_epitech";

const char* mqtt_broker_host = "mottet.xyz";
const uint16_t mqtt_broker_port = 8883;

#define DEFAULT_MODULE_ID "AUTH_1337"
// Random ID button
#define BUTTON_PIN A0     // the number of the pushbutton pin

String module_id = DEFAULT_MODULE_ID;

// Variables to handle connection to MQTT server
Adafruit_MQTT_Client *mqtt;

#define PUBLISH_TOPIC "employee_badging_information"
#define VALIDATION_TOPIC "module_validation"
Adafruit_MQTT_Publish *pub;
Adafruit_MQTT_Publish *mod_validation;

Adafruit_MQTT_Subscribe *authorised;

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
  write_lcd("", "WiFi connection ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

#ifdef SECURE_CO
void verifyFingerprint() {
  
  while (! wifi_client.connect(mqtt_broker_host, mqtt_broker_port)) {
    //Serial.print(".");
    delay(2000);
  }

  if (wifi_client.verify(fingerprint, mqtt_broker_host)) {
    write_lcd("", "Connection secure.");
    delay(2000);
  } else {
    write_lcd("Co. insecure!", "Halting exec...");
    while(1) delay(2000);
  }

}
#endif

void mqtt_connect() {
  write_lcd("", "MQTT connection ");
  while(!mqtt->connected()) {
    if (!mqtt->connect()) {
      delay(5000);
    }
  }
}

void valid_module() {
 while(42) {
  if (!mqtt->connected()) {
    mqtt_connect();
  }
  write_lcd("Module", "validation...");
  if (mod_validation->publish(String("{\"moduleID\":\"" + module_id + "\"}").c_str())) {
    Adafruit_MQTT_Subscribe *subscription;
    subscription = mqtt->readSubscription(5000);
    if (subscription == authorised && (strcmp((char *)authorised->lastread, "OK") == 0)) { 
      write_lcd("", "valid!");
      delay(2000);
      return;
    } else {
      write_lcd("", "invalid");
      delay(2000);
      write_lcd("Waiting token", "validation");
      delay(5000);
    }
  } else {
    write_lcd("Sending", "Failed...");
    delay(2000);
  }
 }
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  delay(500);
  if (analogRead(BUTTON_PIN) > 100) {
    module_id = "AUTH_" + String(random(1000, 9999));
    write_lcd("Random mode:", module_id);
  }
  else {
    write_lcd("Normal mode:", module_id);
  }
  delay(4000);
  write_lcd(module_id);
  
  mqtt = new Adafruit_MQTT_Client(&wifi_client, mqtt_broker_host, mqtt_broker_port, module_id.c_str());
  
  pub = new Adafruit_MQTT_Publish(mqtt, PUBLISH_TOPIC);
  mod_validation = new Adafruit_MQTT_Publish(mqtt, VALIDATION_TOPIC);
  
  authorised = new Adafruit_MQTT_Subscribe(mqtt, module_id.c_str());

  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
    
  wifi_connect();
  
#ifdef SECURE_CO
  verifyFingerprint();
#endif

  mqtt->subscribe(authorised);
  
  mqtt_connect();

  valid_module();

  write_lcd("Present your", "ID card...");
}

String decToHex(char _byte)
{
  const char* tab = "0123456789ABCDEF";
  return String(tab[_byte >> 4]) + String(tab[_byte & 0x0F]);
}

void loop() {
  
  if (!mqtt->connected()) {
    mqtt_connect();
    write_lcd("Present your", "ID card...");
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
  if (pub->publish(String("{\"moduleID\":\"" + module_id + "\",\"cardID\":\"" + cur_str + "\"}").c_str())) {
    Adafruit_MQTT_Subscribe *subscription;
    subscription = mqtt->readSubscription(5000);
    if (subscription == authorised && (strcmp((char *)authorised->lastread, "OK") == 0)) { 
      write_lcd("ID check OK");
    } else {
      write_lcd("ID check FAIL");
    }
  } else {
    write_lcd("Sending", "Failed...");
  }
  delay(2000);
  write_lcd("Present your", "ID card...");
}
