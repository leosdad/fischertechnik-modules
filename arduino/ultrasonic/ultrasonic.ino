// -----------------------------------------------------------------------------
//
// Ultrasonic sensor
//
// Rubem Pechansky
// Based on: Arduino ultrasonic sensor HC-SR04 by Arbi Abdul Jabbaar
// https://create.arduino.cc/projecthub/abdularbi17/ultrasonic-sensor-hc-sr04-with-arduino-tutorial-327ff6
//
// -----------------------------------------------------------------------------

#include <Wire.h>

// -----------------------------------------------------------------------------

// Constants

#define BAUD_RATE		57600
#define SLAVE_ADDRESS	10
#define SOUND_K			(0.034 / 2) // Speed of sound wave divided by 2

// Arduino pins

#define echoPin 2	// Arduino pin D2 -- pin Echo
#define trigPin 3	// Arduino pin D3 -- pin Trig

// -----------------------------------------------------------------------------

// Variables

long duration;		// Duration of sound wave travel
int distance;		// Distance measurement

// -----------------------------------------------------------------------------

void setup()
{
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(requestEvent);

	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);

	// DEBUG: serial monitor
	// Serial.begin(BAUD_RATE);
}

// -----------------------------------------------------------------------------

void loop()
{
	// Clears the trigPin condition

	digitalWrite(trigPin, LOW);
	delayMicroseconds(2);

	// Sets the trigPin HIGH (active) for 10 microseconds

	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);

	// Reads the echoPin, returns the sound wave travel time in microseconds

	duration = pulseIn(echoPin, HIGH);

	// Calculates the distance

	distance = duration * SOUND_K;

	// DEBUG: displays the distance on the serial monitor

	// Serial.print("Distance: ");
	// Serial.print(distance);
	// Serial.print(" cm ");
	// Serial.println();
}

// -----------------------------------------------------------------------------

// Sends data requested by master

void requestEvent()
{
	byte data[2];

	data[0] = distance & 0xff;
	data[1] = (distance & 0xff00) >> 8;
	Wire.write(data, 2);
}

// -----------------------------------------------------------------------------
