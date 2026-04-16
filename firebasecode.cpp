#include <WiFi.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>

// Firebase
#define API_KEY "AIzaSyAnOlSp29VeB0AIaeKSk9G-Y3R3f70XrGU"
#define DATABASE_URL "https://iot-project-2fce4-default-rtdb.firebaseio.com/"

// WiFi
const char* ssid = "Aman_2.4G_6vYj6J";
const char* password = "zxcvbnmm";

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Sensors
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ135_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// CO2 UART
HardwareSerial co2Serial(2);

void setup() {
  Serial.begin(115200);
  dht.begin();

  co2Serial.begin(9600, SERIAL_8N1, 16, 17);

  // WiFi connect with timeout
  WiFi.begin(ssid, password);
  Serial.print("Connecting...");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed!");
  }

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Proper authentication (IMPORTANT)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Signup OK");
  } else {
    Serial.printf("Signup error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = NULL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Ready");
}

int readCO2() {
  byte cmd[9] = { 0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79 };
  byte response[9];

  co2Serial.write(cmd, 9);
  delay(10);

  if (co2Serial.available() >= 9) {
    co2Serial.readBytes(response, 9);
    return (256 * response[2]) + response[3];
  }
  return -1;
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int mq135 = analogRead(MQ135_PIN);
  int co2 = readCO2();

  Serial.println("------ Sending to Firebase ------");

  // Temperature
  if (Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temp)) {
    Serial.println("Temp OK");
  } else {
    Serial.println(fbdo.errorReason());
  }

  // Humidity
  if (Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", hum)) {
    Serial.println("Humidity OK");
  } else {
    Serial.println(fbdo.errorReason());
  }

  // MQ135
  if (Firebase.RTDB.setInt(&fbdo, "/sensor/mq135", mq135)) {
    Serial.println("MQ135 OK");
  } else {
    Serial.println(fbdo.errorReason());
  }

  // CO2
  if (co2 != -1) {
    if (Firebase.RTDB.setInt(&fbdo, "/sensor/co2", co2)) {
      Serial.println("CO2 OK");
    } else {
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("CO2 not available");
  }

  delay(5000);
}