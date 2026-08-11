void setup() {
  Serial.begin(9600); // Usa RX(0) e TX(1)
}

void loop() {
  Serial.println("223,156");  // Envia dados simulados para o NodeMCU
  delay(2000);
}
