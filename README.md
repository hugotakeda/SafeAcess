# 🔒 Sistema Inteligente de Controle de Acesso (ESP32 + Python + Discord)

![Exemplo de Funcionamento do Discord](exemplo.png)
*(Exemplo das notificações recebidas no Discord com o status de Acesso Liberado e Acesso Negado)*

## 📖 Sobre o Projeto

Este projeto implementa um **Sistema Inteligente de Controle de Acesso** combinando hardware de baixo custo (ESP32-CAM) com o poder do processamento na nuvem/local (Python) para reconhecimento facial e notificações em tempo real. 

O sistema é ativado via sensor de movimento (PIR) e captura uma foto, que é enviada para o servidor Python. O servidor avalia se o rosto reconhecido está cadastrado, aciona um relé para abrir a porta, exibe o resultado num display OLED e notifica instantaneamente um servidor do **Discord** com a imagem capturada e o status do acesso.

---

## ✨ Funcionalidades

- 🧑‍💻 **Reconhecimento Facial Preciso:** Baseado na biblioteca `face_recognition` (dlib).
- 💬 **Notificações no Discord:** Alertas em tempo real enviados via Webhook (incluindo imagens do momento).
- 🚪 **Controle de Acesso Físico:** Acionamento de relé para abertura de fechaduras eletrônicas.
- 👀 **Acionamento por Movimento:** Integração com sensor PIR para ativação sob demanda.
- 📺 **Feedback Visual Local:** Status de "Acesso Liberado" ou "Acesso Negado" num Display OLED.

---

## 🛠️ Requisitos e Hardware

### Hardware Necessário
- **Placa:** ESP32 com Câmera (ex: TTGO T-Camera, ESP32-CAM AI-Thinker).
- **Sensores e Atuadores:** Sensor PIR (Movimento), Módulo Relé (Para a fechadura).
- **Display:** Display OLED (I2C) para feedback local.

### Software e Dependências
- **Arduino IDE** (com suporte à placa ESP32 instalado).
- **Python 3.8+** instalado no servidor/computador.
- **Bibliotecas Python:** `Flask`, `requests`, `Pillow`, `face_recognition`, `dlib`, `cmake`.

---

## 📁 Estrutura do Projeto

```text
📦 Projeto de Controle de Acesso
 ┣ 📂 AccessControl/           # Código do firmware ESP32 (Arduino IDE)
 ┃ ┗ 📜 AccessControl.ino 
 ┣ 📂 known_faces/             # Pessoas autorizadas (Fotos com o Nome.jpg)
 ┣ 📂 templates/               # (Opcional) Templates HTML do Flask
 ┣ 📜 cloud_bridge_server.py   # Servidor Python de Processamento e Integração
 ┗ 📜 README.md                # Esta documentação
```

---

## 🚀 Como Configurar e Rodar

### 1. Configurar as Faces Conhecidas
1. Na raiz do projeto, garanta que existe a pasta `known_faces`.
2. Adicione fotos claras do rosto das pessoas autorizadas.
3. O nome do arquivo será o identificador mostrado no acesso! (Ex: `Hugo_Takeda.jpg`, `Joao.png`).

### 2. Configurar o Webhook do Discord
1. No seu servidor do Discord, vá em **Configurações do Canal** > **Integrações** > **Webhooks**.
2. Clique em **Novo Webhook**, dê o nome "Controle de Acesso API" e copie a **URL do Webhook**.
3. Abra o arquivo `cloud_bridge_server.py` e altere a variável:
   ```python
   DISCORD_WEBHOOK_URL = "SUA_URL_COPIADA_AQUI"
   ```

### 3. Executando o Servidor Python
No terminal, dentro da pasta do projeto, instale os requisitos e rode o servidor:
```bash
pip install Flask requests Pillow face_recognition
python cloud_bridge_server.py
```
> **Nota:** Anote o endereço de IP local gerado pelo Flask (ex: `http://192.168.0.X:5000`). Ele será usado no ESP32.

### 4. Configurando e Gravando o ESP32 (`AccessControl.ino`)
1. Abra o arquivo na **Arduino IDE**.
2. Preencha suas credenciais de Wi-Fi:
   ```cpp
   const char* ssid = "SEU_WIFI_SSID";
   const char* password = "SEU_WIFI_SENHA";
   ```
3. Aponte a requisição para o IP correto do seu Servidor Python:
   ```cpp
   String serverUrl = "http://192.168.0.X:5000/recognize";
   ```
4. Verifique as configurações de pinos correspondentes à sua placa ESP32 na IDE.
5. Compile e faça o Upload para a placa.

---

## 📸 Fluxo de Execução
1. O **Sensor PIR** detecta movimento perto da porta.
2. A **Câmera do ESP32** tira uma foto e envia para a API Flask.
3. A **API Python** analisa a biometria na foto comparando a pasta `known_faces`.
4. A API envia o resultado (Liberado/Negado) de volta ao painel local e para o **Discord via Webhook**.
5. Se liberado, o **Relé** atraca por 5 segundos para liberar a entrada.

---
*Desenvolvido para automatizar acessos com segurança e conectividade.*
