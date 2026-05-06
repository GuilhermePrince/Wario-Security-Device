//! MqttManager.h

#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <Arduino.h>

void configurarMQTT();
void conectarMQTT();
void garantirMQTTConectado();
void loopMQTT();

void publicarMensagem(const char *topico, const char *mensagem);

void publicarMensagemNoTopico(int indiceTopico, const char *mensagem);

bool mqttEstaConectado();

const char *obterTopicoPublicacao(int indiceTopico);
const char *obterTopicoRecebimento(int indiceTopico);

typedef void (*CallbackMensagemMQTT)(const char *topico, const String &mensagem);

void registrarCallbackMensagem(CallbackMensagemMQTT callback);

int obterTotalTopicosRecebimento();
int obterTotalTopicosPublicacao();

#endif