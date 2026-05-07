#include <Arduino.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"
#include "secrets.h"
#include <Bounce2.h>

const uint8_t QUANTIDADE_LEDS = 3;
const uint8_t PINO_LED_RGB = 36;

Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800); 


Bounce botaoBoot = Bounce();

void configuraLedRGB();
void alterarCorLedRGB(int verde, int laranja, int vermelho);

void setup() 
{
botaoBoot.attach(0, INPUT_PULLUP);
}

void loop()
{
botaoBoot.update();


}

void configuraLedRGB()
{
 ledRGB.begin();
 ledRGB.setBrightness(100);
 ledRGB.clear();
 ledRGB.show();
}

void alterarCorLedRGB()
{}