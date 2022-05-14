#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#include <Wire.h>
#include "bsec.h"

#include <FastLED.h>

// MCH2022 badge (https://docs.badge.team/badges/mch2022/pinout/)
#define PIN_LED_DATA   5
#define PIN_LED_ENABLE 19
#define PIN_I2C_SDA    22
#define PIN_I2C_SCL    21
#define PIN_SPI_MOSI   23
#define PIN_SPI_SCK    18
#define PIN_LCD_CS     32
#define PIN_LCD_DC     33
#define PIN_LCD_MODE   26
#define PIN_LCD_RST    25

#define LCD_FREQ       40000000
#define NUM_LEDS       64

//Adafruit_ILI9341 tft = Adafruit_ILI9341(PIN_LCD_CS, PIN_LCD_DC, PIN_SPI_MOSI, PIN_SPI_SCK, PIN_LCD_RST, -1);
Adafruit_ILI9341 tft = Adafruit_ILI9341(PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

Bsec iaqSensor;
String output;
CRGB leds[NUM_LEDS];

void setup() {
  pinMode(PIN_LCD_MODE, OUTPUT);
  digitalWrite(PIN_LCD_MODE, LOW);
  pinMode(PIN_LCD_RST, OUTPUT);
  digitalWrite(PIN_LCD_RST, LOW);
 
  Serial.begin(115200);

  FastLED.addLeds<SK6812,PIN_LED_DATA,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(96);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  tft.begin(LCD_FREQ);
  tft.setRotation(1);

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);

  pinMode(PIN_LED_ENABLE, OUTPUT); // This has to be placed after SPI (LCD) has been initialized (Arduino wants to use this pin as SPI MISO...)
  digitalWrite(PIN_LED_ENABLE, HIGH);


  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

  

  tft.fillScreen(ILI9341_PURPLE);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(3);
  tft.println("MCH2022");
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.println("BME680");
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("BSEC "+ String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix));

  setLeds(CRGB::Purple);
  delay(2000);

}

void setLeds(CRGB color) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}


CRGB iaqToLedColor(float value) {
  if (value < 50.0) return CRGB::Green;
  if (value < 100.0) return CRGB::Yellow;
  if (value < 15.0) return CRGB::Orange;
  if (value < 200.0) return CRGB::Red;
  return CRGB::Purple;
}

uint16_t iaqToLcdColor(float value) {
  if (value < 50.0) return ILI9341_GREEN;
  if (value < 100.0) return ILI9341_YELLOW;
  if (value < 15.0) return ILI9341_ORANGE;
  if (value < 200.0) return ILI9341_RED;
  return ILI9341_PURPLE;
}

void loop(void) {
 if (iaqSensor.run()) { // If new data is available
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Raw temperature: " + String(iaqSensor.rawTemperature));
  tft.println("Temperature:     " + String(iaqSensor.temperature));
  tft.println("Pressure:        " + String(iaqSensor.pressure));
  tft.println("Raw humidity:    " + String(iaqSensor.rawHumidity));
  tft.println("Humidity:        " + String(iaqSensor.humidity));
  tft.println("Gas resistance:  " + String(iaqSensor.gasResistance));
  //tft.println("IAQ:             " + String(iaqSensor.iaq));
  tft.println("Static IAQ:      " + String(iaqSensor.staticIaq));
  tft.println("CO2 equivalent:  " + String(iaqSensor.co2Equivalent));
  tft.println("BreathVocEq.:    " + String(iaqSensor.breathVocEquivalent));
  tft.setTextSize(4);
  tft.setTextColor(iaqToLcdColor(iaqSensor.iaq));
  tft.println("IAQ: " + String(iaqSensor.iaq));
  tft.setTextSize(2);
  tft.println("(Accuracy: " + String(iaqSensor.iaqAccuracy) + ")");
  setLeds(iaqToLedColor(iaqSensor.iaq));

    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaq);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    output += ", " + String(iaqSensor.staticIaq);
    output += ", " + String(iaqSensor.co2Equivalent);
    output += ", " + String(iaqSensor.breathVocEquivalent);
    Serial.println(output);
  } else if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      tft.fillScreen(ILI9341_RED);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(3);
      tft.setCursor(0, 0);
      tft.println("BSEC error: " + String(iaqSensor.status));
      delay(5000);
      esp_restart();
    } else {
      tft.fillScreen(ILI9341_YELLOW);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(3);
      tft.setCursor(0, 0);
      tft.println("BSEC warning: " + String(iaqSensor.status));
    }
  }
}
