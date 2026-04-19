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

// MH-Z19E UART
HardwareSerial co2Serial(2);

int readCO2() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79};
  byte response[9];

  while (co2Serial.available()) co2Serial.read();

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

void setup() {
  Serial.begin(115200);

  delay(2000);
  dht.begin();

  co2Serial.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Signup OK");
  } else {
    Serial.println(config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Ready");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int mq135 = analogRead(MQ135_PIN);
  int co2 = readCO2();

  Serial.println("------ Sensor Data ------");

  Serial.print("Temperature: ");
  Serial.println(temp);

  Serial.print("Humidity: ");
  Serial.println(hum);

  Serial.print("MQ135: ");
  Serial.println(mq135);

  Serial.print("CO2: ");
  Serial.println(co2);

  if (!isnan(temp)) {
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temp))
      Serial.println("Temp Uploaded");
    else
      Serial.println(fbdo.errorReason());
  } else {
    Serial.println("DHT Temp Failed");
  }

  if (!isnan(hum)) {
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", hum))
      Serial.println("Humidity Uploaded");
    else
      Serial.println(fbdo.errorReason());
  } else {
    Serial.println("DHT Humidity Failed");
  }

  if (Firebase.RTDB.setInt(&fbdo, "/sensor/mq135", mq135))
    Serial.println("MQ135 Uploaded");
  else
    Serial.println(fbdo.errorReason());

  if (co2 != -1) {
    if (Firebase.RTDB.setInt(&fbdo, "/sensor/co2", co2))
      Serial.println("CO2 Uploaded");
    else
      Serial.println(fbdo.errorReason());
  } else {
    Serial.println("CO2 Sensor Not Ready");
  }

  Serial.println("-------------------------");
  delay(5000);
}