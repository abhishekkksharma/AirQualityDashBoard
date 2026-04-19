#include <WiFi.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>

// ── Firebase credentials ───────────────────────────────────────
#define API_KEY      "AIzaSyAnOlSp29VeB0AIaeKSk9G-Y3R3f70XrGU"
#define DATABASE_URL "https://iot-project-2fce4-default-rtdb.firebaseio.com/"

// ── WiFi credentials ───────────────────────────────────────────
const char* ssid     = "Aman_2.4G_6vYj6J";
const char* password = "zxcvbnmm";

// ── Firebase objects ───────────────────────────────────────────
FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

// ── Sensor pins / config ───────────────────────────────────────
#define DHTPIN    4
#define DHTTYPE   DHT22
#define MQ135_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// MH-Z19E on UART2 (RX=16, TX=17)
HardwareSerial co2Serial(2);

// ── Read CO₂ from MH-Z19E ─────────────────────────────────────
int readCO2() {
  byte cmd[9]      = {0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79};
  byte response[9] = {0};

  while (co2Serial.available()) co2Serial.read(); // flush

  co2Serial.write(cmd, 9);
  delay(200);

  if (co2Serial.available() >= 9) {
    co2Serial.readBytes(response, 9);
    if (response[0] == 0xFF && response[1] == 0x86) {
      return response[2] * 256 + response[3];
    }
  }
  return -1;
}

// ── Setup ──────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(2000);

  dht.begin();
  co2Serial.begin(9600, SERIAL_8N1, 16, 17);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // Firebase setup
  config.api_key     = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase anonymous signup OK");
  } else {
    Serial.println("Firebase signup error: ");
    Serial.println(config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase ready");
}

// ── Loop ───────────────────────────────────────────────────────
void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int   mq   = analogRead(MQ135_PIN);
  int   co2  = readCO2();

  Serial.println("------ Sensor Data ------");
  Serial.print("Temperature : "); Serial.println(temp);
  Serial.print("Humidity    : "); Serial.println(hum);
  Serial.print("MQ135       : "); Serial.println(mq);
  Serial.print("CO2         : "); Serial.println(co2);

  if (!Firebase.ready()) {
    Serial.println("Firebase not ready, skipping upload.");
    delay(5000);
    return;
  }

  // ── 1. Update /sensor  (live dashboard cards) ──────────────
  if (!isnan(temp)) {
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temp))
      Serial.println("Temp uploaded");
    else
      Serial.println("Temp error: " + fbdo.errorReason());
  } else {
    Serial.println("DHT temp read failed");
  }

  if (!isnan(hum)) {
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", hum))
      Serial.println("Humidity uploaded");
    else
      Serial.println("Humidity error: " + fbdo.errorReason());
  } else {
    Serial.println("DHT humidity read failed");
  }

  if (Firebase.RTDB.setInt(&fbdo, "/sensor/mq135", mq))
    Serial.println("MQ135 uploaded");
  else
    Serial.println("MQ135 error: " + fbdo.errorReason());

  if (co2 != -1) {
    if (Firebase.RTDB.setInt(&fbdo, "/sensor/co2", co2))
      Serial.println("CO2 uploaded");
    else
      Serial.println("CO2 error: " + fbdo.errorReason());
  } else {
    Serial.println("CO2 sensor not ready");
  }

  // ── 2. Push to /readings  (history log) ────────────────────
  // Build a JSON object with all readings + Unix-style timestamp
  FirebaseJson json;
  // ts = seconds since boot (good enough for ordering/display)
  json.set("ts", (int)(millis() / 1000));

  if (!isnan(temp)) json.set("temperature", temp);
  if (!isnan(hum))  json.set("humidity",    hum);
  json.set("mq135", mq);
  if (co2 != -1)    json.set("co2",         co2);

  if (Firebase.RTDB.pushJSON(&fbdo, "/readings", &json))
    Serial.println("History entry pushed: " + fbdo.pushName());
  else
    Serial.println("History push error: " + fbdo.errorReason());

  Serial.println("-------------------------");
  delay(5000); // upload every 5 seconds
}
