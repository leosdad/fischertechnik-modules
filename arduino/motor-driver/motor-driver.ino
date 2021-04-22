// -----------------------------------------------------------------------------
//
// fischertechnik DC motor with encoder
//
// Rubem Pechansky
//
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>

// -----------------------------------------------------------------------------

// Constants

#define BAUD_RATE		57600
#define SLAVE_ADDRESS	8

// Macros

#ifndef uint
#define uint unsigned int
#endif

#define TWOCC(ch0, ch1) ((word)(byte)(ch0) | ((word)(byte)(ch1) << 8))

#define MOTOR_CCW	{ analogWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, speed[currentMotor]); analogWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, 0); }
#define MOTOR_CW	{ analogWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, 0); analogWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, speed[currentMotor]); }
#define MOTOR_IDLE	{ digitalWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, LOW); digitalWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, LOW); }
#define MOTOR_BRAKE	{ digitalWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, HIGH); digitalWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, HIGH); }

// Arduino pins

// https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/
// "The PWM outputs generated on pins 5 and 6 will have higher-than-expected duty cycles"

//					0		// D0 TX
//					1		// D1 RX
#define encoderA1	2		// D2 / INT0
#define motorBPin1	3		// D3 PWM / INT1
#define encoderA2	4		// D4
#define motorAPin1	5		// D5 PWM
#define motorAPin2	6		// D6 PWM
#define encoderB1	7		// D7
#define encoderB2	8		// D8
#define motorBPin2	9		// D9 PWM
//					10		// D10 PWM / SPI / SS
//					11		// D11 PWM / SPI / MOSI
//					12		// D12 / SPI / MISO
#define eventPin	13		// D13 / LED
#define limitA1Pin	14		// A0
#define limitA2Pin	15		// A1
#define limitB1Pin	16		// A2
#define limitB2Pin	17		// A3
//					18		// A4 / I2C SDA
//					19		// A5 / I2C SCL
//					20		// A6
//					21		// A7

// I2C commands

#define cmdBackwards	TWOCC('B', 'w')
#define cmdBrake		TWOCC('B', 'r')
#define cmdForward		TWOCC('F', 'w')
#define cmdGoal			TWOCC('G', 'o')
#define cmdHome			TWOCC('H', 'o')
#define cmdHello		TWOCC('H', 'i')
#define cmdIdle			TWOCC('I', 'd')
#define cmdMode			TWOCC('M', 'd')
#define cmdSetMotor		TWOCC('M', 't')
#define cmdSpeed		TWOCC('S', 'p')

// Motor states

enum {
	IDLE = 0,
	FORWARD,
	BACKWARDS,
	HOME,
	CCW,
	CW,
	GOHOME,
	BRAKE,
	RESET,
};

enum {
	MOTOR_A = 0,
	MOTOR_B,
};

enum {
	PULSES = 0,
	ENDLIMITSWITCH,
};

// Variables

bool direction[2] = {false, false};
byte currentMotor = MOTOR_A;
byte encoderLast[2] = {-1, -1};
byte state[2] = {IDLE, IDLE};
byte speed[2] = {255, 255};
byte mode[2] = {PULSES, PULSES};
uint pulses[2] = {0, 0};
uint targetPulses[2] = {0, 0};

// -----------------------------------------------------------------------------

void setup()
{
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(requestEvent);
	Wire.onReceive(receiveEvent);

	Serial.begin(BAUD_RATE);
	Serial.println("Arduino reset");

	pinMode(limitA1Pin, INPUT_PULLUP);
	pinMode(limitA2Pin, INPUT_PULLUP);
	pinMode(limitB1Pin, INPUT_PULLUP);
	pinMode(limitB2Pin, INPUT_PULLUP);

	pinMode(eventPin, OUTPUT);
	digitalWrite(eventPin, LOW);

	// pinMode(encoderA1, INPUT_PULLUP);	// Does nothing: probably 20kΩ is not enough
	// pinMode(encoder.... (not tested)

	initMotors();
	initEncoders();
	resetState();
}

// -----------------------------------------------------------------------------

void loop()
{
	switch(state[currentMotor]) {

		case HOME:

			if(!digitalRead(currentMotor == MOTOR_A ? limitA1Pin : limitB1Pin)) {
				MOTOR_BRAKE;
				resetState();
			} else {
				pulses[currentMotor] = 0;
				state[currentMotor] = GOHOME;
				MOTOR_CW;
				Serial.println("State = GOHOME");
			}
			break;

		case FORWARD:

			pulses[currentMotor] = 0;
			state[currentMotor] = CCW;
			MOTOR_CCW;
			Serial.println("State = CCW");
			break;

		case BACKWARDS:

			pulses[currentMotor] = 0;
			state[currentMotor] = CW;
			MOTOR_CW;
			Serial.println("State = CW");
			break;

		case GOHOME:

			if(!digitalRead(currentMotor == MOTOR_A ? limitA1Pin : limitB1Pin)) {
				// Stop motor
				MOTOR_BRAKE;
				sendSignal();
				pulses[currentMotor] = 0;
				state[currentMotor] = IDLE;
				Serial.println("State = IDLE");
			}
			break;

		case CW:
		case CCW:

			if(mode[currentMotor] == PULSES) {
				if(pulses[currentMotor] >= targetPulses[currentMotor]) {
					// Stop motor
					MOTOR_BRAKE;
					sendSignal();
					pulses[currentMotor] = 0;
					state[currentMotor] = IDLE;
					Serial.println("State = IDLE");
				// } else {
					// Serial.print(pulses[currentMotor]);
					// Serial.print(" / ");
					// Serial.println(speed[currentMotor]);
				}
			} else {
				if(!digitalRead(currentMotor == MOTOR_A ? limitA2Pin : limitB2Pin)) {
					MOTOR_BRAKE;
					sendSignal();
					state[currentMotor] = IDLE;
					Serial.println("State = IDLE");
				}
			}
			break;

	}
}

// -----------------------------------------------------------------------------

// Receives data from master

void receiveEvent(int nBytes)
{
	// Serial.print("Receiving ");
	// Serial.print(nBytes);
	// Serial.print(" bytes: ");

	char cmd[6];

	for(int count = 0; Wire.available(); count++) {
		cmd[count] = Wire.read();
	}

	// Received commands

	switch(((word)(byte)cmd[0] | ((word)(byte)cmd[1] << 8))) {

		case cmdHello:
			Serial.println("Hello yourself!");
			requestEvent();
			break;

		case cmdSetMotor:
			currentMotor = cmd[2] & 0x01;
			Serial.print("Current motor is ");
			Serial.println(currentMotor);
			break;

		case cmdMode:
			mode[currentMotor] = cmd[2] & 0x01;
			Serial.print("Motor mode is ");
			Serial.println(mode[currentMotor]);
			break;

		case cmdForward:
			Serial.println("Moving forward...");
			state[currentMotor] = FORWARD;
			Serial.println("State = FORWARD");
			break;

		case cmdBackwards:
			Serial.println("Moving backwards...");
			state[currentMotor] = BACKWARDS;
			Serial.println("State = BACKWARDS");
			break;

		case cmdHome:
			Serial.println("Going home...");
			state[currentMotor] = HOME;
			Serial.println("State = HOME");
			break;

		case cmdSpeed:
			speed[currentMotor] = cmd[2];
			Serial.print("Speed set to ");
			Serial.println(speed[currentMotor]);
			break;

		case cmdGoal:
			targetPulses[currentMotor] = ((word)(byte)(cmd[2]) | ((word)(byte)(cmd[3]) << 8));
			Serial.print("Goal set to ");
			Serial.print(targetPulses[currentMotor]);
			Serial.println(" pulses");
			break;

		default:
			Serial.print("Unknown command: \"");
			Serial.print(cmd[0]);
			Serial.print(cmd[1]);
			Serial.println("\"");
			break;
	}
}

// Sends data requested by master

void requestEvent()
{
	byte data[4];

	data[0] = state[currentMotor];
	data[1] = currentMotor;
	data[2] = pulses[currentMotor] & 0xff;
	data[3] = (pulses[currentMotor] & 0xff00) >> 8;
	Wire.write(data, 4);

	Serial.println("--------- Requested: ");
	Serial.println(state[currentMotor]);
	Serial.println(pulses[currentMotor]);
	Serial.println("---------");
}

void sendSignal()
{
	// Signals to the micro:bit that an operation was completed

	digitalWrite(eventPin, HIGH);
	delay(10);
	digitalWrite(eventPin, LOW);
	delay(100);

	// Serial.print("Pulses: ");
	// Serial.println(pulses[currentMotor]);
}

void initMotors()
{
	// TODO: set the PWM frequency 

	pinMode(motorAPin1, OUTPUT);
	pinMode(motorAPin2, OUTPUT);
	pinMode(motorBPin1, OUTPUT);
	pinMode(motorBPin2, OUTPUT);
}

void initEncoders()
{
	pinMode(encoderA1, INPUT);
	pinMode(encoderA2, INPUT);
	pinMode(encoderB1, INPUT);
	pinMode(encoderB2, INPUT);

	attachInterrupt(0, calcPulse, CHANGE);
}

void resetState()
{
	pulses[MOTOR_A] = 0;
	pulses[MOTOR_B] = 0;
	direction[MOTOR_A] = false;
	direction[MOTOR_B] = false;
	state[MOTOR_A] = IDLE;
	state[MOTOR_B] = IDLE;
	Serial.println("State = IDLE");
}

void calcPulse()
{
	pulses[currentMotor]++;
}

// ******* NOT TESTED
void calcPulseWithQuadrature()
{
	int Lstate = digitalRead(encoderA1);

	if((encoderLast[currentMotor] == LOW) && Lstate == HIGH) {
		int val = digitalRead(encoderA2);
		if(val == LOW && direction[currentMotor]) {
			direction[currentMotor] = false;	// Reverse
		} else if(val == HIGH && !direction[currentMotor]) {
			direction[currentMotor] = true;	// Forward
		}
	}
	encoderLast[currentMotor] = Lstate;

	if(!direction[currentMotor]) {
		pulses[currentMotor]++;
	} else {
		pulses[currentMotor]--;
	}
}

// -----------------------------------------------------------------------------
