#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP085.h>

// CONSTANTES
#define KInput 0.201
#define KOutput 0.218
#define SOUND_VELOCITY 331 // m/s
#define Kp 35.615
#define Ki 4.58
#define HEIGHT 54.58

// PINES
const byte FlowmeterIn = 14, // (D5) Sensor de entrada al tanque
    FlowmeterOut = 12,       // (D6) Sensor de salida del tanque
    StandBy = 13,            // (D7) Enable TB6612FNG
    WaterPump = 15,          // (D8) Bomba de Agua
    Adjust = A0,             // ADC
    Trigger = 0,             // (D3) Emisor del HC-SR04
    Echo = 2;                // (D4) Receptor del HC-SR04
Adafruit_BMP085 bmp;         // D1=SCL D2=SDA Sensor de Temperatura

// VARIABLES
double QIn = 0, QOut = 0;               // Caudales medidos
volatile int CountIn = 0, CountOut = 0; // Contadores de pulsos
unsigned long TimeRef = 0, PreviousTime = 0, CurrentTime = 0, Ts = 0;
long Duration;
const int NumReadings = 10;
float Distance, SoundVel, LevelAVG, LevelTotal, Readings[10]; // Height;
int Index = 0;
String command = "nothing";
// PI CONTROLLER
int setpoint = 15;
int PWMset = 0, PWM_prev = 0;
float error = 0, error_prev = 0, integral = 0;

// put function declarations here:
void SetPins();
void IRAM_ATTR FlowIn();
void IRAM_ATTR FlowOut();
long UltrasonicSensor(byte TPin, byte EPin);
float SetSoundVelocity();
void SendData(double x1, double x2, float x3, int x4, int x5);

void setup()
{
  // put your setup code here, to run once:
  delay(500);
  Serial.begin(115200);
  SetPins();
  // Inicializo el vector de lecturas en cero
  for (int i = 0; i < NumReadings; i++)
  {
    Readings[i] = 0;
  }
  // Seteo el rango del PWM a 0-1023 y la frecuencia a 1KHz
  analogWriteRange(1023);
  analogWriteFreq(1000);
  // Configuro los pines de interrupción para los caudalímetros
  attachInterrupt(digitalPinToInterrupt(FlowmeterIn), FlowIn, RISING);
  attachInterrupt(digitalPinToInterrupt(FlowmeterOut), FlowOut, RISING);
  // Inicializo la comunicación SPI con el BMP180
  if (!bmp.begin())
  {
    Serial.println("Fallo en la comunicacion.");
    while (1)
    {
    }
  }
  SoundVel = SetSoundVelocity();
  // Leo el potenciometro seteando un primer setpoint
  setpoint = analogRead(Adjust);
  setpoint = map(setpoint, 0, 1023, 10, 40);
  Serial.println(setpoint);
  // Se espera el comando de inicio
  Serial.println("Esperando...");
  do
  {
    command = Serial.readStringUntil('\n');
  } while (command != "start");
  digitalWrite(StandBy, HIGH);
  PreviousTime = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  CountIn = 0;
  CountOut = 0;
  TimeRef = millis();
  // Mido el Caudal
  interrupts();
  while ((millis() - TimeRef) < 1000)
  {
    QIn = (CountIn * KInput);
    QOut = (CountOut * KOutput);
  }
  noInterrupts();
  // Obtención del Nivel Actual
  Duration = UltrasonicSensor(Trigger, Echo);
  Distance = Duration * SoundVel / 2;
  if (Distance >= 15)
  {
    /*LevelTotal = LevelTotal - Readings[Index];
    Readings[Index] = HEIGHT - Distance;
    LevelTotal = LevelTotal + Readings[Index];
    Index++;
    if (Index >= NumReadings)
    {
      Index = 0;
    }
    LevelAVG = LevelTotal / NumReadings;*/
    LevelAVG = HEIGHT - Distance;
  }
  // Leo el potenciometro para saber el setpoint del nivel
  setpoint = analogRead(Adjust);
  setpoint = map(setpoint, 0, 1023, 10, 40);
  // Calcular el periodo de muestreo
  CurrentTime = millis();
  Ts = (CurrentTime - PreviousTime) / 1000.0; // Convertir a segundos
  PreviousTime = CurrentTime;
  // Calculo el PWM con el controlador PI
  error = setpoint - LevelAVG;
  integral = error + error_prev;
  PWMset = (error * Kp) + PWM_prev + (Ki * Ts * 0.5 * integral);
  if (PWMset >= 1023)
  {
    PWMset = 1023;
  }
  if (PWMset <= 0)
  {
    PWMset = 0;
  }
  analogWrite(WaterPump, PWMset);
  error_prev = error;
  PWM_prev = PWMset;
  // Envio los datos por puerto serie
  SendData(QIn, QOut, LevelAVG, PWMset, setpoint);
}

// put function definitions here:
void IRAM_ATTR FlowIn()
{
  CountIn++;
}

void IRAM_ATTR FlowOut()
{
  CountOut++;
}

long UltrasonicSensor(byte TPin, byte EPin)
{
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

float SetSoundVelocity()
{
  float Tc, SV;
  Tc = bmp.readTemperature();
  SV = SOUND_VELOCITY * sqrt(1.0 + (Tc / 273.0));
  SV = SV / 10000.0;
  return SV;
}

void SendData(double x1, double x2, float x3, int x4, int x5)
{
  Serial.print("Qi=");
  Serial.print(x1);
  Serial.print("/Qo=");
  Serial.print(x2);
  Serial.print("/WL=");
  Serial.print(x3);
  Serial.print("/DC=");
  Serial.print(x4);
  Serial.print("/SP=");
  Serial.println(x5);
}

void SetPins()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FlowmeterIn, INPUT);
  pinMode(FlowmeterOut, INPUT);
  pinMode(StandBy, OUTPUT);
  pinMode(WaterPump, OUTPUT);
  pinMode(Adjust, INPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
}