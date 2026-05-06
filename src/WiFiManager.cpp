//* WiFiManager.cpp

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "WiFiManager.h"
#include "secrets.h"
#include "DebugManager.h"

bool wifiConectado()
{
    return WiFi.status() == WL_CONNECTED;
}

void conectarWiFi()
{
    debugInfo("====================================");
    debugInfo("Iniciando conexão WiFi...");
    debugInfo("====================================");

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_SENHA);

    debugInfo("conectando");

    int tentativasWiFi = 0;
    const int maxTentativasWiFi = 30;

    while (WiFi.status() != WL_CONNECTED && tentativasWiFi < maxTentativasWiFi)
    {
        delay(500);
        debugInfoSemLinha(".");
        tentativasWiFi++;
    }

    debugInfo("");

    if (WiFi.status() == WL_CONNECTED)
    {
        debugInfo("WiFi conectado com sucesso!");
        debugInfoSemLinha("[INFO] Endereço IP: ");
        debugInfoSemLinha( WiFi.localIP().toString());
        debugInfoSemLinha("\n\r");
    }

    else
    {
        debugErro("Falha ao conectar no WiFi.");
        debugErro("Verifique SSID, senha e sinal de rede.");
    }
}

void garantirWiFiConectado()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        debugErro("WiFi desconectado. Tentando reconectar...");
        conectarWiFi();
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        debugErro("Não foi possível reconectar ao WiFi.");
    }
}