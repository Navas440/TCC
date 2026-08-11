#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>

// --- CONFIGURAÇÕES DO SEU WI-FI ---
const char* ssid     = "MotoG";        // Nome da rede Wi-Fi
const char* password = "vitoria00";    // Senha da rede Wi-Fi

// --- CONFIGURAÇÕES DO THINGSPEAK ---
const char* apiKey   = "4759EDP53WMGOXY8";  // Write API Key do seu canal
WiFiClient client;

// --- SERIAL ENTRE ARDUINO E NODEMCU ---
// Agora: D6 = RX, D5 = TX
SoftwareSerial arduinoSerial(D6, D5); // RX, TX

void setup() {
  Serial.begin(115200);        // Debug no PC
  arduinoSerial.begin(9600);   // Comunicação com Arduino

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
}

void loop() {
  // Se houver dados do Arduino, lê a linha completa
  if (arduinoSerial.available()) {
    String dados = arduinoSerial.readString(); // lê até timeout
    dados.trim();

    Serial.println("Recebido do Arduino: " + dados);

    // Aqui você pode enviar para o ThingSpeak se quiser
    if (WiFi.status() == WL_CONNECTED && dados.length() > 0) {
      HTTPClient http;
      http.begin(client, "http://api.thingspeak.com/update");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Supondo que os dados venham no formato "123,456"
      int separador = dados.indexOf(',');
      if (separador > 0) {
        String frente = dados.substring(0, separador);
        String costa  = dados.substring(separador + 1);

        String postData = "api_key=" + String(apiKey) +
                          "&field1=" + frente +
                          "&field2=" + costa;

        int httpCode = http.POST(postData);
        Serial.println("HTTP code: " + String(httpCode));

        if (httpCode == 200) {
          Serial.println("Enviado ao ThingSpeak ✅");
        } else {
          Serial.println("Falha ao enviar ❌");
        }
      }
      http.end();
    }
  }

  delay(16000); // respeita limite do ThingSpeak (>=15s)
}
