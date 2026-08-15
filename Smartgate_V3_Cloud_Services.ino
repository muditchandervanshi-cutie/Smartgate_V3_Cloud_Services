#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);




// ---------------- RFID ----------------

#define SS_PIN 4
#define RST_PIN 2
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- Pins ----------------
#define TRIG_PIN 5
#define ECHO_PIN 17
#define IR_PIN 27

#define RED_LED 32
#define GREEN_LED 25
#define BLUE_LED 26

#define SERVO_PIN 13

Servo gateServo;

// ---------------- WiFi ----------------

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ---------------- Firebase ----------------

#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
bool systemEnabled = true;
bool lastSystemEnabled = true;

// ---------------- HEARTBEAT ----------------
unsigned long heartbeatTimer = 0;

// ---------------- USER DATABASE ----------------

struct AccessCard {
  byte uid[4];
  const char* name;
  const char* role;
};

// ---------------- TIME ----------------
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;


// ---------------- ACCESS EVENT ----------------

struct AccessEvent
{
    const char* name;
    const char* role;
    bool granted;
    String date;
    String time;
    const char* gateName;


};

AccessCard cards[] = {

  {{0x00, 0x00, 0x00, 0x01}, "Owner", "Owner"},
  {{0x00, 0x00, 0x00, 0x02}, "Owner 2", "Owner"},

  {{0x00, 0x00, 0x00, 0x03}, "Admin 1", "Admin"},
  {{0x00, 0x00, 0x00, 0x04}, "Admin 2", "Admin"},

  {{0x00, 0x00, 0x00, 0x05}, "Member 1", "Member"},
  {{0x00, 0x00, 0x00, 0x06}, "Member 2", "Member"}

};

const int NUM_CARDS = sizeof(cards) / sizeof(cards[0]);
AccessEvent lastEvent;

// ---------------- STATE ----------------
enum State {
  IDLE,
  VISITOR_DETECTED,
  SCAN_CARD,
  GRANTED,
  DENIED
};

State state = IDLE;
State lastState = IDLE;

// ---------------- LCD ----------------
void showLCD(String l1, String l2 = "") {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  lcd.setCursor(0, 0);
  lcd.print(l1);

  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// ---------------- LED ----------------
void setLED(int r, int g, int b) {
  digitalWrite(RED_LED, r);
  digitalWrite(GREEN_LED, g);
  digitalWrite(BLUE_LED, b);
}

// ---------------- DISTANCE (0–50 cm filter) ----------------
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  float distance = duration * 0.034 / 2;

  if (distance > 50) return -1;

  return distance;
}

// ---------------- UID MATCH ----------------
bool matchUID(byte *a, byte *b) {
  for (byte i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

AccessCard* findCard(byte *uid)
{
  for (int i = 0; i < NUM_CARDS; i++)
  {
    if (matchUID(uid, cards[i].uid))
    {
      return &cards[i];
    }
  }

  return nullptr;
} 

// ---------------- GATE ANIMATION ----------------
void openGateAnimated()
{
    setLED(0,0,1);
    showLCD("Gate Opening","Please wait");

    // Open
    for(int pos = 135; pos >= 0; pos -= 2)
    {
        gateServo.write(pos);
        delay(20);
    }

    showLCD("Access Granted","Welcome!");
    setLED(0,1,0);
    delay(2000);

    showLCD("Closing Gate","");

    // Close
    for(int pos = 0; pos <= 135; pos += 2)
    {
        gateServo.write(pos);
        delay(20);
    }
}

   void printAccessEvent()
    {
    
  Serial.println("------ Access Event ------");
  Serial.print("Name: ");
  Serial.println(lastEvent.name);

  Serial.print("Role: ");
  Serial.println(lastEvent.role);

  Serial.print("Granted: ");
  Serial.println(lastEvent.granted ? "YES" : "NO");

  Serial.println("--------------------------");
    }

// ---------------- CONNECTION ----------------

void connectWiFi()
{
    showLCD("Connecting", "WiFi...");
    setLED(0, 0, 1);   // Blue while connecting

    Serial.print("Connecting to WiFi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    showLCD("WiFi Connected");
    setLED(0, 1, 0);   // Green = ready

    delay(1500);
}

// ---------------- TIME FUNCTION ----------------

void updateCurrentTime()
{
    struct tm timeinfo;

    if (getLocalTime(&timeinfo))
    {
        char dateBuffer[20];
        char timeBuffer[20];

        strftime(dateBuffer, sizeof(dateBuffer), "%d-%m-%Y", &timeinfo);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);

        lastEvent.date = String(dateBuffer);
        lastEvent.time = String(timeBuffer);
    }
}

void initTime()
{ 
  Serial.println("Synchronizing time ...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
     
     struct tm timeinfo;
      
      while (!getLocalTime(&timeinfo))
      {Serial.print(".");
      delay(500);
      }
      Serial.println();
      Serial.println("Time synchronized!");
}


// ---------------- FIREBASE ----------------

void initFirebase()
{
    Serial.println("\nInitializing Firebase...");

    // API Key
    config.api_key = API_KEY;

    // Database URL
    config.database_url = DATABASE_URL;

    // Automatically reconnect WiFi if disconnected
    Firebase.reconnectNetwork(true);

    // SSL buffer
    fbdo.setBSSLBufferSize(4096, 1024);

    Serial.print("Signing in anonymously... ");

    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("Success!");
        signupOK = true;
    }
    else
    {
        Serial.println("Failed!");
        Serial.println(config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("Firebase initialized.");
}


// ---------------- EVENTSLOG ----------------

void uploadAccessEvent()
{
    if (!Firebase.ready() || !signupOK)
        return;

    FirebaseJson json;

    json.set("name", lastEvent.name);
    json.set("role", lastEvent.role);
    json.set("granted", lastEvent.granted);
    json.set("date", lastEvent.date);
    json.set("time", lastEvent.time);
    json.set("gate", lastEvent.gateName);

    if (Firebase.pushJSON(fbdo, "/AccessLogs", json))
    {
        Serial.println("Access event uploaded!");
    }
    else
    {
        Serial.print("Upload failed: ");
        Serial.println(fbdo.errorReason());
    }
}

// ---------------- HEARTBEAT ----------------

void uploadHeartbeat()
{
    if (!Firebase.ready() || !signupOK)
        return;

    FirebaseJson json;

  
    json.set("wifiConnected", WiFi.status() == WL_CONNECTED);
    json.set("wifiRSSI", WiFi.RSSI());
    json.set("ipAddress", WiFi.localIP().toString());

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
        json.set("lastHeartbeat", buffer);
    }

    if (Firebase.setJSON(fbdo, "/SystemStatus", json))
    {
        Serial.println("Heartbeat updated.");
    }
    else
    {
        Serial.println(fbdo.errorReason());
    }
}

// ---------------- REMOTE CONTROL ----------------

void checkRemoteControl()
{
    if (!Firebase.ready() || !signupOK)
        return;

    if (Firebase.getBool(fbdo, "/RemoteControl/openGate"))
    {
        if (fbdo.boolData())
        {
          if (!systemEnabled)
{
    Serial.println("Remote command rejected. System is in Lockdown.");
    Firebase.setBool(fbdo, "/RemoteControl/openGate", false);
    return;
}
            Serial.println("Remote Gate Open Requested!");

            state = GRANTED;

            Firebase.setBool(fbdo, "/RemoteControl/openGate", false);

            Serial.println("Remote Gate Opened!");
        }
    }
    else
    {
        Serial.print("Firebase Read Error: ");
        Serial.println(fbdo.errorReason());
    }
}

// ---------------- Emergency Lockdown---------------

void checkSystemStatus()
{
    if (!Firebase.ready() || !signupOK)
        return;

    if (Firebase.getBool(fbdo, "/RemoteControl/systemEnabled"))
    {
        systemEnabled = fbdo.boolData();

        if (systemEnabled != lastSystemEnabled)
{
    lastSystemEnabled = systemEnabled;

    if (systemEnabled)
    {
        showLCD("System Ready");
        setLED(0, 1, 0);
        state = IDLE;
    }
    else
    {
        showLCD("SYSTEM LOCKED", "Contact Admin");
        setLED(1, 0, 0);
    }
}

//        Serial.print("System Enabled = ");
//        Serial.println(systemEnabled);
    }
    else
    {
        Serial.println(fbdo.errorReason());
    }
}

// ---------------- SETUP ----------------
void setup() {



  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  connectWiFi();

  initTime();

  initFirebase();



  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Initialized");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

 

  gateServo.attach(SERVO_PIN);
  gateServo.write(135);

  showLCD("Waking up", "Initializing...");
  setLED(0, 0, 1);
  delay(2000);

  showLCD("System ready");
  setLED(0, 1, 0);

}

// ---------------- LOOP ----------------
void loop() {

  if (millis() - heartbeatTimer > 10000)
{
    heartbeatTimer = millis();
    uploadHeartbeat();
}

  checkSystemStatus();
  checkRemoteControl();

  if (!systemEnabled)
{
    return;
}

  float dist = getDistance();
  bool irDetected = digitalRead(IR_PIN);

  // -------- STATE UPDATE DISPLAY --------
  if (state != lastState) {
    lastState = state;

    if (state == IDLE) {
      showLCD("System Ready");
      setLED(0, 1, 0);
    }

    else if (state == VISITOR_DETECTED) {
      showLCD("Visitor Detected");
      setLED(0, 1, 0);
    }

    else if (state == SCAN_CARD) {
      showLCD("Scan Your Card");
      setLED(1, 0, 0);
    }

    else if (state == DENIED) {
      showLCD("Access Denied");
      setLED(0, 0, 1);
    }
  }

  // -------- LOGIC --------
  if (state == IDLE) {
    if (dist > 0 && dist <= 30) {
      state = VISITOR_DETECTED;
    }
  }

  else if (state == VISITOR_DETECTED) {
    if (irDetected == LOW) {
      state = SCAN_CARD;
    }
  }

  else if (state == SCAN_CARD) {

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
      return;
    }

    AccessCard* user = findCard(rfid.uid.uidByte);

  if (user != nullptr)
{
    lastEvent.name = user->name;
    lastEvent.role = user->role;
    lastEvent.granted = true;
    lastEvent.gateName = "Main Gate";

    if (strcmp(user->role, "Owner") == 0)
    {
        showLCD("Hello Cutie!", "Welcome Home");
    }
    else if (strcmp(user->role, "Admin") == 0)
    {
        showLCD("Admin Access", "Granted");
    }
    else
    {
        showLCD("Member Access", "Granted");
    }

    state = GRANTED;
}
else
{
    lastEvent.name = "Unknown";
    lastEvent.role = "Unknown";
    lastEvent.granted = false;
    lastEvent.gateName = "Main Gate";

    state = DENIED;
}

updateCurrentTime();
printAccessEvent();
uploadAccessEvent();

rfid.PICC_HaltA();
rfid.PCD_StopCrypto1();
 
}
else if (state == GRANTED) {
    openGateAnimated();
    state = IDLE;
}

else if (state == DENIED) {

    delay(1500);
    state = IDLE;
}
}

  