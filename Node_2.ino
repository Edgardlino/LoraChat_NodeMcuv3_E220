/*
 * LoRa Chat Node 2 (sem OLED, com Web UI)
 * Recebe e envia mensagens LoRa, mostra numa interface web.
 * Funciona como Access Point Wi-Fi.
 */

// --- Bibliotecas ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <ArduinoJson.h> // Para formatar o histórico
#include <list>          // Para guardar o histórico

// --- Configurações ---
const char* ap_ssid = "LoRaChatNode2";     // Nome da rede Wi-Fi deste nó (DIFERENTE DO NODE 1)
const char* ap_password = "password123"; // Senha
const char* nodeIdentifier = "Node2";    // Identificador deste nó
#define HISTORY_MAX_SIZE 10              // Número máximo de mensagens no histórico

// --- Pinos LoRa ---
#define LORA_RX_PIN 12 // D6
#define LORA_TX_PIN 14 // D5
#define LORA_M0_PIN 16 // D0
#define LORA_M1_PIN 15 // D8
#define LORA_AUX_PIN 13 // D7

// --- LoRa ---
SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);
LoRa_E220 e220ttl(&loraSerial, LORA_AUX_PIN, LORA_M0_PIN, LORA_M1_PIN);
byte peerAddrH = 0; // Endereço do outro nó
byte peerAddrL = 0;
byte peerChan = 65; // Canal de comunicação

// --- Web Server ---
ESP8266WebServer server(80);
std::list<String> chatHistory;
bool newWebMessageAvailable = false;

// --- Funções Auxiliares ---
void addMessageToHistory(const String& msg) {
  chatHistory.push_back(msg);
  if (chatHistory.size() > HISTORY_MAX_SIZE) {
    chatHistory.pop_front();
  }
  newWebMessageAvailable = true;
}

String cleanString(const String& input) {
  String output = "";
  for (int i = 0; i < input.length(); i++) {
    if (isPrintable(input.charAt(i))) {
      output += input.charAt(i);
    }
  }
  output.trim();
  return output;
}

// --- Funções Web Server ---

// Servir a página HTML principal (similar, muda apenas o título)
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><title>LoRa Chat - Node2</title><meta name='viewport' content='width=device-width, initial-scale=1'>
<style>body{font-family: sans-serif;} #chatbox{height: 300px; border: 1px solid #ccc; overflow-y: scroll; margin-bottom: 10px; padding: 5px;}
form{display: flex;} input[type=text]{flex-grow: 1; padding: 8px;} button{padding: 8px; margin-left: 5px;}</style></head>
<body><h1>LoRa Chat - Node2</h1><div id='chatbox'>Carregando histórico...</div>
<form onsubmit='sendMessage(); return false;'>
<input type='text' id='msg' placeholder='Digite sua mensagem...' maxlength='50' required><button type='submit'>Enviar</button>
</form>
<script>
let lastMessageCount = 0;
function fetchMessages(){fetch('/messages').then(response=>response.json()).then(data=>{let chatbox=document.getElementById('chatbox');if(data.length!==lastMessageCount){chatbox.innerHTML='';data.forEach(msg=>{let p=document.createElement('p');p.textContent=msg;chatbox.appendChild(p);});chatbox.scrollTop=chatbox.scrollHeight;lastMessageCount=data.length;}}).catch(error=>console.error('Error fetching messages:',error));}
function sendMessage(){let msgInput=document.getElementById('msg');let message=msgInput.value.trim();if(message){fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded',},body:'message='+encodeURIComponent(message)}).then(response=>{if(response.ok){msgInput.value='';fetchMessages();}else{alert('Erro ao enviar mensagem');}}).catch(error=>console.error('Error sending message:',error));}}
setInterval(fetchMessages, 3000);
window.onload=fetchMessages;
</script></body></html>
)rawliteral";
  server.send(200, "text/html", html);
}

// Enviar histórico (idêntica ao Node 1)
void handleMessages() {
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.to<JsonArray>();
  for (const String& msg : chatHistory) {
    array.add(msg);
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
  newWebMessageAvailable = false;
}

// Receber e enviar (idêntica ao Node 1)
void handleSend() {
  if (server.hasArg("message")) {
    String messageToSend = server.arg("message");
    messageToSend = cleanString(messageToSend);

    if (messageToSend.length() > 0) {
      String formattedMsg = String(nodeIdentifier) + ": " + messageToSend;
      Serial.println("Enviando LoRa: " + formattedMsg);
      ResponseStatus rs = e220ttl.sendFixedMessage(peerAddrH, peerAddrL, peerChan, formattedMsg);

      if (rs.code == E220_SUCCESS) {
        Serial.println("Envio LoRa OK!");
        addMessageToHistory("You: " + messageToSend);
        server.send(200, "text/plain", "OK");
      } else {
        Serial.print("Falha no envio LoRa! ");
        Serial.println(rs.getResponseDescription());
        server.send(500, "text/plain", "LoRa Send Failed");
      }
    } else {
      server.send(400, "text/plain", "Empty message");
    }
  } else {
    server.send(400, "text/plain", "No message argument");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nInicializando LoRa Chat Node 2...");

  // Inicializa LoRa
  loraSerial.begin(9600);
  e220ttl.begin();
  Serial.println("Módulo LoRa E220 inicializado.");
  addMessageToHistory("System: Node 2 iniciado.");

  // --- Configura AP Wi-Fi ---
  Serial.print("Configurando Access Point: ");
  Serial.println(ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // --- Configura Web Server ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/messages", HTTP_GET, handleMessages);
  server.on("/send", HTTP_POST, handleSend);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// --- LOOP ---
void loop() {
  server.handleClient(); // Processa requisições web

  // Verifica se há mensagens LoRa recebidas
  if (e220ttl.available()) {
    Serial.println("\n*** LoRa AVAILABLE! ***");

    ResponseContainer rc = e220ttl.receiveMessageRSSI();

    if (rc.status.code == E220_SUCCESS) {
      String receivedData = rc.data;
      int rssiValue = (int8_t)rc.rssi;
      Serial.println("Dados Brutos Recebidos: '" + receivedData + "'");
      Serial.println("RSSI: " + String(rssiValue) + "dBm");

      String cleanedMsg = cleanString(receivedData);

      // Adiciona ao histórico geral
      addMessageToHistory(cleanedMsg);
      Serial.println("Dados Limpos Guardados: '" + cleanedMsg + "'");

    } else {
      Serial.print("Erro ao receber dados LoRa: ");
      Serial.println(rc.status.getResponseDescription());
      addMessageToHistory("System: Erro RX LoRa - " + rc.status.getResponseDescription());
    }
     // Não usa rc.close() para ResponseContainer
  }
  yield(); // Permite processos de background
}