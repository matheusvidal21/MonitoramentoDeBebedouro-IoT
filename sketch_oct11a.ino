#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "AdafruitIO_WiFi.h"

WiFiClient ESPWiFiClient;
PubSubClient mqtt_client(ESPWiFiClient);


//variaveis do WiFi e do MQTT
const char* WIFI_SSID = "projeto";
const char* WIFI_PASS = "projeto1";
int wifi_timeout = 200000;

const char* mqtt_broker = "io.adafruit.com";
const int mqtt_port = 1883;
int mqtt_timeout = 10000;

//variaveis do adafruit
const char* mqtt_usernameAdafruitIO = "matheusvidal";
const char* mqtt_keyAdafruitIO = "aio_XDJT01qkap2W2lPyYBqKkts2nfhc";


#if defined(USE_AIRLIFT) || defined(ADAFRUIT_METRO_M4_AIRLIFT_LITE) ||         \
    defined(ADAFRUIT_PYPORTAL)
// Configure the pins used for the ESP32 connection
#if !defined(SPIWIFI_SS) // if the wifi definition isnt in the board variant
// Don't change the names of these #define's! they match the variant ones
#define SPIWIFI SPI
#define SPIWIFI_SS 10 // Chip select pin
#define NINA_ACK 9    // a.k.a BUSY or READY pin
#define NINA_RESETN 6 // Reset pin
#define NINA_GPIO0 -1 // Not connected
#endif
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS, SPIWIFI_SS,
                   NINA_ACK, NINA_RESETN, NINA_GPIO0, &SPIWIFI);
#else
AdafruitIO_WiFi io(mqtt_usernameAdafruitIO, mqtt_keyAdafruitIO, WIFI_SSID, WIFI_PASS);
#endif

AdafruitIO_Feed *presenca = io.feed("presenca");
AdafruitIO_Feed *vibracao = io.feed("vibracao");


//Pinos do projeto
int tringPin = 13;
int echoPin = 12;
int pinoVibracao = 34;


//Variável do som
#define SOUND_SPEED 0.034

//Variáveis do projeto
long duracaoPulso;
float distanciaCm;


//conectando o MQTT
void connectMQTT() {
  unsigned long tempoInicial = millis();
  while   (!mqtt_client.connected() && (millis() - tempoInicial < mqtt_timeout)) {
    if (WiFi.status() != WL_CONNECTED) {
      conecteWiFi();
    }
    Serial.print("Conectando ao MQTT Broker..");

    if (mqtt_client.connect("ESP32Client", mqtt_usernameAdafruitIO, mqtt_keyAdafruitIO)) {
      Serial.println();
      Serial.print("Conectado ao broker MQTT!\n");
    } else {
      Serial.println();
      Serial.print("Conexão com o broker MQTT falhou!");
      delay(1000);
    }
  }
  Serial.println();
}


//conectando o WiFi
void conecteWiFi(){
  WiFi.mode(WIFI_STA); //"station mode": permite o ESP32 ser um cliente da rede WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando à rede WiFi .. ");

  unsigned long startTime = millis();
  
  while(WiFi.status() != WL_CONNECTED && (millis() - startTime < wifi_timeout)){
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Falhou!");
  }else{
    Serial.print("Conectado com o IP: ");
    Serial.println(WiFi.localIP());
  }
}


//configurando setup
void setup() {
  Serial.begin(115200);
  pinMode(tringPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinoVibracao, INPUT);
  conecteWiFi();
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Conectando ao broker MQTT...\n");
    mqtt_client.setServer(mqtt_broker, mqtt_port);
  }
}


void loop() {
  if (!mqtt_client.connected()){
    connectMQTT();
  }

  //Recebendo o valor do sensor
  int vibracao_valor;
  vibracao_valor = digitalRead(pinoVibracao);
  
  //Envio de sinal de tring
  digitalWrite(tringPin, LOW);
  delayMicroseconds(2);
  //Seta o pino Tring alto por 10ms
  digitalWrite(tringPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(tringPin, LOW);

  Serial.println("A vibração é: " + String(vibracao_valor));

  //Calcular o tempo que o pino fica alto em MS
  duracaoPulso = pulseIn(echoPin, HIGH);

  //Calcula a distancia
  distanciaCm = (duracaoPulso * SOUND_SPEED) / 2;
  Serial.print("A distância é:" + String(distanciaCm));
  Serial.println("\n");

  if (mqtt_client.connected()){
    mqtt_client.loop();
    mqtt_client.publish("matheusvidal/feeds/presenca", String(distanciaCm).c_str(), true);
    mqtt_client.publish("matheusvidal/feeds/vibracao", String(vibracao_valor).c_str(), true + "\n");
    Serial.println("Publicou o dado da vibração: " + String(vibracao_valor));
    Serial.println("Publicou o dado da presença: " + String(distanciaCm) + "\n");
    delay(2700);
  }

  delay(1000);
}
