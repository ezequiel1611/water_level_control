#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP085.h>

// CONSTANTES
#define KInput 0.201
#define KOutput 0.218
#define SOUND_VELOCITY 331 // m/s

// PINES
const byte FlowmeterIn = 14,  // Sensor de entrada al tanque
           FlowmeterOut = 12, // Sensor de salida del tanque
           StandBy = 13,      // Enable TB6612FNG
           WaterPump = 15,    // Bomba de Agua
           Adjust = A0,       // ADC
           Trigger = 0,       // Emisor del HC-SR04
           Echo = 2;          // Receptor del HC-SR04

// VARIABLES
int PWMset = 0;
double QIn = 0, QOut = 0; // Caudales medidos
volatile int CountIn = 0, CountOut = 0; //Contadores de pulsos
unsigned long TimeRef = 0; 
long Duration;
float Distance;
Adafruit_BMP085 bmp;
float SoundVel;

// put function declarations here:
void SetPins();
void FlowIn();
void FlowOut();
long UltrasonicSensor(byte TPin, byte EPin);
float SetSoundVelocity();
void SendData(double x1, double x2, float x3);

void setup() {
  // put your setup code here, to run once:
  SetPins();
  digitalWrite(StandBy,LOW);
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
}

void loop() {
  // put your main code here, to run repeatedly:
  TimeRef = millis();
  interrupts();
  CountIn = 0;
  CountOut = 0;
  while((millis() - TimeRef) < 1000){
    QIn = (CountIn * KInput);
    QOut = (CountOut * KOutput);
    PWMset = analogRead(Adjust);
    analogWrite(WaterPump, PWMset);
  }
  noInterrupts();
  Duration = UltrasonicSensor(Trigger, Echo);
  Distance = Duration * SoundVel/2;
  SendData(QIn,QOut,Distance);
}

// put function definitions here:
void FlowIn() {
  CountIn++;
}

void FlowOut() { 
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

void SendData(double x1, double x2, float x3) {
  Serial.print("QIn=");
  Serial.print(x1);
  Serial.print("ml/s");
  Serial.print("  QOut=");
  Serial.print(x2);
  Serial.print("ml/s");
  Serial.print("  H=");
  Serial.print(x3);
  Serial.println("cm");
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