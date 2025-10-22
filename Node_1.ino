/*
 * LoRa Chat Node 1 (com OLED e Web UI)
 * Recebe e envia mensagens LoRa, mostra no OLED e numa interface web.
 * Funciona como Access Point Wi-Fi.
 */

// --- Bibliotecas ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include <ArduinoJson.h> // Para formatar o histórico
#include <list>          // Para guardar o histórico

// --- Configurações ---
const char* ap_ssid = "LoRaChatNode1";     // Nome da rede Wi-Fi deste nó
const char* ap_password = "password123"; // Senha
const char* nodeIdentifier = "Node1";    // Identificador deste nó
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
byte peerAddrH = 0; // Endereço do outro nó (0 para broadcast/simplicidade)
byte peerAddrL = 0;
byte peerChan = 65; // Canal de comunicação

// --- Display OLED ---
SSD1306Wire display(0x3c, SDA, SCL);
OLEDDisplayUi ui ( &display );
String oledLastMsg = "Aguardando..."; // Variável separada para o OLED
String oledLastRSSI = "RSSI: --";

// --- Web Server ---
ESP8266WebServer server(80);
std::list<String> chatHistory; // Guarda o histórico de mensagens
bool newWebMessageAvailable = false; // Flag para indicar nova mensagem via web

// --- Funções Auxiliares ---
// Adiciona mensagem ao histórico (com limite de tamanho)
void addMessageToHistory(const String& msg) {
  chatHistory.push_back(msg);
  if (chatHistory.size() > HISTORY_MAX_SIZE) {
    chatHistory.pop_front(); // Remove a mais antiga
  }
  newWebMessageAvailable = true; // Sinaliza que há novas mensagens para o web client
}

// Limpa caracteres não imprimíveis
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

// --- Funções Display OLED ---
void oledStatusFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 5 + y, String(nodeIdentifier) + " Status");
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 22 + y, "Addr: 0x00"); // Endereço deste nó
  display->drawString(0 + x, 40 + y, "Ch: " + String(peerChan));
}

void oledMessageFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 5 + y, "Ultima RX:");
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 22 + y, oledLastMsg.substring(0, 15)); // Mostra só o início
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 45 + y, oledLastRSSI);
  display->display(); // Força atualização
}

FrameCallback oledFrames[] = { oledStatusFrame, oledMessageFrame };
int oledFrameCount = 2;
void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
}
OverlayCallback oledOverlays[] = { clockOverlay }; // Reutiliza a função vazia
int oledOverlaysCount = 1;


// --- Funções Web Server ---

// Servir a página HTML principal
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><title>LoRa Chat - Node1</title><meta name='viewport' content='width=device-width, initial-scale=1'>
<style>body{font-family: sans-serif;} #chatbox{height: 300px; border: 1px solid #ccc; overflow-y: scroll; margin-bottom: 10px; padding: 5px;}
form{display: flex;} input[type=text]{flex-grow: 1; padding: 8px;} button{padding: 8px; margin-left: 5px;}</style></head>
<body><h1>LoRa Chat - Node1</h1><div id='chatbox'>Carregando histórico...</div>
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

// Enviar histórico de mensagens como JSON
void handleMessages() {
  StaticJsonDocument<1024> doc; // Ajuste o tamanho conforme necessário
  JsonArray array = doc.to<JsonArray>();
  for (const String& msg : chatHistory) {
    array.add(msg);
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
  newWebMessageAvailable = false; // Reset a flag após enviar
}

// Receber mensagem via POST e enviar via LoRa
void handleSend() {
  if (server.hasArg("message")) {
    String messageToSend = server.arg("message");
    messageToSend = cleanString(messageToSend); // Limpa a mensagem

    if (messageToSend.length() > 0) {
      String formattedMsg = String(nodeIdentifier) + ": " + messageToSend;
      Serial.println("Enviando LoRa: " + formattedMsg);

      // Envia a mensagem formatada
      ResponseStatus rs = e220ttl.sendFixedMessage(peerAddrH, peerAddrL, peerChan, formattedMsg);

      if (rs.code == E220_SUCCESS) {
        Serial.println("Envio LoRa OK!");
        addMessageToHistory("You: " + messageToSend); // Adiciona ao histórico local
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
  Serial.println("\n\nInicializando LoRa Chat Node 1...");

  // Inicializa LoRa
  loraSerial.begin(9600);
  e220ttl.begin();
  Serial.println("Módulo LoRa E220 inicializado.");
  addMessageToHistory("System: Node 1 iniciado.");

  // --- Configurações Display ---
  ui.setTargetFPS(30);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(TOP);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(oledFrames, oledFrameCount);
  ui.setOverlays(oledOverlays, oledOverlaysCount);
  ui.init();
  display.flipScreenVertically();
  Serial.println("Display OLED OK.");

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
  ui.update();           // Atualiza o display OLED

  // Verifica se há mensagens LoRa recebidas
  if (e220ttl.available()) {
    Serial.println("\n*** LoRa AVAILABLE! ***");

    ResponseContainer rc = e220ttl.receiveMessageRSSI();

    if (rc.status.code == E220_SUCCESS) {
      String receivedData = rc.data;
      int rssiValue = (int8_t)rc.rssi; // Interpreta como byte sinalizado
      Serial.println("Dados Brutos Recebidos: '" + receivedData + "'");
      Serial.println("RSSI Bruto: " + String(rc.rssi) + ", Convertido: " + String(rssiValue) + "dBm");

      String cleanedMsg = cleanString(receivedData);

      // Atualiza variáveis do OLED
      oledLastMsg = cleanedMsg;
      oledLastRSSI = "RSSI: " + String(rssiValue) + "dBm";

      // Adiciona ao histórico geral (sem o prefixo "You:")
      addMessageToHistory(cleanedMsg);
      Serial.println("Dados Limpos Guardados: '" + cleanedMsg + "'");

    } else {
      Serial.print("Erro ao receber dados LoRa: ");
      Serial.println(rc.status.getResponseDescription());
      oledLastMsg = "Erro RX"; // Atualiza OLED em caso de erro
      oledLastRSSI = "RSSI: Erro";
      addMessageToHistory("System: Erro RX LoRa - " + rc.status.getResponseDescription());
    }
     // Não usa rc.close() para ResponseContainer
  }

  // Pequeno delay/yield para estabilidade e permitir processos de background
  // delay(1); // Pode ser muito curto
  yield(); // Melhor para ESP8266
}