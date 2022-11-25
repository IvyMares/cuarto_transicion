#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DHT_U.h>

LiquidCrystal_I2C lcd(0x27,16,2);   //crear el objeto con los pines que se van a usarxxxxxxxxxx SDA A4, SCL A5

// Declaracion de pines E/S todo ok
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
const int motoreductor = 12;

// Array de entradas
const int entradas[5] = { 2, 3, 5, 6, 11};
//const int salidas [4] = { 4, 7, 8, 12};

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
  pinMode(motoreductor, OUTPUT); // pin 12

  lcd.init(); //Inicializar pantalla lcd conectando SDA A4, SCL A5
  lcd.backlight();
  
  digitalWrite(imanEntrada, HIGH); // Al arrancar el codigo el iman de la entrada estara activado siempre
}

void loop() {
  //La humedad se esta leyendo siempre
  Humd = dht.readHumidity();
  Serial.println("Hum:"+ String(Humd,1));
  lcd.setCursor(0, 0); //COLOCAR EN POSICION
  lcd.print("Hum:"+ String(Humd,1) + "% ");
  
  for(int i = 0; i<=4; i++){
    Serial.print("Leeo Entrada:");
    Serial.println(entradas[i]);
    leerEntradas(entradas[i]);
    delay(500);
  }
  //delay(1000);
}

void timer(int pin){
  digitalWrite(pin, LOW);
  int timer = 1000;
  digitalWrite(pin, HIGH);
  while(timer > 0 && banderaParoEmergencia == false){
    timer--;
    banderaParoEmergencia = digitalRead(paroEmergencia) == HIGH ? true : false;  
    Serial.println(timer);
    }
  digitalWrite(pin, LOW);
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
    lcd.print("distancia: "+ String(distancia) + " cm");
    if(distancia <= 10 && distancia >= 0){
      dispatch(11);
    }
}

void dispatch(int pin){
  switch(pin){
    case 2:
        banderaParoEmergencia = true;
        Serial.println("Paro Emergencia");
      break;
    case 3:
        // puerta entrada paroemergencia false imanentrada LOW puertaentrada true
        Serial.println("Puerta Entrada");
        banderaPuertaEntrada = true;
        digitalWrite(imanEntrada, LOW); // Apagamos los imanes
        banderaParoEmergencia = false; // seteamos paro de emergencia a false
      break;
    case 5:
        // sensor infrarrojo imanentrada hight infrarrojo true if paroemergencia false si paro de emergencia true salidas en low y deshabilita los imanes
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaRociador == false){ //&& banderaInfrarrojoEntrada == false
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            banderaInfrarrojoEntrada = true;
          }
        }else{
          digitalWrite(imanEntrada, LOW);
          banderaPuertaEntrada = false;
          banderaInfrarrojoEntrada = false;
          banderaRociador = false;
        }
      break;
    case 6: 
        // desinfeccion solo si infrarrojo es true imanentrada hight led desinfeccion hihg, rociador high si y solo si paro de emergencia low si paro de emergencia true salidas en low y deshabilita los imanes
        Serial.println("Boton Desinfeccion");
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaInfrarrojoEntrada == true && banderaRociador == false && Humd<humedadIdeal)
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            digitalWrite(ledDesinfeccion, HIGH);
            timer(rociador);
            digitalWrite(ledDesinfeccion, LOW);
            banderaRociador = true;
        }else{
          digitalWrite(imanEntrada, LOW);
          banderaPuertaEntrada = false;
          banderaInfrarrojoEntrada = false;
          banderaRociador = false;
        }
      break;
    case 11:
        // trig pin si desinfeccion en true imanentrada high rociador low piston hihg  si y solo si paro de emergencia low si paro de emergencia true salidas en low y deshabilita los imanes
        Serial.println("Abrir puerta final");
        if(banderaParoEmergencia == false){
          if(banderaPuertaEntrada == true && banderaInfrarrojoEntrada == true && banderaRociador == true && Humd>=humedadIdeal){
            digitalWrite(imanEntrada, HIGH); // Encendemos los imanes
            timer(motoreductor);
          }        
        }else{
          digitalWrite(imanEntrada, LOW);
          banderaPuertaEntrada = false;
          banderaInfrarrojoEntrada = false;
          banderaRociador = false;
        }
      break;
  }
}

