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


const uint8_t PINO_LED_RGB = 48;
const uint8_t QUANTIDADE_LEDS = 1;



const char TOPICO_COMANDO[] = "";  //Não é necessário ponteiro pois nesse caso o número de caracteres é fixo

Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800       //Duas constantes que definem a saidas do rgb e a frquencia de propagacao da informacao entre os LED`s, nesse caso é igual a 82
);

Bounce botaoBoot = Bounce();

uint8_t estadoAlerta = 0;


void configuraLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul);
void tratarJsonLateral(const String& mensagem);

void tratarMensagemRecebida(const char* topico, const String& mensagem);

void tratamentoEspLateral();


void setup() 
{
configurarDebug();

configuraLedRGB();

conectarWiFi();
configurarMQTT();
registrarCallbackMensagem(tratarMensagemRecebida);
conectarMQTT();
}

void loop() 
{
    garantirWiFiConectado();
    garantirMQTTConectado();
    botaoBoot.update();
    loopMQTT();

    
   tratamentoEspLateral();
}

void tratarMensagemRecebida(const char* topico, const String& mensagem)
{
    debugInfo("======================================");
    debugInfo("Mensagem recebida na aplicação");
    
    if (topico == nullptr)
    {
        debugErro("Tópico MQTT inválido");
        return;
    }

    debugInfo("Tópico: " + String(topico));
    debugInfo("Mensagem " + mensagem);

    if(strcmp(topico, TOPICO_COMANDO) == 0)
    {
        tratarJsonLateral(mensagem);
        return;
    }
    
    debugErro("Tópico não tratado: " + String(topico));

}


void configuraLedRGB()
{
    ledRGB.begin();
    ledRGB.setBrightness(80);   //Colocamos a qtd de brilho para o led de 0 a 255
    ledRGB.clear();
    ledRGB.show();      //atualiza estado led

    debugInfo("LED RGB configurado no GPIO " + String(PINO_LED_RGB));
}

void alterarCorLedRGB(int vermelho, int verde, int azul)
{
    vermelho = constrain(vermelho, 0 ,255);
    verde  = constrain(verde, 0 ,255);
    azul = constrain(azul, 0 ,255);
    
    ledRGB.setPixelColor(0, ledRGB.Color(vermelho, verde, azul));
    ledRGB.show();

    debugInfo("Cor aplicada o LED RGB");
    debugInfo("R: " + String(vermelho));
    debugInfo("G: " + String(verde));
    debugInfo("B: " + String(azul));


}


void tratarJsonLateral(const String& mensagem)
{
    static bool estadoAlerta = false;
    const char *message = mensagem.c_str();
    if(strcmp(message, "alerta") == 0) 
    {
        alterarCorLedRGB(255, 0, 0);
        debugInfo("Alarme disparado, há um invasor");
        return;
    }
    else debugErro("Comando incorreto.");

   















    // JsonDocument doc;

    // DeserializationError erro = deserializeJson(doc, mensagem);

    // if(erro)
    // {
    //     debugErro("Erro ao interpretar o Json");
    //     debugErro(erro.c_str());
    //     return;
    // }

    
    // if(doc["led"].is<JsonObject>())
    // {
    //     if(!doc["lampada"].is<bool>())
    //     {
    //         debugErro("Json invalido. Use valores booleanos");
    //         return;
    //     }
    //     else
    //     {
    //         bool estadoLampada = doc["lampada"].as<bool>();
    //     }

    //     if(!doc["led"]["r"].is<int>() || 
    //     !doc["led"]["g"].is<int>() || 
    //     !doc["led"]["b"].is<int>())
    //     {
    //         debugErro("JSON Invalido. Use led.r, led.g e led.b");
    //         return;
    //     }
    //     else
    //     {
    //         int vermelho = doc["led"]["r"].as<int>();
    //         int verde = doc["led"]["g"].as<int>();
    //         int azul = doc["led"]["b"].as<int>();

    //         alterarCorLedRGB(vermelho, verde, azul);
    //     }
    // }
}


void tratamentoEspLateral()
{
    bool estadoBotao = 1;
    static bool estadoBotaoAnterior = estadoBotao;
    static unsigned int tempoAnterior = 0;
    const char *mensagem = "";
    
    static int countdown = 20;

    if (millis() - tempoAnterior >= 1000)
    {
        countdown--;
    }

    if(botaoBoot.changed())
    {
        estadoBotao = botaoBoot.read();
    }

    if(estadoBotaoAnterior && !estadoBotao)
    {
        estadoAlerta = 0;
        tempoAnterior == millis();
        countdown = 20;
        mensagem = "resetCountdown";
        publicarMensagemNoTopico(0, mensagem);
    }
    
    if(countdown <= 20 && countdown > 5) estadoAlerta = 0;
    else if(countdown < 0) estadoAlerta = 1;
    else estadoAlerta = 2;

}
