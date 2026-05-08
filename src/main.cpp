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
#include "time.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

const uint8_t PINO_LED_RGB = 48;
const uint8_t QUANTIDADE_LEDS = 1;
const uint8_t PINO_BUZZER = 36;
Led lampada(38);

bool alerta = 0;

Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800
);
LiquidCrystal_I2C lcd(0x27, 20, 4);

const char TOPICO_COMANDO[] = "wsd/comando";
const char TOPICO_CENTRAL[] = "wsd/central";
const char TOPICO_LOG[] = "wsd/log";
const char TOPICO_LATERAIS_A[] = "wsd/laterais/a";
const char TOPICO_LATERAIS_B[] = "wsd/laterais/b";

const uint8_t TEMPO_ALARME = 30;
int countdown = TEMPO_ALARME;
bool pressionado = 0;

// PROTOTIPOS

void tratarMensagemRecebida(const char *topico, const String &mensagem);
void tratarComando(const String &mensagem);
void tratarJson(const String &mensagem);
void configurarLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul);
void updateEstado();
void serializarAlerta(bool modo);
void sendLog();

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
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    updateEstado();

}

void loop()
{
    static unsigned long before = 0;
    if (millis() - before >= 1000)
    {
        before = millis();
        if (countdown > 0)
        {
            countdown--;

            if (countdown == 0)
            {
                alerta = true;
                updateEstado();
                serializarAlerta(alerta);
                sendLog();
            }
        }
    }

    garantirMQTTConectado();
    garantirWiFiConectado();
    loopMQTT();
    lampada.update();
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

    if (strcmp(topico, TOPICO_LATERAIS_A) == 0 || strcmp(topico, TOPICO_LATERAIS_B) == 0)
    {
        tratarJson(mensagem);
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
    if(strcmp(mensagem.c_str(), "RESET") == 0)
    {
        alerta = false;
        countdown = TEMPO_ALARME;
        serializarAlerta(alerta);
        updateEstado();
        debugInfo("Alerta resetado");
    }
    else if(strcmp(mensagem.c_str(), "ALERTA") == 0)
    {
        alerta = true;
        serializarAlerta(alerta);
        updateEstado();
        debugInfo("Modo alerta ativado");
    }
    else
    {
        debugErro("Comando desconhecido: " + mensagem);
    }
}

void tratarJson(const String &mensagem)
{
    JsonDocument doc;
  DeserializationError error = deserializeJson(doc, mensagem);

  if (!error)
  {
    if (doc["pressionado"].is<JsonVariant>())
    {
      pressionado = doc["pressionado"];
      if (pressionado)
      {
        countdown = TEMPO_ALARME;
        pressionado = 0;
      }
    }
  }
}

void updateEstado()
{
    if(!alerta)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WS DEVICE v1.0");
        lcd.setCursor(0, 2);
        lcd.print("NODE A: ONLINE");
        lcd.setCursor(0, 3);
        lcd.print("NODE B: ONLINE");

        alterarCorLedRGB(0, 255, 0);
        lampada.on();
        noTone(PINO_BUZZER);
    }
    else
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WS DEVICE v1.0");
        lcd.setCursor(0, 2);
        lcd.print("  >>>>>ALERTA<<<<<  ");

        alterarCorLedRGB(255, 0, 0);
        lampada.blink();
        tone(PINO_BUZZER, 1000);
    }
    
}

void serializarAlerta(bool modo)
{
    JsonDocument doc;
    doc["alerta"] = modo;

    String mensagem;
    serializeJson(doc, mensagem);

    publicarMensagem(TOPICO_CENTRAL, mensagem.c_str());
}

void sendLog()
{
    {
       struct tm timeinfo;
       if (getLocalTime(&timeinfo))
       {
           char buffer[20];
           strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
           String timestamp(buffer);
           publicarMensagem(TOPICO_LOG, "Alerta ativado em: ");
           publicarMensagem(TOPICO_LOG, timestamp.c_str());
       }
       else
       {
           debugErro("Falha ao obter a hora local");
       }  
    }
}