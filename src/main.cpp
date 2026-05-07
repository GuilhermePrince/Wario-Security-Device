#include <Arduino.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"
#include "secrets.h"
#include <LED.h>

const uint8_t PINO_LED_RGB = 48;
const uint8_t QUANTIDADE_LEDS = 1;
const uint8_t PINO_BUZZER = 36;

Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800 // Duas constantes que definem a saidas do rgb e a frquencia de propagacao da informacao entre os LED`s, nesse caso é igual a 82
);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Led lampada(38);

const char TOPICO_COMANDO[] = "wsd/comando";
const char TOPICO_CENTRAL[] = "wsd/central";
const char TOPICO_LATERAIS[] = "wsd/laterais";

int countdown = 20;

// PROTOTIPOS

void tratarMensagemRecebida(const char *topico, const String &mensagem);
void tratarComando(const String &mensagem);
void configurarLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul);
void modoAlerta();

void setup()
{
    pinMode(PINO_BUZZER, OUTPUT);
    configurarDebug();
    configurarMQTT();
    configurarLedRGB();
    conectarWiFi();
    registrarCallbackMensagem(tratarMensagemRecebida);
    conectarMQTT();

    lcd.init();
    lcd.backlight();
    lcd.print("WS DEVICE");

}

void loop()
{
    static unsigned long before = 0;
    static bool buzzerSound = 0;
    if (millis() - before >= 1000)
    {
        before = millis();
        if (countdown > 0)
        {
            countdown--;
            lcd.setCursor(0, 2);
            lcd.printf("Tempo restante: %d  ", countdown);

            if (countdown == 0)
            {
                lampada.blink(2);
                publicarMensagem(TOPICO_LATERAIS, "alerta");
                alterarCorLedRGB(255, 0, 0);
            }
        }
        else
        {
        buzzerSound = !buzzerSound;
        buzzerSound ? tone(PINO_BUZZER, 800) : tone(PINO_BUZZER, 400);
        }    
    }

    garantirMQTTConectado();
    garantirWiFiConectado();
    loopMQTT();
}

void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
    debugInfo("==============================");
    debugInfo("Mensagem recebida na aplicação");
    debugInfo("==============================");

    if (topico == nullptr)
    {
        debugErro("Tópico MQTT inválido");
        return;
    }

    debugInfo("Tópico: " + String(topico));
    debugInfo("Mensagem: " + mensagem);

    if (strcmp(topico, TOPICO_COMANDO) == 0)
    {
        tratarComando(mensagem);
        return;
    }

    if (strcmp(mensagem.c_str(), "pressionado") == 0)
    {
        countdown = 20;
        alterarCorLedRGB(0, 255, 0);
        noTone(PINO_BUZZER);
        return;
    }

    debugErro("Tópico não tratado: " + String(topico));
}

void configurarLedRGB()
{
    ledRGB.begin();
    ledRGB.setBrightness(80);
    ledRGB.clear();
    ledRGB.show();
}

void alterarCorLedRGB(int vermelho, int verde, int azul)
{
    vermelho = constrain(vermelho, 0, 255);
    verde = constrain(verde, 0, 255);
    azul = constrain(azul, 0, 255);

    ledRGB.setPixelColor(0, ledRGB.Color(vermelho, verde, azul));
    ledRGB.show();

    debugInfo("Cor aplicada o LED RGB");
    debugInfo("R: " + String(vermelho));
    debugInfo("G: " + String(verde));
    debugInfo("B: " + String(azul));
}

void tratarComando(const String &mensagem)
{
    const char *mensagemChar = mensagem.c_str();
    lcd.clear();
    if (strcmp(mensagemChar, "help") == 0)
    {
        debugInfo("Comandos:\n> reset\n> start\n> ...");
    }
    else if (strcmp(mensagemChar, "start") == 0)
    {
        publicarMensagem(TOPICO_LATERAIS, "start");
    }
    else if (strcmp(mensagemChar, "reset") == 0)
    {
        publicarMensagem(TOPICO_LATERAIS, "reset");
    }
    else
        debugErro("Comando não encontrado! Use help para listar todos comandos.");
}