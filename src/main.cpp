#include <Arduino.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <Bounce2.h>
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

const uint8_t PINO_LED_RGB = 48;
const uint8_t QUANTIDADE_LEDS = 1;

const int TEMPO_ALARME = 60;
int countdown = TEMPO_ALARME;
bool houveTroca = 0;

const char TOPICO_LATERAIS_A[] = "wsd/laterais/a";
const char TOPICO_CENTRAL[] = "wsd/central";

Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800 // Duas constantes que definem a saidas do rgb e a frquencia de propagacao da informacao entre os LED`s, nesse caso é igual a 82
);

Bounce botaoBoot = Bounce();

bool estadoAlerta = 0;
bool locked = 0;

void configuraLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul);
void tratarJsonLateral(const String &mensagem);

void tratarMensagemRecebida(const char *topico, const String &mensagem);

void tratamentoEspLateral();

void updateLed();

void setup()
{
    configurarDebug();

    configuraLedRGB();

    conectarWiFi();
    configurarMQTT();
    registrarCallbackMensagem(tratarMensagemRecebida);
    conectarMQTT();
    botaoBoot.attach(0, INPUT_PULLUP);
    botaoBoot.interval(5);
botaoBoot.attach(0, INPUT_PULLUP);
}

void loop()
{
    houveTroca = false;
    garantirWiFiConectado();
    garantirMQTTConectado();
    botaoBoot.update();
    loopMQTT();

    tratamentoEspLateral();
}

void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
    debugInfo("======================================");
    debugInfo("Mensagem recebida na aplicação");
    debugInfo("======================================");

    if (topico == nullptr)
    {
        debugErro("Tópico MQTT inválido");
        return;
    }

    debugInfo("Tópico: " + String(topico));
    debugInfo("Mensagem " + mensagem);

    if (strcmp(topico, TOPICO_LATERAIS_A) == 0)
    {
        tratarJsonLateral(mensagem);
        return;
    }

    debugErro("Tópico não tratado: " + String(topico));
}

void configuraLedRGB()
{
    ledRGB.begin();
    ledRGB.setBrightness(80); // Colocamos a qtd de brilho para o led de 0 a 255
    ledRGB.clear();
    ledRGB.show(); // atualiza estado led

    debugInfo("LED RGB configurado no GPIO " + String(PINO_LED_RGB));
}

void alterarCorLedRGB(int vermelho, int verde, int azul)
{
    vermelho = constrain(vermelho, 0, 255);
    verde = constrain(verde, 0, 255);
    azul = constrain(azul, 0, 255);
botaoBoot.update();


    ledRGB.setPixelColor(0, ledRGB.Color(vermelho, verde, azul));
    ledRGB.show();

    debugInfo("Cor aplicada o LED RGB");
    debugInfo("R: " + String(vermelho));
    debugInfo("G: " + String(verde));
    debugInfo("B: " + String(azul));
}

void tratarJsonLateral(const String &mensagem)
{
    JsonDocument doc;
    DeserializationError erro = deserializeJson(doc, mensagem);
    
    if(erro)
    {
        debugErro("Json inválido, corrija a formatação.");
        return;
    }
    
    if(doc["alerta"].is<JsonObject>())
    {
        if(doc["alerta"].is<bool>())
        {
            estadoAlerta = doc["alerta"].as<bool>();
        }
        
    }

}

void updateLed()
{
    switch (estadoAlerta)
    {
    case 0:
        alterarCorLedRGB(0, 255, 0);
        break;
    case 1:
        alterarCorLedRGB(255, 150, 0);
        break;
    case 2:
        alterarCorLedRGB(255, 25, 0);
        break;
    case 3:
        alterarCorLedRGB(255, 0, 0);
        break;
    }
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