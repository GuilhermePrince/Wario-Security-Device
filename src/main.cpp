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

const uint8_t QUANTIDADE_LEDS = 1;
const uint8_t PINO_LED_RGB = 48; // Pino do LED REGB soldado no ESP

// Objeto LED RGB
Adafruit_NeoPixel ledRGB(
    QUANTIDADE_LEDS,
    PINO_LED_RGB,
    NEO_GRB + NEO_KHZ800 // Duas constantes que definem a saidas do rgb e a frquencia de propagacao da informacao entre os LED`s, nesse caso é igual a 82
);

// Objeto Botao com tratamento de Bounce
Bounce botaoBoot = Bounce();


const int TEMPO_ALARME = 15; // Tempo para verificação até que o alarme dispare

int countdown = TEMPO_ALARME;

bool estadoAlerta = 0; // Dispara o alarme e trava as funções do ESP lateral

bool publicarMsg = false; // Verifica se houve alguma alteração para publicar ao esp central

void tratarMensagemRecebida(const char *topico, const String &mensagem); // Trata da mensagem recebida, verifica se o tópico existe e chama a tratar JSOn Lateral
void tratarJsonLateral(const String &mensagem);                          // Deserializa a mensagem recebida
void updateEsp();
String RespostaJson();
void configuraLedRGB();
void alterarCorLedRGB(int vermelho, int verde, int azul); // Seta a cor do RGB

// void tratamentoEspLateral();

void contagemTempo(); // Realiza uma contagem decrescente

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

    if (strcmp(topico, TOPICOS_RECEBER[0]) == 0)
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

    if (doc["alerta"].is<bool>())
    {
        estadoAlerta = doc["alerta"].as<bool>();

        debugInfo("Estado alerta: " + String(estadoAlerta));
    }
}

void updateEsp()
{
    if (!estadoAlerta)
    {
        if (botaoBoot.fell())
        {
            publicarMsg = true;
            countdown = TEMPO_ALARME;
            debugInfo("Resetando contagem");
        }

        if (countdown > 5)
        {
            alterarCorLedRGB(0,255,0);
        }
        else if (countdown > 0)
        {
            alterarCorLedRGB(255,165,0);
        }
        else
        {
            alterarCorLedRGB(255,0,0);
            estadoAlerta = true;
            publicarMsg = true;
        }
    }
    else
    {
        alterarCorLedRGB(255,0,0);
    }

    if (publicarMsg)
    {
        String msg = RespostaJson();

        publicarMensagemNoTopico(0, msg.c_str());

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
    if (estadoAlerta)
    {
        countdown = TEMPO_ALARME;
        return;
    }
    unsigned int tempoAtual = millis();
    static unsigned int tempoAnterior = 0;
    if (tempoAtual - tempoAnterior >= 1000)
    {
    tempoAnterior = tempoAtual;

    if (countdown > 0)
        countdown--;
    }
}

String RespostaJson()
{
    JsonDocument doc;

    doc["pressionado"] = true;
    
    
    String mensagem;

    serializeJson(doc, mensagem);

    return mensagem;
}