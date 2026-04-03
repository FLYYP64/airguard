#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PMS.h>

// Definicje pinów ESP-32D
#define BUTTON_PIN 4    
#define PMS_SET_PIN 5 
#define PMS_RX 16       
#define PMS_TX 17       

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;

PMS pms(Serial2);
PMS::DATA data;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PMS_SET_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("Blad OLED");
  if(!bme.begin(0x76)) Serial.println("Blad BME280");

  Serial2.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  
  // Start w trybie uspienia
  digitalWrite(PMS_SET_PIN, LOW);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("System AirGuard");
  display.println("Gotowy");
  display.println("Nacisnij guzik");
  display.println("aby wykonac pomiar");
  display.display();
  setRGB(0, 0, 0); 
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    performMeasurement();
  }
}

void performMeasurement() {
  // Budzenie
  digitalWrite(PMS_SET_PIN, HIGH); 
  
  // Odliczanie 10s
  for(int i = 10; i > 0; i--) {
    display.clearDisplay();
    display.setCursor(0,10);
    display.setTextSize(1);
    display.print("Budzenie sensora...");
    display.setCursor(0,30);
    display.setTextSize(2);
    display.print("Odczyt za:    "); display.print(i);
    display.display();
    setRGB(50, 50, 0); 
    delay(1000);
  }

  // Pomiar
  if (pms.readUntil(data)) {
    int pm25 = data.PM_AE_UG_2_5;
    int pm10 = data.PM_AE_UG_10_0;
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;

    // ŁADNY UKŁAD NA WYŚWIETLACZU
    display.clearDisplay();
    display.setCursor(0, 0);
    
    // PM2.5 i PM10 - Duże i wyraźne
    display.setTextSize(2);
    display.print("PM2.5:"); display.println(pm25);
    
    // Separator
    display.setTextSize(1);
    display.println("---------------------");
    
    // Dane meteo - mniejsze
    display.print("PM10:"); display.print(pm10); display.println(" ug/m3");
    display.print("Temperatura:  "); display.print(temp, 1); display.println(" C");
    display.print("Wilgotnosc:  "); display.print(hum, 0); display.println(" %");
    display.print("Cisnienie:  "); display.print(pres, 0); display.println(" hPa");
    display.display();
    
    delay(10000); //Wynik przez 10s
  }

  //Uśpienie
  digitalWrite(PMS_SET_PIN, LOW); 
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Sensor uspiony.");
  display.println("Wcisnik guzik");
  display.println("aby wykonac pomiar");
  display.display();
  setRGB(0, 0, 0);
}