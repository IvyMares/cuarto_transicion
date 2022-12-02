#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DHT_U.h>

LiquidCrystal_I2C lcd(0x27,16,2);   //crear el objeto con los pines que se van a usarxxxxxxxxxx SDA A4, SCL A5

// Declaracion de pines E/S
const int paroEmergencia = 2;
const int puertaEntrada = 3;
const int imanEntrada = 4;
const int sensorInfrarrojo = 5; // const int botonYaCerre = 5; 
const int botonDeDesinfeccion = 6;
const int ledDesinfeccion = 7;
const int rociador = 8;
DHT dht(9, DHT11);
const int trigPin = 10; //sensor de proximidad
const int echoPin = 11; //sensor de proximidad
const int piston = 12;
const int ledAdentro = 13;

// Array de entradas
const int entradas[5] = { 2, 3, 5, 6, 11 };
// const int salidas [5] = { 4, 7, 8, 12, 13 };

long duracion;
int distancia;
int ultimaDistancia = 0;
int humedadIdeal=5; //Es el valor que el sensor de humedad tiene que dar para que se procesa al cuarto limpio
float Humd = 0;

//Banderas estados estado inicial en false
bool banderaParoEmergencia = false;
bool banderaPuertaEntrada = false;
bool banderaInfrarrojoEntrada = false;
bool banderaRociador = false;


// Funciones 
void timer(int);
void leerEntradas(int);
void dispatch(int);
void estadoInicial();

void setup() {
  Serial.begin(9600);
  pinMode(paroEmergencia, INPUT); // pin 2
  pinMode(puertaEntrada, INPUT); // pin 3
  pinMode(imanEntrada, OUTPUT); // pin 4
  pinMode(sensorInfrarrojo, INPUT); // pin 5
  pinMode(botonDeDesinfeccion, INPUT); // pin 6
  pinMode(ledDesinfeccion, OUTPUT); // pin 7
  pinMode(rociador, OUTPUT); // pin 8
  dht.begin(); // Inicializar dht pin 9
  pinMode(trigPin, OUTPUT); // pin 10
  pinMode(echoPin, INPUT); // pin 11
  pinMode(piston, OUTPUT); // pin 12
  pinMode(ledAdentro, OUTPUT); // pin 13

  lcd.init(); //Inicializar pantalla lcd conectando SDA A4, SCL A5
  lcd.backlight();
  
  estadoInicial(); // Al arrancar el codigo seteamos estado inicial
}

void loop() {
  //La humedad se esta leyendo siempre
  Humd = isnan(dht.readHumidity()) ? 0 : dht.readHumidity() ;
  Serial.println("Humidity:"+ String(Humd,1));
  lcd.setCursor(0, 0); //COLOCAR EN POSICION
  lcd.print("Humidity:"+ String(Humd,1) + "% ");
  
  for(int i = 0; i<=4; i++){
    Serial.print("Leo Entrada:");
    Serial.println(entradas[i]);
    leerEntradas(entradas[i]);
    delay(500);
  }
}

void timer(int pin){
  int timer = 1000;
  digitalWrite(pin, HIGH);
  if(pin != 12){
    while(timer > 0 && banderaParoEmergencia == false){
      timer--;
      banderaParoEmergencia = digitalRead(paroEmergencia) == HIGH ? true : false;  
      Serial.println(timer);
      }
    digitalWrite(pin, LOW);
  }else{
    while(timer > 0){
        timer--; 
        Serial.println(timer);
      }
    digitalWrite(pin, LOW);
  }
}

void leerEntradas (int pin){
  if(pin != 5){
    if(digitalRead(pin) == HIGH){
        delay(500);
        Serial.print("Entrada del pin:");
        Serial.println(pin);
        dispatch(pin);
      }
    }else{
      if(digitalRead(pin) == LOW){
          delay(500);
          Serial.print("Entrada del pin:");
          Serial.println(pin);
          dispatch(pin);
        }
    }
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duracion = pulseIn(echoPin, HIGH);
    distancia = (duracion * 0.034) / 2;
    lcd.setCursor(0, 1); //COLOCAR EN POSICION
    lcd.print("Distancia: "+ String(distancia) + " cm");
    if(distancia <= 10 && distancia >= 0){
      dispatch(11);
    }
}

void dispatch(int pin){
  switch(pin){
    case 2:
        Serial.println("Paro Emergencia");
        estadoInicial();
        banderaParoEmergencia = true;
        digitalWrite(imanEntrada, LOW);
      break;
    case 3:
        // puerta entrada paroemergencia false imanentrada LOW puertaentrada true
        Serial.println("Puerta Entrada");
        digitalWrite(imanEntrada, LOW); // Apagamos los imanes
        banderaPuertaEntrada = true;
        banderaParoEmergencia = false; // seteamos paro de emergencia a false
      break;
    case 5:
        // sensor infrarrojo imanentrada hight infrarrojo true if paroemergencia false si paro de emergencia true salidas en low y deshabilita los imanes
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaInfrarrojoEntrada == false && banderaRociador == false){
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            digitalWrite(ledAdentro, HIGH); // Led indicando que hay alguien adentro
            banderaInfrarrojoEntrada = true;
          }
        }else{
          estadoInicial();
          digitalWrite(imanEntrada, LOW);
        }
      break;
    case 6: 
        // desinfeccion solo si infrarrojo es true imanentrada hight led desinfeccion hihg, rociador high si y solo si paro de emergencia low si paro de emergencia true salidas en low y deshabilita los imanes
        Serial.println("Boton Desinfeccion");
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaInfrarrojoEntrada == true && (Humd < humedadIdeal || banderaRociador == false))
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            digitalWrite(ledDesinfeccion, HIGH);
            timer(rociador);
            digitalWrite(ledDesinfeccion, LOW);
            banderaRociador = true;
        }else{
          estadoInicial();
          digitalWrite(imanEntrada, LOW);
        }
      break;
    case 11:
        // trig pin si desinfeccion en true imanentrada high rociador low piston hihg  si y solo si paro de emergencia low si paro de emergencia true salidas en low y deshabilita los imanes
        Serial.println("Abrir puerta final");
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaInfrarrojoEntrada == true && banderaRociador == true && Humd>=humedadIdeal){
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            timer(piston);
            estadoInicial();
          }        
        }else{
          estadoInicial();
          digitalWrite(imanEntrada, LOW);
        }
      break;
  }
}

void estadoInicial(){
  digitalWrite(paroEmergencia, LOW); //pin 2
  digitalWrite(puertaEntrada, LOW); //pin 3
  digitalWrite(imanEntrada, HIGH); //pin 4
  digitalWrite(sensorInfrarrojo, HIGH); //pin 5
  digitalWrite(botonDeDesinfeccion, LOW); //pin 6
  digitalWrite(ledDesinfeccion, LOW); //pin 7
  digitalWrite(rociador, LOW); //pin 8
  digitalWrite(9, LOW); //pin 9
  digitalWrite(trigPin, LOW); //pin 10
  digitalWrite(echoPin, LOW); //pin 11
  digitalWrite(piston, HIGH); //pin 12
  digitalWrite(ledAdentro, LOW); //pin 13
               
  banderaParoEmergencia = false;
  banderaPuertaEntrada = false;
  banderaInfrarrojoEntrada = false;
  banderaRociador = false;
}
