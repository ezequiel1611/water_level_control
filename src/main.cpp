#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP085.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <SimpleKalmanFilter.h>
#include <DNSServer.h>

// CONSTANTES
#define KInput 0.201  //Constante del Caudalimetro de entrada
#define KOutput 0.218 //Constante del Caudalimetro de salida
#define SOUND_VELOCITY 331 // m/s Velocidad del sonido a T amb
#define Kp 35.615 //Constante Proporcional
#define Ki 4.58   //Constante Integral
#define HEIGHT 54.10  //Distancia al fondo del tanque
#define STDEV 0.35 // Desviación Estandar del HC-SR04

// FILTRO DE KALMAN
SimpleKalmanFilter kalmanFilter(STDEV, 1, 0.05);

// PINES
const byte FlowmeterIn = 14, // (D5) Sensor de Entrada al Tanque
    FlowmeterOut = 12,       // (D6) Sensor de Salida del Tanque
    LedOn = 13,              // (D7) LED Indicador de Encendido
    WaterPump = 15,          // (D8) PWM Bomba de Agua
    Adjust = A0,             // ADC  Setear Nivel del Agua
    Trigger = 0,             // (D3) Emisor del HC-SR04
    Echo = 2;                // (D4) Receptor del HC-SR04
Adafruit_BMP085 bmp;         // D1=SCL D2=SDA Sensor de Temperatura

// VARIABLES
double QIn = 0, QOut = 0;               // Caudales medidos
volatile int CountIn = 0, CountOut = 0; // Contadores de pulsos
unsigned long TimeRef = 0, PreviousTime = 0, CurrentTime = 0, Ts = 0;
unsigned long lastTime = 0;
float SoundVel, Level;

// WEBSOCKET
const char* ssid = "ControlDeNivel";
const char* password = "patronato1914";
IPAddress ip(192,168,1,200);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);  
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar readings;
String message = "";

// SERVER DNS
const byte DNS_PORT = 53;
DNSServer dns_server;

// PI CONTROLLER
int setpoint = 0;
int PWMset = 0, PWM_prev = 0;
volatile int flag = 0;
float error = 0, error_prev = 0, integral = 0;

// put function declarations here:
void SetPins();
void IRAM_ATTR FlowIn();
void IRAM_ATTR FlowOut();
long UltrasonicSensor(byte TPin, byte EPin);
float SetSoundVelocity();
float getLevelDistance();
String getSensorReadings();
void initFS();
void initWiFi();
void notifyClients(String sensorReadings);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();
float getLevelDistanceFiltered();
String cleanMessage(String rawMessage);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  SetPins();
  // Seteo el rango del PWM a 0-1023 y la frecuencia a 1KHz
  analogWriteRange(1023);
  analogWriteFreq(1000);
  analogWrite(WaterPump, 0);
  // Configuro los pines de interrupción para los caudalímetros
  attachInterrupt(digitalPinToInterrupt(FlowmeterIn), FlowIn, RISING);
  attachInterrupt(digitalPinToInterrupt(FlowmeterOut), FlowOut, RISING);
  // Inicializo la comunicación SPI con el BMP180
  if (!bmp.begin()){
    Serial.println("Fallo en la comunicacion.");
    while (1){
      ESP.deepSleep(0);
    }
  }
  // Mido la velocidad del sonido
  SoundVel = SetSoundVelocity();
  // Inicializo el websocket
  initWiFi();
  initFS();
  initWebSocket();
  server.on("/",HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/",LittleFS,"/");
  server.begin();
  //
  dns_server.start(DNS_PORT, "*", WiFi.softAPIP());
  //
  digitalWrite(LedOn, HIGH);
  PreviousTime = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  QIn = (CountIn * KInput);
  QOut = (CountOut * KOutput);
  if((millis()-lastTime)>1000){
    String sensorReadings = getSensorReadings();
    Serial.println(sensorReadings);
    notifyClients(sensorReadings);
    CountIn = 0;
    CountOut = 0;
    Level = getLevelDistance();
    if(Level < 15){Level = Level + 1.0;}
    else if(Level < 20 && Level > 15){Level = Level + 0.4;}
    else if(Level < 30 && Level > 20){Level = Level + 0.8;}
    else if(Level < 40 && Level > 30){Level = Level + 1.2;}
    lastTime = millis();
  }
  ws.cleanupClients();
  // Calcular el periodo de muestreo para el PI
  CurrentTime = millis();
  Ts = (CurrentTime - PreviousTime) / 1000.0; // Convertir a segundos
  PreviousTime = CurrentTime;
  // Calculo el PWM con el controlador PI
  error = setpoint - Level;
  integral = error + error_prev;
  PWMset = (error * Kp) + PWM_prev + (Ki * Ts * 0.5 * integral);
  if (PWMset >= 1023){
    PWMset = 1023;
  }
  if (PWMset <= 0){
    PWMset = 0;
  }
  analogWrite(WaterPump, (flag * PWMset));
  error_prev = error;
  PWM_prev = PWMset;
  //
  dns_server.processNextRequest();
  //
}

// put function definitions here:
void IRAM_ATTR FlowIn() {
  CountIn++;
}

void IRAM_ATTR FlowOut() {
  CountOut++;
}

long UltrasonicSensor(byte TPin, byte EPin) {
  long Response;
  // Asegura el 0 en el trigger
  digitalWrite(TPin, LOW);
  delayMicroseconds(2);
  // Set trigger en 1 por 10us
  digitalWrite(TPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TPin, LOW);
  Response = pulseIn(EPin, HIGH);
  return Response;
}

float SetSoundVelocity() {
  float Tc, SV;
  Tc = bmp.readTemperature();
  SV = SOUND_VELOCITY * sqrt(1.0 + (Tc / 273.0));
  SV = SV / 10000.0;
  return SV;
}

float getLevelDistance() {
  long Duration = UltrasonicSensor(Trigger, Echo);
  float LevelDistance = HEIGHT - (Duration * SoundVel) / 2.0;
  return LevelDistance;
}

float getLevelDistanceFiltered() {
  float filteredDistance = kalmanFilter.updateEstimate(Level);
  return filteredDistance;
}

void SetPins() {
  pinMode(FlowmeterIn, INPUT);
  pinMode(FlowmeterOut, INPUT);
  pinMode(LedOn, OUTPUT);
  pinMode(WaterPump, OUTPUT);
  pinMode(Adjust, INPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
}

String getSensorReadings(){
  float level_filtered = getLevelDistanceFiltered();
  readings["nivel"] = String(level_filtered);
  readings["qin"] = String(QIn);
  readings["qout"] = String(QOut);
  readings["pwm"] = String(PWMset);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initFS(){
  if(!LittleFS.begin()){
    Serial.println("Error montando LittleFS.");
  }
  else{
    Serial.println("LittleFS se montó correctamente.");
  }
}

void initWiFi(){
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(ip, gateway, subnet);
  Serial.print("Iniciando AP:\t");
  Serial.println(ssid);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());
}

void notifyClients(String sensorReadings){
  ws.textAll(sensorReadings);
}

String cleanMessage(String rawMessage) {
    if (rawMessage.length() > 2){
      return rawMessage.substring(0,3);
    }
    else {
      return rawMessage;
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len){
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
    String rawMessage = String((char*)data);
    Serial.print("Raw message received: ");
    Serial.println(rawMessage);
       
    message = cleanMessage(rawMessage);

    Serial.print("Cleaned message: ");
    Serial.println(message);

    if(message.equals("sto")){
      flag = 0;
      digitalWrite(LedOn, LOW);
      Serial.println("Stop");
    }
    else if(message.equals("sta")){
      flag = 1;
      digitalWrite(LedOn, HIGH);
      Serial.println("Start");
    }
    else if(message.equals("get")){
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    }
    else{
      setpoint = message.toInt();
      Serial.print("New setpoint: ");
      Serial.println(setpoint);
    }
  }
  else{
    Serial.println("WebSocket message is not valid");
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket(){
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}