#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Pms5003.h>

// Konfiguracja OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Konfiguracja PMS5003 (Używamy Serial2: RX=16, TX=17)
Pms5003 pms(Serial2);

void setup() {
  Serial.begin(115200);
  
  // Inicjalizacja UART2 dla sensora pyłu
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Inicjalizacja OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Nie znaleziono ekranu OLED"));
    for(;;);
  }

  pms.begin();
  pms.waitForData(); // Czekaj na pierwsze dane z sensora

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  Pms5003::data data;
  
  if (pms.read(data)) {
    int pm25 = data.pm25_atm;
    int pm10 = data.pm10_atm;
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();



    //Wyświetlanie na OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("JAKOSC POWIETRZA:");
    
    display.setTextSize(2);
    display.print("PM2.5: "); display.println(pm25);
    
    display.setTextSize(1);
    display.print("PM10: "); display.print(pm10); display.println(" ug/m3");
    
    display.display();
    
    //Debugowanie na Serial Monitor
    Serial.print("PM2.5: "); Serial.println(pm25);
  }

  delay(2000); // Odświeżaj co 2 sekundy
}