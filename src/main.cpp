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

// Objeto LED RGB
Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800 // Duas constantes que definem a saidas do rgb e a frquencia de propagacao da informacao entre os LED`s, nesse caso é igual a 82
);

// Objeto Botao com tratamento de Bounce
Bounce botaoBoot = Bounce();

const uint8_t PINO_LED_RGB = 48; // Pino do LED REGB soldado no ESP
const uint8_t QUANTIDADE_LEDS = 1;

const int TEMPO_ALARME = 30; // Tempo para verificação até que o alarme dispare

int countdown = TEMPO_ALARME;

bool estadoAlerta = 0; // Dispara o alarme e trava as funções do ESP lateral

bool publicarMsg = false; // Verifica se houve alguma alteração para publicar ao esp central

void tratarMensagemRecebida(const char *topico, const String &mensagem); // Trata da mensagem recebida, verifica se o tópico existe e chama a tratar JSOn Lateral
void tratarJsonLateral(const String &mensagem);                          // Deserializa a mensagem recebida

void configuraLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul); // Seta a cor do RGB

// void tratamentoEspLateral();

void contagemTempo();   //Realiza uma contagem decrescente

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
}

void loop()
{
    garantirWiFiConectado();
    garantirMQTTConectado();
    botaoBoot.update();
    loopMQTT();
    contagemTempo();

    updateEsp();
    
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

    if (strcmp(topico, TOPICOS_PUBLICAR[0]) == 0)
    {
        tratarJsonLateral(mensagem);
        return;
    }

    debugErro("Tópico não tratado: " + String(topico));
}

void tratarJsonLateral(const String &mensagem)
{
    JsonDocument doc;
    DeserializationError erro = deserializeJson(doc, mensagem);

    if (erro)
    {
        debugErro("Json inválido, corrija a formatação.");
        return;
    }

    if (doc["alerta"].is<JsonObject>())
    {
        if (doc["alerta"].is<bool>())
        {
            estadoAlerta = doc["alerta"].as<bool>();
        }
    }
}



void updateEsp()   
{
    if (!estadoAlerta)
    {
        if (botaoBoot.fell())
    {
        publicarMsg = true;
        debugInfo("Resetando Contagem");
        countdown = TEMPO_ALARME;
    }
        if (countdown > 5 && countdown <= TEMPO_ALARME)
        {
            alterarCorLedRGB(0, 255, 0);
            return;
        }
        else if (countdown <= 5)
        {
            alterarCorLedRGB(255, 165, 0);
            return;
        }
        else
        {
            alterarCorLedRGB(255, 0, 0);
            publicarMsg = true;
        }
    }
    else 
    {
        alterarCorLedRGB(255, 0, 0);
        return;
    }
    if(publicarMsg)
    {
        publicarMensagemNoTopico(0, RespostaJson());
        publicarMsg = false;
    }
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

    ledRGB.setPixelColor(0, ledRGB.Color(vermelho, verde, azul));
    ledRGB.show();

    debugInfo("Cor aplicada o LED RGB");
    debugInfo("R: " + String(vermelho));
    debugInfo("G: " + String(verde));
    debugInfo("B: " + String(azul));
}
void contagemTempo() // função que conta o tempo continuamente
{
    if(estadoAlerta)
    {
        countdown = TEMPO_ALARME;
        return;
    }
    unsigned int tempoAtual = millis();
    static unsigned int tempoAnterior = 0;
    if (tempoAtual - tempoAnterior >= 1000)
    {
        if (countdown > 0)
            countdown--;
    }
}

const char* RespostaJson()
{
    JsonDocument doc;
    
    doc["pressionado"] = true;

    String mensagem = "";
    
    serializeJsonPretty(doc, mensagem);

    return mensagem.c_str();
}