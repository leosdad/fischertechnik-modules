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

// Arduino pins

#define motorAPin1	10
#define motorAPin2	11
#define motorBPin1	5
#define motorBPin2	6

#define encoderA	2
#define encoderB	3

#define limitA1Pin	14
#define limitA2Pin	15
#define limitB1Pin	16
#define limitB2Pin	17

// I2C commands

#define cmdBackwards	TWOCC('B', 'w')
#define cmdBrake		TWOCC('B', 'r')
#define cmdCoast		TWOCC('C', 'o')
#define cmdForward		TWOCC('F', 'w')
#define cmdGoal			TWOCC('G', 'o')
#define cmdHome			TWOCC('H', 'o')
#define cmdHello		TWOCC('H', 'i')
#define cmdMode			TWOCC('M', 'd')
#define cmdSetMotor		TWOCC('M', 't')
#define cmdSpeed		TWOCC('S', 'p')

// -----------------------------------------------------------------------------

// Motor states

enum {
	COAST = 0,
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
	DIRECT = 0,
	PULSES,
	ENDLIMITSWITCH,
};

// -----------------------------------------------------------------------------

// Variables

bool direction[2] = {false, false};
byte currentMotor = MOTOR_A;
byte encoderLast[2] = {-1, -1};
byte state[2] = {COAST, COAST};
byte speed[2] = {255, 255};
byte mode[2] = {DIRECT, DIRECT};
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

	// pinMode(encoderA, INPUT_PULLUP);	// Does nothing: probably 20kΩ is not enough
	// pinMode(encoderB, INPUT_PULLUP);

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
				MotorBrake();
				resetState();
			} else {
				pulses[currentMotor] = 0;
				state[currentMotor] = GOHOME;
				MotorCW();
				Serial.println("State = GOHOME");
			}
			break;

		case FORWARD:

			if(mode[currentMotor] != DIRECT) {
				pulses[currentMotor] = 0;
				state[currentMotor] = CCW;
				Serial.println("State = CCW");
			}
			MotorCCW();
			break;

		case BACKWARDS:

			if(mode[currentMotor] != DIRECT) {
				pulses[currentMotor] = 0;
				state[currentMotor] = CW;
				Serial.println("State = CW");
			}
			MotorCW();
			break;

		case BRAKE:

			MotorBrake();
			break;

		case COAST:

			MotorCoast();
			break;

		case GOHOME:

			if(!digitalRead(currentMotor == MOTOR_A ? limitA1Pin : limitB1Pin)) {
				// Stop motor
				MotorBrake();
				pulses[currentMotor] = 0;
				state[currentMotor] = COAST;
				Serial.println("State = COAST");
			}
			break;

		case CW:
		case CCW:

			if(mode[currentMotor] == PULSES) {
				if(pulses[currentMotor] >= targetPulses[currentMotor]) {
					// Stop motor
					MotorBrake();
					pulses[currentMotor] = 0;
					state[currentMotor] = COAST;
					Serial.println("State = COAST");
				// } else {
					// Serial.print(pulses[currentMotor]);
					// Serial.print(" / ");
					// Serial.println(speed[currentMotor]);
				}
			} else {
				if(!digitalRead(currentMotor == MOTOR_A ? limitA2Pin : limitB2Pin)) {
					MotorBrake();
					state[currentMotor] = COAST;
					Serial.println("State = COAST");
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
			mode[currentMotor] = cmd[2] & 0x0F;
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

		case cmdBrake:
			Serial.println("Braking...");
			state[currentMotor] = BRAKE;
			Serial.println("State = BRAKE");
			break;

		case cmdCoast:
			Serial.println("Coasting...");
			state[currentMotor] = COAST;
			Serial.println("State = COAST");
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

	data[0] = pulses[0] & 0xff;
	data[1] = (pulses[0] & 0xff00) >> 8;
	data[2] = pulses[1] & 0xff;
	data[3] = (pulses[1] & 0xff00) >> 8;

	Wire.write(data, 4);

	Serial.println("--------- Requested:");
	for(int motor = 0; motor < 2; motor++) {
		Serial.print("  Motor ");
		Serial.print(motor);
		Serial.print(" / state ");
		Serial.print(state[motor]);
		Serial.print(" / pulses ");
		Serial.println(pulses[motor]);
	}
	Serial.println("---------");
}

void initMotors()
{
	// TODO: set the PWM frequency 

	pinMode(motorAPin1, OUTPUT);
	pinMode(motorAPin2, OUTPUT);
	pinMode(motorBPin1, OUTPUT);
	pinMode(motorBPin2, OUTPUT);
}

void MotorCCW()
{
	digitalWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, HIGH);
	analogWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, 255 - speed[currentMotor]);
}

void MotorCW()
{
	analogWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, 255 - speed[currentMotor]);
	digitalWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, HIGH);
}

void MotorCoast()
{
	digitalWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, LOW);
	digitalWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, LOW);
}

void MotorBrake()
{
	digitalWrite(currentMotor == MOTOR_A ? motorAPin1 : motorBPin1, HIGH);
	digitalWrite(currentMotor == MOTOR_A ? motorAPin2 : motorBPin2, HIGH);
}

void resetState()
{
	pulses[MOTOR_A] = 0;
	pulses[MOTOR_B] = 0;
	direction[MOTOR_A] = false;
	direction[MOTOR_B] = false;
	state[MOTOR_A] = COAST;
	state[MOTOR_B] = COAST;
	Serial.println("State = COAST");
}

void initEncoders()
{
	pinMode(encoderA, INPUT);
	pinMode(encoderB, INPUT);
	attachInterrupt(digitalPinToInterrupt(encoderA), incrementPulseA, FALLING);
	attachInterrupt(digitalPinToInterrupt(encoderB), incrementPulseB, FALLING);
	// attachInterrupt(0, incrementPulseA, CHANGE);
	// attachInterrupt(1, incrementPulseB, CHANGE);
}

void incrementPulseA()
{
	pulses[0]++;
}

void incrementPulseB()
{
	pulses[1]++;
}

// -----------------------------------------------------------------------------
