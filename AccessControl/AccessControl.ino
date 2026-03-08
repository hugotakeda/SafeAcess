#include "esp_camera.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>

// ==========================================
// Configurações Globais
// ==========================================
const char *ssid = "SSID";
const char *password = "Senha";

// URL do Servidor Python Bridge (Substitua IP_DO_PC pelo IP local do PC onde o
// Python roda)
String serverUrl = "http://SEU_IP_AQUI/recognize";

// ==========================================
// Configuração Hardware - TTGO T-Camera
// ==========================================
#define PWDN_GPIO_NUM 26
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 32
#define SIOD_GPIO_NUM 13
#define SIOC_GPIO_NUM 12
#define Y9_GPIO_NUM 39
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 23
#define Y6_GPIO_NUM 18
#define Y5_GPIO_NUM 15
#define Y4_GPIO_NUM 4
#define Y3_GPIO_NUM 14
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 27
#define HREF_GPIO_NUM 25
#define PCLK_GPIO_NUM 19

// Display OLED
#define I2C_SDA 21
#define I2C_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensores e Atuadores
#define PIR_PIN 33
#define BUTTON_PIN 34
#define RELAY_PIN 2

// Variáveis de Estado
bool motionDetected = false;
unsigned long lastMotionTime = 0;
const int motionDelayCooldown = 10000; // 10 segundos de cooldown entre capturas

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Sistema de Controle de Acesso...");

  // Configurar Pinos
  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Inicializar Display OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println("FALHA: Não foi possível iniciar o SSD1306");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  showTextOnOLED("Iniciando...");

  // Conectar ao Wi-Fi
  setupWiFi();

  // Inicializar Câmera
  setupCamera();

  showTextOnOLED("Sistema Pronto!\nAguardando...");
}

void loop() {
  // Verificar comandos pela Serial (Ex: ADD_USER:nome, REMOVE_USER:nome)
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      handleSerialCommand(command);
    }
  }

  // Verificar PIR
  int pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH && !motionDetected &&
      (millis() - lastMotionTime > motionDelayCooldown)) {
    motionDetected = true;
    lastMotionTime = millis();

    Serial.println("Movimento Detectado!");
    showTextOnOLED("MovimentoDetectado!\nProcessando...");

    // Capturar Foto localmente - SUBSTITUIDO POR WEBCAM USB
    // A câmera não estava funcionando. Agora vamos apenas avisar o Python para
    // tirar a foto!
    showTextOnOLED("Tirando Foto...");

    // Enviar trigger para o servidor Python
    String result = sendTriggerToCloud();

    // Analisar Resposta
    if (result == "GRANTED") {
      Serial.println("Acesso Liberado!");
      showTextOnOLED("ACESSO\nLIBERADO!");

      // Acionar Relé por 5 segundos
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY_PIN, LOW);
    } else if (result == "DENIED") {
      Serial.println("Acesso Negado!");
      showTextOnOLED("ACESSO\nNEGADO!");
      delay(3000);
    } else {
      Serial.println("Erro Servidor (API): " + result);
      showTextOnOLED("Erro API");
      delay(3000);
    }

    motionDetected = false;
    showTextOnOLED("Sistema Pronto!\nAguardando...");
  }

  delay(10);
}

void setupWiFi() {
  showTextOnOLED("Conectando Wi-Fi..");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("Falha ao conectar Wi-Fi");
    showTextOnOLED("Falha Wi-Fi");
  }
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Se tiver PSRAM, melhor resolução
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Initialização da câmera falhou: 0x%x", err);
    showTextOnOLED("Erro Camera");
    while (true) {
      delay(1000);
    }
  }
}

String sendTriggerToCloud() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado!");
    return "ERROR_WIFI";
  }

  HTTPClient http;
  http.begin(serverUrl);

  // Enviamos uma requisição vazia apenas para acionar a rota
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("trigger=true");

  String payload = "";
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Resposta Servidor: " + payload);
  } else {
    Serial.println("Erro no envio POST, HTTP_CODE: " + String(httpCode));
    http.end();
    return "ERROR_CONNECTION";
  }
  http.end();

  if (httpCode == 200) {
    if (payload.indexOf("GRANTED") != -1)
      return "GRANTED";
    if (payload.indexOf("DENIED") != -1)
      return "DENIED";
  }

  return "ERROR_SERVER";
}

void showTextOnOLED(String text) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(text);
  display.display();
}

void handleSerialCommand(String cmd) {
  // Exemplo: ADD_USER:takeda
  cmd.toUpperCase();
  Serial.println("Processando comando: " + cmd);

  // Aqui poderia haver lógica para capturar uma foto via Serial
  // e direcioná-la para o endpoint /enroll no Python.
  if (cmd.startsWith("ADD_USER:")) {
    String username = cmd.substring(9);
    Serial.println("Comando para adicionar: " + username);
    Serial.println("DICA: Posicione a pessoa, aguarde movimento, e mude o "
                   "endpoint no python. (Implementação futura)");
  }
}
