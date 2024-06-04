#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP085.h>

// CONSTANTES
#define KInput 0.210
#define KOutput 0.218
#define SOUND_VELOCITY 331 // m/s

// PINES
const byte FlowmeterIn = 14,  // (D5) Sensor de entrada al tanque
           FlowmeterOut = 12, // (D6) Sensor de salida del tanque
           StandBy = 13,      // (D7) Enable TB6612FNG
           WaterPump = 15,    // (D8) Bomba de Agua
           Adjust = A0,       // ADC
           Trigger = 0,       // (D3) Emisor del HC-SR04
           Echo = 2;          // (D4) Receptor del HC-SR04
Adafruit_BMP085 bmp;          // D1=SCL D2=SDA Sensor de Temperatura

// VARIABLES
int PWMset = 0;
double QIn = 0, QOut = 0; // Caudales medidos
volatile int CountIn = 0, CountOut = 0; //Contadores de pulsos
unsigned long TimeRef = 0; 
long Duration;
float Distance,Height,Level,SoundVel;

// put function declarations here:
void SetPins();
void IRAM_ATTR FlowIn();
void IRAM_ATTR FlowOut();
long UltrasonicSensor(byte TPin, byte EPin);
float SetSoundVelocity();
float SetZero(float SoundVelocity);
void SendData(double x1, double x2, float x3, int x4);

void setup() {
  // put your setup code here, to run once:
  SetPins();
  analogWriteRange(1023);
  analogWriteFreq(1000);
  attachInterrupt(digitalPinToInterrupt(FlowmeterIn),FlowIn,RISING);
  attachInterrupt(digitalPinToInterrupt(FlowmeterOut),FlowOut,RISING);
  Serial.begin(115200);
  if(!bmp.begin()) {
    Serial.println("Fallo en la comunicacion.");
    while(1){}
  }
  SoundVel = SetSoundVelocity();
  Height = SetZero(SoundVel);
  Serial.print("Distancia al Fondo:");
  Serial.print(Height);
  Serial.println("[cm].");
  delay(5000);
  digitalWrite(StandBy, HIGH);
  analogWrite(WaterPump,PWMset);
}

void loop() {
  // put your main code here, to run repeatedly:
  CountIn = 0;
  CountOut = 0;
  TimeRef = millis();
  interrupts();
  while((millis() - TimeRef) < 1000){
    QIn = (CountIn * KInput);
    QOut = (CountOut * KOutput);
    PWMset = analogRead(Adjust);
    analogWrite(WaterPump, PWMset);
  }
  noInterrupts();
  Duration = UltrasonicSensor(Trigger, Echo);
  Distance = Duration * SoundVel/2;
  Level = Height - Distance;
  SendData(QIn,QOut,Level,PWMset);
}

// put function definitions here:
void IRAM_ATTR FlowIn() {
  CountIn++;
}

void IRAM_ATTR FlowOut() { 
  CountOut++;
}

long UltrasonicSensor(byte TPin, byte EPin){
  long Response;
  //Asegura el 0 en el trigger
  digitalWrite(TPin, LOW);
  delayMicroseconds(2);
  //Set trigger en 1 por 10us
  digitalWrite(TPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(TPin,LOW);
  Response = pulseIn(EPin,HIGH);
  return Response;
}

float SetSoundVelocity(){
  float Tc,SV;
  Tc = bmp.readTemperature();
  SV = SOUND_VELOCITY * sqrt(1.0+(Tc/273.0));
  SV = SV/10000.0;
  return SV;
}

float SetZero(float SoundVelocity){
  long TimeMeasured = 0;
  float DistanceMeasured = 0.0,Zero = 0.0;
  delay(1000);
  for (int i = 0; i < 10; i++)
  {
    TimeMeasured = UltrasonicSensor(Trigger,Echo);
    DistanceMeasured = TimeMeasured * SoundVelocity/2;
    Zero += DistanceMeasured;
    delay(1000);
  }
  Zero = Zero / 10.00;
  return Zero;
}

void SendData(double x1, double x2, float x3, int x4) {
  Serial.print("QIn=");
  Serial.print(x1);
  Serial.print("[ml/s]  Qout=");
  Serial.print(x2);
  Serial.print("[ml/s]  H=");
  Serial.print(x3);
  Serial.print("[cm]  PWM=");
  Serial.println(x4);
}

void SetPins() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FlowmeterIn, INPUT);
  pinMode(FlowmeterOut, INPUT);
  pinMode(StandBy, OUTPUT);
  pinMode(WaterPump, OUTPUT);
  pinMode(Adjust,INPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
}