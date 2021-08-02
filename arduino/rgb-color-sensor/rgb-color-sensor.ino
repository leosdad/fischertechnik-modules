/****************************************************************
ColorSensor.ino
APDS-9960 RGB and Gesture Sensor
Shawn Hymel @ SparkFun Electronics
October 15, 2014
https://github.com/sparkfun/APDS-9960_RGB_and_Gesture_Sensor

Conexões

Importante: O módulo APDS-9960 funciona somente com 3.3V!!!
 
Arduino Pin  APDS-9960 Board  Function
 
 3.3V            VCC            Power
 GND             GND            Ground
 A4              SDA            I2C Data
 A5              SCL            I2C Clock
 
******************************************************************/

//Inclusão de bibliotecas
#include <Wire.h>
#include <SparkFun_APDS9960.h>

//Instanciando o objeto "apds" e atribuindo como valor uma função existente na biblioteca
SparkFun_APDS9960 apds = SparkFun_APDS9960();

//Variáveis globais
uint16_t ambient_light = 0;
uint16_t red_light = 0;
uint16_t green_light = 0;
uint16_t blue_light = 0;

void setup() {
  
  //Inicializando a comunicação serial
  Serial.begin(9600);
  Serial.println();
  Serial.println(F("--------------------------------"));
  Serial.println(F("SparkFun APDS-9960 - Sensor de Cor"));
  Serial.println(F("--------------------------------"));
  
  //Inicializa o módulo APDS-9960
  if ( apds.init() ) {
    Serial.println(F("APDS-9960: Inicialização completa!"));
  } else {
    Serial.println(F("APDS-9960: Algo deu errado durante a inicialização!"));
  }
  
  //Executa o sensor de luminosidade
  if ( apds.enableLightSensor(false) ) {
    Serial.println(F("O sensor de luz está funcionando!"));
  } else {
    Serial.println(F("Algo deu errado durante a unidade do sensor de luz!"));
  }
  
  // Aguarde a inicialização e calibração terminar
  delay(500);
}

void loop() {
  
  // Lê os níveis de luminosidade (ambiente, vermelho, verde, azul)
  if (  !apds.readAmbientLight(ambient_light) ||
        !apds.readRedLight(red_light) ||
        !apds.readGreenLight(green_light) ||
        !apds.readBlueLight(blue_light) ) {
    Serial.println("Erro ao ler os valores de luminosidade");
  } else {
    Serial.print("R: ");
    Serial.print(red_light);
    Serial.print(" G: ");
    Serial.print(green_light);
    Serial.print(" B: ");
    Serial.print(blue_light);
    Serial.print(" C: ");
    Serial.println(ambient_light);
  }
  
  // Espera 1 segundo e executa a próxima leitura
  delay(300);
}
