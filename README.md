# LoraChat
# ESP8266 LoRa Chat com OLED e Interface Web

Este projeto implementa um sistema de chat bidirecional simples usando dois módulos ESP8266 (NodeMCU) e módulos LoRa Ebyte E220 (baseados no LLCC68). Um dos nós possui um display OLED SSD1306 para mostrar status e a última mensagem recebida, enquanto ambos os nós hospedam uma interface web para visualização do histórico e envio de mensagens. Cada nó opera como um Access Point (AP) Wi-Fi independente.

## Funcionalidades

* **Chat Bidirecional via LoRa:** Envio e recebimento de mensagens de texto entre os dois nós.
* **Interface Web:** Cada nó possui uma interface web acessível via navegador para:
    * Visualizar o histórico das últimas mensagens (enviadas e recebidas).
    * Digitar e enviar novas mensagens.
    * Atualização automática do histórico (via AJAX polling).
* **Display OLED (Nó 1):** Mostra informações de status (Endereço LoRa, Canal), a última mensagem recebida e o RSSI (Indicador de Força do Sinal Recebido).
* **Modo Access Point (AP):** Cada nó cria sua própria rede Wi-Fi, permitindo que dispositivos se conectem diretamente a eles para usar a interface de chat.
* **Módulos Ebyte E220:** Utiliza os módulos LoRa E220 (testado com E220-900T22D) para comunicação de longo alcance.
* **RSSI:** Exibe a força do sinal da última mensagem LoRa recebida.

## Hardware Necessário

* NodeMCU ESP8266 (ou placa similar) x 2
* Módulo LoRa Ebyte E220-900T22D (ou compatível com a biblioteca) x 2
* Antena para os módulos LoRa x 2
* Display OLED SSD1306 I2C (128x64) x 1 (Apenas para o Nó 1)
* Fios Jumper Macho-Fêmea e Macho-Macho
* Fonte de alimentação para os NodeMCUs (Cabos USB, adaptadores, etc.)

## Software e Bibliotecas (Arduino IDE)

* **Plataforma:** ESP8266 Community Core (Instalar via Gerenciador de Placas)
* **Bibliotecas:**
    * `ESP8266WiFi` (Integrada ao Core)
    * `ESP8266WebServer` (Integrada ao Core)
    * `SoftwareSerial` (Integrada ao Core - para comunicação LoRa)
    * `LoRa_E220` by Renzo Mischianti (Instalar via Gerenciador de Bibliotecas)
    * `Wire` (Integrada ao Core - para I2C do OLED)
    * `ESP8266 and ESP32 Oled Driver for SSD1306 displays` by ThingPulse (Instalar via Gerenciador de Bibliotecas - procurar por `ssd1306 ThingPulse`)
    * `OLEDDisplayUi` by ThingPulse (Instalada como dependência da anterior)
    * `ArduinoJson` by Benoit Blanchon (Instalar via Gerenciador de Bibliotecas)

## Fiação

**É crucial que AMBOS os módulos LoRa E220 sejam configurados com os mesmos parâmetros antes de usar os sketches principais.**

**Nó 1 (Com OLED):**

* **E220 LoRa:**
    * `M0`  -> NodeMCU `D0` (GPIO16)
    * `M1`  -> NodeMCU `D8` (GPIO15)
    * `RXD` -> NodeMCU `D5` (GPIO14) - *Ligação Cruzada*
    * `TXD` -> NodeMCU `D6` (GPIO12) - *Ligação Cruzada*
    * `AUX` -> NodeMCU `D7` (GPIO13)
    * `VCC` -> NodeMCU `3.3V` ou `VIN` (Verificar tensão do seu módulo/adaptador)
    * `GND` -> NodeMCU `GND`
* **OLED SSD1306 (I2C):**
    * `SDA` -> NodeMCU `D2` (GPIO4)
    * `SCL` -> NodeMCU `D1` (GPIO5)
    * `VCC` -> NodeMCU `3.3V`
    * `GND` -> NodeMCU `GND`

**Nó 2 (Sem OLED):**

* **E220 LoRa:** (Mesma fiação do Nó 1)
    * `M0`  -> NodeMCU `D0` (GPIO16)
    * `M1`  -> NodeMCU `D8` (GPIO15)
    * `RXD` -> NodeMCU `D5` (GPIO14) - *Ligação Cruzada*
    * `TXD` -> NodeMCU `D6` (GPIO12) - *Ligação Cruzada*
    * `AUX` -> NodeMCU `D7` (GPIO13)
    * `VCC` -> NodeMCU `3.3V` ou `VIN` (Verificar tensão do seu módulo/adaptador)
    * `GND` -> NodeMCU `GND`

## Configuração Prévia Obrigatória (Módulos LoRa)

Antes de carregar os sketches principais do chat, ambos os módulos E220 **precisam** ter configurações idênticas para poderem comunicar. Use o sketch `LoRa_Configuration_Sketch.ino` (incluído neste repositório) para isso:

1.  Carregue o sketch `LoRa_Configuration_Sketch.ino` no **primeiro** NodeMCU.
2.  Abra o Serial Monitor (115200 bps). Ele lerá a configuração atual, definirá a nova configuração e a lerá novamente para confirmação. Verifique se não há erros e anote os parâmetros definidos (especialmente Addr=0x0000, Chan=65, RSSI=Enabled, Mode=Fixed).
3.  Carregue o **mesmo** sketch `LoRa_Configuration_Sketch.ino` no **segundo** NodeMCU.
4.  Abra o Serial Monitor para ele e confirme que a configuração gravada é **idêntica** à do primeiro nó.

*Nota: O sketch de configuração está definido para usar os mesmos pinos `SoftwareSerial` (D5/D6) dos sketches principais.*

## Instalação e Execução

1.  Clone este repositório ou baixe os arquivos.
2.  Configure o Arduino IDE com a placa ESP8266 e instale as bibliotecas listadas acima.
3.  Monte o hardware conforme a seção "Fiação".
4.  Execute a "Configuração Prévia Obrigatória (Módulos LoRa)" em ambos os nós.
5.  Abra o arquivo `Node1_With_OLED.ino`. Se desejar, altere o `ap_ssid` e `ap_password` (Nome e Senha da rede Wi-Fi que este nó criará). Carregue este código no NodeMCU com o OLED.
6.  Abra o arquivo `Node2_No_OLED.ino`. Se desejar, altere o `ap_ssid` (deve ser diferente do Node1) e `ap_password`. Carregue este código no outro NodeMCU.
7.  Abra os Monitores Seriais (115200 bps) para ambos os nós para verificar o status e obter os endereços IP (normalmente `192.168.4.1` para ambos, pois são APs separados).

## Como Usar

1.  Ligue ambos os NodeMCUs.
2.  Com um telemóvel, tablet ou PC, conecte-se à rede Wi-Fi criada pelo Node 1 (ex: `LoRaChatNode1`).
3.  Abra um navegador web e acesse o endereço IP do Node 1 (normalmente `http://192.168.4.1`). Você verá a interface de chat do Node 1.
4.  Com outro dispositivo (ou o mesmo, se conseguir alternar), conecte-se à rede Wi-Fi criada pelo Node 2 (ex: `LoRaChatNode2`).
5.  Abra um navegador web e acesse o endereço IP do Node 2 (`http://192.168.4.1`). Você verá a interface de chat do Node 2.
6.  Digite mensagens em qualquer uma das interfaces e clique em "Enviar". A mensagem deve aparecer no histórico de ambas as interfaces web (após um pequeno delay) e a última mensagem recebida aparecerá no display OLED do Node 1, junto com o RSSI.

## Troubleshooting

* **Interface Web não carrega:**
    * Verifique se está conectado à rede Wi-Fi correta (`LoRaChatNode1` ou `LoRaChatNode2`).
    * Confirme o endereço IP no Serial Monitor (deve ser 192.168.4.1).
    * Verifique a alimentação do NodeMCU.
    * No Node 1, tente reduzir o `ui.setTargetFPS()` no `setup()` se a interface estiver lenta ou não responder (pode ser sobrecarga).
    * Verifique o Serial Monitor em busca de mensagens de erro.
    * Limpe o cache do navegador e tente recarregar (Ctrl+Shift+R).
* **Mensagens não chegam / Não envia:**
    * Confirme se **ambos** os módulos LoRa foram configurados com **parâmetros idênticos** usando o sketch de configuração (especialmente Canal, Modo Fixo, RSSI ativado).
    * Verifique a fiação LoRa (RX/TX cruzados, M0, M1, AUX, GND).
    * Verifique se as antenas estão bem conectadas.
    * Verifique o Serial Monitor de ambos os nós em busca de erros de envio (`Falha no envio LoRa!`) ou recepção (`Erro ao receber dados LoRa:`).
    * Verifique a distância e possíveis obstáculos entre os módulos.
* **Display OLED não funciona (Node 1):**
    * Verifique a fiação I2C (SDA, SCL, VCC, GND).
    * Confirme o endereço I2C do display (normalmente `0x3C`).

## Possíveis Melhorias Futuras

* Usar WebSockets em vez de AJAX polling para atualizações de chat mais instantâneas.
* Armazenar o histórico de chat na memória flash (SPIFFS/LittleFS) para persistência.
* Implementar nomes de utilizador ou identificadores mais dinâmicos.
* Adicionar confirmações de entrega de mensagens (Acknowledgements).
* Integrar com uma rede Wi-Fi existente em vez do modo AP (exigiria mudanças significativas).
* Usar HardwareSerial em placas com mais UARTS (como ESP32) para comunicação LoRa mais robusta.
* Adicionar criptografia às mensagens LoRa.

## Licença

MIT License
