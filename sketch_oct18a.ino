#include <SoftwareSerial.h>

// --- Definição dos pinos dos sensores US-100 ---
#define RX_FRONT 5   // Arduino recebe do TX do US-100 da frente
#define TX_FRONT 4   // Arduino envia para RX do US-100 da frente
#define RX_BACK 3    // Arduino recebe do TX do US-100 de trás
#define TX_BACK 2    // Arduino envia para RX do US-100 de trás

// --- Definição dos pinos dos motores vibracall ---
#define MOTOR_FRONT 9
#define MOTOR_BACK 10

// --- Instâncias de SoftwareSerial para os sensores ---
SoftwareSerial us100_front(RX_FRONT, TX_FRONT);
SoftwareSerial us100_back(RX_BACK, TX_BACK);

// --- Instância de SoftwareSerial para comunicação com NodeMCU ---
// RX=12, TX=13
SoftwareSerial mySerial(12, 13);

// ----- CONFIGURAÇÕES DO FILTRO -----
float alpha = 0.7;      // fator de suavização EMA
float ema_front = -1;   // valor inicial do EMA (frente)
float ema_back  = -1;   // valor inicial do EMA (trás)

int leitura_front[3];   // buffer para mediana (frente)
int leitura_back[3];    // buffer para mediana (trás)

// ----- LIMIAR DE DISTÂNCIA -----
const int LIMIAR = 140; // cm -> quando alguém estiver a menos de 1,4m

// Função para calcular a mediana de 3 valores
int median3(int a, int b, int c) {
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  else if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  else return c;
}

void setup() {
  Serial.begin(9600);       // Debug no PC
  mySerial.begin(9600);     // Comunicação com NodeMCU

  us100_front.begin(9600);
  us100_back.begin(9600);

  pinMode(MOTOR_FRONT, OUTPUT);
  pinMode(MOTOR_BACK, OUTPUT);

  Serial.println("US-100 Frente/Trás + Vibracall + Envio NodeMCU");
}

void loop() {
  // ---------- SENSOR DA FRENTE ----------
  for (int i = 0; i < 3; i++) {
    leitura_front[i] = medirDistancia(us100_front);
    delay(20);
  }
  int mediana_front = median3(leitura_front[0], leitura_front[1], leitura_front[2]);

  if (ema_front < 0) ema_front = mediana_front;
  else {
    if (mediana_front < ema_front - 30) ema_front = mediana_front;
    else ema_front = alpha * mediana_front + (1 - alpha) * ema_front;
  }

  // ---------- SENSOR DE TRÁS ----------
  for (int i = 0; i < 3; i++) {
    leitura_back[i] = medirDistancia(us100_back);
    delay(20);
  }
  int mediana_back = median3(leitura_back[0], leitura_back[1], leitura_back[2]);

  if (ema_back < 0) ema_back = mediana_back;
  else {
    if (mediana_back < ema_back - 30) ema_back = mediana_back;
    else ema_back = alpha * mediana_back + (1 - alpha) * ema_back;
  }

  // ---------- MOSTRA RESULTADOS ----------
  Serial.print("Frente -> EMA: ");
  Serial.print(ema_front, 1);

  if (ema_front > 0 && ema_front < LIMIAR) {
    int intensidade = map(ema_front, 10, LIMIAR, 255, 80);
    intensidade = constrain(intensidade, 80, 255);
    analogWrite(MOTOR_FRONT, intensidade);

    int porcentagem = map(intensidade, 0, 255, 0, 100);
    Serial.print(" | Motor Frente ON (");
    Serial.print(porcentagem);
    Serial.print("%)");
  } else {
    analogWrite(MOTOR_FRONT, 0);
    Serial.print(" | Motor Frente OFF");
  }
  Serial.println();

  Serial.print("Trás   -> EMA: ");
  Serial.print(ema_back, 1);

  if (ema_back > 0 && ema_back < LIMIAR) {
    int intensidade = map(ema_back, 10, LIMIAR, 255, 80);
    intensidade = constrain(intensidade, 80, 255);
    analogWrite(MOTOR_BACK, intensidade);

    int porcentagem = map(intensidade, 0, 255, 0, 100);
    Serial.print(" | Motor Trás ON (");
    Serial.print(porcentagem);
    Serial.print("%)");
  } else {
    analogWrite(MOTOR_BACK, 0);
    Serial.print(" | Motor Trás OFF");
  }
  Serial.println();
  Serial.println("------------------------");

  // ---------- ENVIA PARA NODEMCU ----------
static unsigned long lastSend = 0;
if (millis() - lastSend > 1000) {
  mySerial.print(ema_front, 1);
  mySerial.print(",");
  mySerial.println(ema_back, 1);
  lastSend = millis();
}

  // Também mostra no monitor do Arduino
  Serial.print("Enviando ao NodeMCU: ");
  Serial.print(ema_front, 1);
  Serial.print(",");
  Serial.println(ema_back, 1);

  delay(150);
}

// Função para medir distância via US-100 (UART)
int medirDistancia(SoftwareSerial &sensor) {
  sensor.listen();
  sensor.write(0x55);
  delay(70); // tempo mínimo do US-100 (~65ms)

  if (sensor.available() >= 2) {
    int highByte = sensor.read();
    int lowByte  = sensor.read();
    int distancia = (highByte << 8) + lowByte;
    return distancia / 10; // em cm
  }
  return -1; // erro
}