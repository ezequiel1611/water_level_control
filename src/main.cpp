#include <Arduino.h>

// CONSTANTES SENSORES
#define KInput 0.201
#define KOutput 0.218

// PINES
const byte FlowmeterIn = 14,  // Sensor de entrada al tanque
           FlowmeterOut = 12, // Sensor de salida del tanque
           StandBy = 5,       // Enable TB6612FNG
           WaterPump = 4,     // Bomba de Agua
           Adjust = A0;       // ADC

// VARIABLES
int PWMset = 0;
double QIn = 0, QOut = 0; // Caudales medidos
volatile int CountIn = 0, CountOut = 0; //Contadores de pulsos
unsigned long TimeRef = 0; 

// put function declarations here:
void FlowIn();
void FlowOut();
void SetPins();
void SendData(double x1, double x2);

void setup() {
  // put your setup code here, to run once:
  SetPins();
  digitalWrite(StandBy,LOW);
  analogWriteRange(1023);
  analogWriteFreq(1000);
  attachInterrupt(digitalPinToInterrupt(FlowmeterIn),FlowIn,RISING);
  attachInterrupt(digitalPinToInterrupt(FlowmeterOut),FlowOut,RISING);
  Serial.begin(9600);
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
  if(abs(QIn-QOut) < (0.05*QOut)){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else{
    digitalWrite(LED_BUILTIN,LOW);
  }
  SendData(QIn,QOut);
}

// put function definitions here:
void FlowIn() {
  CountIn++;
}

void FlowOut() { 
  CountOut++;
}

void SendData(double x1, double x2) {
  Serial.print("QIn=");
  Serial.print(x1);
  Serial.print("ml/s");
  Serial.print("  QOut=");
  Serial.print(x2);
  Serial1.println("ml/s");
}

void SetPins() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FlowmeterIn, INPUT);
  pinMode(FlowmeterOut, INPUT);
  pinMode(StandBy, OUTPUT);
  pinMode(WaterPump, OUTPUT);
  pinMode(Adjust,INPUT);
}