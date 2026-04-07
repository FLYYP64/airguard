#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PMS.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- KONFIGURACJA SIECI (PAMIĘTAJ O DOBREJ SIECI!) ---
const char* ssid = "Tp-link AC1200";
const char* password = "#internetFilip";
const char* mqtt_server = "192.168.10.100";

WiFiClient espClient;
PubSubClient client(espClient);

// Definicje pinów ESP-32D
#define BUTTON_PIN 4    
#define PMS_SET_PIN 5   
#define RGB_RED 13      
#define RGB_GREEN 12    
#define RGB_BLUE 14     
#define PMS_RX 16       
#define PMS_TX 17       

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;

PMS pms(Serial2);
PMS::DATA data;

// Funkcja łącząca z WiFi z komunikatami na ekranie
void setup_wifi() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Siec: " + String(ssid));
  display.println("Status: Laczenie...");
  display.display();
  
  WiFi.begin(ssid, password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter > 20) { // Jeśli nie połączy w 10s, idź dalej
      Serial.println("Blad WiFi, ale sprawdzam dalej...");
      break; 
    }
  }
  Serial.println("WiFi OK");
}

// Funkcja łącząca z MQTT
void reconnect() {
  if (WiFi.status() != WL_CONNECTED) return; // Nie próbuj bez WiFi
  
  while (!client.connected()) {
    Serial.print("Proba MQTT...");
    if (client.connect("AirGuard_ESP32")) {
      Serial.println("polaczono");
    } else {
      Serial.print("blad, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setRGB(int r, int g, int b) {
  analogWrite(RGB_RED, r);
  analogWrite(RGB_GREEN, g);
  analogWrite(RGB_BLUE, b);
}

void setup() {
  Serial.begin(115200);
  
  // 1. PINY
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PMS_SET_PIN, OUTPUT);
  digitalWrite(PMS_SET_PIN, LOW); // Sensor śpi na start
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);

  // 2. EKRAN STARTUJE PIERWSZY (0x3C to standardowy adres)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Blad OLED! Sprawdz kable SDA/SCL");
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE); // Wymuszenie koloru tekstu
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("System AirGuard");
  display.println("Wersja 2.0 (MQTT)");
  display.println("Inicjalizacja...");
  display.display();

  // 3. CZUJNIKI I SERIAL2
  if(!bme.begin(0x76)) Serial.println("Blad BME280");
  Serial2.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  
  // 4. WIFI I SERWER
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // 5. GOTOWOŚĆ
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("SYSTEM GOTOWY");
  display.println("Online: " + WiFi.localIP().toString());
  display.println("");
  display.println("Nacisnij guzik...");
  display.display();
  
  setRGB(0, 0, 0); 
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    performMeasurement();
  }

  // Utrzymuj połączenie z serwerem w tle
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect();
  }
  client.loop();
}

void performMeasurement() {
  // Budzenie
  digitalWrite(PMS_SET_PIN, HIGH); 
  
  for(int i = 10; i > 0; i--) {
    display.clearDisplay();
    display.setCursor(0,10);
    display.setTextSize(1);
    display.println("Pomiar za:");
    display.setTextSize(3);
    display.setCursor(40, 30);
    display.print(i);
    display.display();
    setRGB(50, 50, 0); // Pomarańczowy/Żółty podczas nagrzewania
    delay(1000);
  }

  if (pms.readUntil(data)) {
    int pm25 = data.PM_AE_UG_2_5;
    int pm10 = data.PM_AE_UG_10_0;
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;

    // --- WYSYŁKA DO TWOJEGO PROXMOXA ---
    if (client.connected()) {
      client.publish("airguard/pm25", String(pm25).c_str());
      client.publish("airguard/pm10", String(pm10).c_str());
      client.publish("airguard/temp", String(temp, 1).c_str());
      client.publish("airguard/hum", String(hum, 1).c_str());
      client.publish("airguard/pres", String(pres, 1).c_str());
    }

    // WYŚWIETLANIE WYNIKÓW
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.print("PM2.5:"); display.println(pm25);
    
    display.setTextSize(1);
    display.println("---------------------");
    display.print("PM10: "); display.print(pm10); display.println(" ug/m3");
    display.print("Temp: "); display.print(temp, 1); display.println(" C");
    display.print("Wilg: "); display.print(hum, 0); display.println(" %");
    display.print("Cisn: "); display.print(pres, 0); display.println(" hPa");
    display.display();
    
    setRGB(0, 50, 0); // Zielony - sukces
    delay(10000); // Wynik widoczny przez 10s
  }

  // Powrót do uśpienia
  digitalWrite(PMS_SET_PIN, LOW); 
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Sensor uspiony.");
  display.println("Dane w chmurze.");
  display.println("");
  display.println("Wcisnij by ponowic");
  display.display();
  setRGB(0, 0, 0);
}