// -----------------------------------------------------------------------------

// fischertechnik driver for DC motors with encoder and servos
// Rubem Pechansky

// References:
// https://playground.arduino.cc/Main/PinChangeInterrupt/

// -----------------------------------------------------------------------------

// #include <Arduino.h>
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
#define ATHOME(m)	(!digitalRead(m == MOTOR_A ? motorAHome : motorBHome))
#define ONEND(m)	(!digitalRead(m == MOTOR_A ? motorAEnd : motorBEnd))

// Arduino pins

#define motorAOut1	3
#define motorAOut2	11
#define motorBOut1	5
#define motorBOut2	6

#define servo1		9
#define servo2		10

#define encoderA	A0
#define encoderB	A1

#define eventPin	13

#define motorAHome	7
#define motorBHome	8
#define motorAEnd	A2
#define motorBEnd	A3

// I2C commands

#define cmdBackwards	TWOCC('B', 'w')
#define cmdBrake		TWOCC('B', 'r')
#define cmdCoast		TWOCC('C', 'o')
#define cmdForward		TWOCC('F', 'w')
#define cmdGoal			TWOCC('G', 'o')
#define cmdHome			TWOCC('H', 'o')
#define cmdHello		TWOCC('H', 'i')
#define cmdMode			TWOCC('M', 'd')
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
	GOINGHOME,
	BRAKE,
	RESET,
};

// Motors

enum {
	MOTOR_A = 0,
	MOTOR_B,
};

// Motor modes

enum {
	DIRECT = 0,
	PULSES,
	ENDSWITCH,
};

// -----------------------------------------------------------------------------

// Variables

byte encoderLast[2] = {-1, -1};
byte state[2] = {COAST, COAST};
byte speed[2] = {255, 255};
byte mode[2] = {DIRECT, DIRECT};
uint pulses[2] = {0, 0};
uint targetPulses[2] = {0, 0};
bool encoderAstatus = false;
bool encoderBstatus = false;

// -----------------------------------------------------------------------------

void setup()
{
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(requestEvent);
	Wire.onReceive(receiveEvent);

	Serial.begin(BAUD_RATE);
	Serial.println("Arduino reset");

	pinMode(motorAHome, INPUT_PULLUP);
	pinMode(motorAEnd, INPUT_PULLUP);
	pinMode(motorBHome, INPUT_PULLUP);
	pinMode(motorBEnd, INPUT_PULLUP);

	pinMode(eventPin, OUTPUT);
	digitalWrite(eventPin, LOW);

	initMotors();
	initEncoders();
}

// -----------------------------------------------------------------------------

void loop()
{
	processMotor(MOTOR_A);
	processMotor(MOTOR_B);
}

// -----------------------------------------------------------------------------

void processMotor(byte motor)
{
	switch(state[motor]) {

		case HOME:

			if(ATHOME(motor)) {
				resetState(motor);
			} else {
				state[motor] = GOINGHOME;
				MotorCW(motor);
				Serial.println("State = GOINGHOME");
			}
			break;

		case FORWARD:

			pulses[motor] = 0;
			state[motor] = CCW;
			Serial.println("State = CCW");
			MotorCCW(motor);
			break;

		case BACKWARDS:

			pulses[motor] = 0;
			state[motor] = CW;
			Serial.println("State = CW");
			MotorCW(motor);
			break;

		case BRAKE:

			MotorBrake(motor);
			break;

		case COAST:

			MotorCoast(motor);
			break;

		case GOINGHOME:

			if(ATHOME(motor)) {
				MotorBrake(motor);
				sendSignal();
				pulses[motor] = 0;
				state[motor] = BRAKE;
				Serial.println("State = BRAKE");
			}
			break;

		case CW:
		case CCW:

			if((mode[motor] == PULSES && pulses[motor] >= targetPulses[motor]) ||
				(mode[motor] == ENDSWITCH && ONEND(motor))) {
				MotorBrake(motor);
				sendSignal();
				state[motor] = BRAKE;
				Serial.print("Pulses: ");
				Serial.println(pulses[motor]);
			}
			break;

	}
}

// -----------------------------------------------------------------------------

// Receives data from master

void receiveEvent(int nBytes)
{
	byte motor;
	char cmd[8];

	// Serial.print("Receiving ");
	// Serial.print(nBytes);
	// Serial.print(" bytes: ");

	for(int count = 0; Wire.available(); count++) {
		cmd[count] = Wire.read();
	}

	// Received commands

	switch(((word)(byte)cmd[0] | ((word)(byte)cmd[1] << 8))) {

		case cmdHello:
			Serial.println("Hello yourself!");
			initMotors();
			MotorCoast(MOTOR_A);
			MotorCoast(MOTOR_B);
			requestEvent();
			break;

		case cmdMode:
			motor = cmd[2] & 0x01;
			mode[motor] = cmd[3];
			switch(mode[motor]) {
				case DIRECT: printMotorCmd(motor, "mode", "DIRECT"); break;
				case PULSES: printMotorCmd(motor, "mode", "PULSES"); break;
				case ENDSWITCH: printMotorCmd(motor, "mode", "ENDSWITCH"); break;
			}
			break;

		case cmdForward:
			motor = cmd[2] & 0x01;
			printMotorCmd(motor, "state", "FORWARD");
			state[motor] = FORWARD;
			break;

		case cmdBackwards:
			motor = cmd[2] & 0x01;
			printMotorCmd(motor, "state", "BACKWARDS");
			state[motor] = BACKWARDS;
			break;

		case cmdBrake:
			motor = cmd[2] & 0x01;
			printMotorCmd(motor, "state", "BRAKING");
			state[motor] = BRAKE;
			break;

		case cmdCoast:
			motor = cmd[2] & 0x01;
			printMotorCmd(motor, "state", "COASTING");
			state[motor] = COAST;
			break;

		case cmdHome:
			motor = cmd[2] & 0x01;
			printMotorCmd(motor, "state", "GOING HOME");
			state[motor] = HOME;
			break;

		case cmdSpeed:
			motor = cmd[2] & 0x01;
			speed[motor] = cmd[3];
			printMotorCmd(motor, "speed", speed[motor]);
			break;

		case cmdGoal:
			motor = cmd[2] & 0x01;
			targetPulses[motor] = ((word)(byte)(cmd[3]) | ((word)(byte)(cmd[4]) << 8));
			printMotorCmd(motor, "goal", targetPulses[motor]);
			break;

		default:
			Serial.print("Unknown command: \"");
			Serial.print(cmd[0]);
			Serial.print(cmd[1]);
			Serial.println("\"");
			break;
	}
}

// Debug methods

void printMotorCmd(byte motor, char *name, int value)
{
	Serial.print("Motor ");
	Serial.print(motor ? "B " : "A ");
	Serial.print(name);
	Serial.print(" is ");
	Serial.println(value);
}

void printMotorCmd(byte motor, char *name, char *str)
{
	Serial.print("Motor ");
	Serial.print(motor ? "B " : "A ");
	Serial.print(name);
	Serial.print(" is ");
	Serial.println(str);
}

// Sends data requested by master

void requestEvent()
{
	byte data[4];

	data[0] = pulses[MOTOR_A] & 0xff;
	data[1] = (pulses[MOTOR_A] & 0xff00) >> 8;
	data[2] = pulses[MOTOR_B] & 0xff;
	data[3] = (pulses[MOTOR_B] & 0xff00) >> 8;

	Wire.write(data, 4);

	for(int motor = MOTOR_A; motor < 2; motor++) {
		printMotorCmd(motor, "pulses", pulses[motor]);
	}
}

void sendSignal()
{
	// Signals to the micro:bit that an operation was completed

	digitalWrite(eventPin, HIGH);
	delay(10);
	digitalWrite(eventPin, LOW);
	delay(10);
}

void initMotors()
{
	// TODO: set the PWM frequency 

	pinMode(motorAOut1, OUTPUT);
	pinMode(motorAOut2, OUTPUT);
	pinMode(motorBOut1, OUTPUT);
	pinMode(motorBOut2, OUTPUT);

	resetState(MOTOR_A);
	resetState(MOTOR_B);
}

void MotorCCW(byte motor)
{
	digitalWrite(motor == MOTOR_A ? motorAOut1 : motorBOut1, HIGH);
	analogWrite(motor == MOTOR_A ? motorAOut2 : motorBOut2, 255 - speed[motor]);
}

void MotorCW(byte motor)
{
	analogWrite(motor == MOTOR_A ? motorAOut1 : motorBOut1, 255 - speed[motor]);
	digitalWrite(motor == MOTOR_A ? motorAOut2 : motorBOut2, HIGH);
}

void MotorCoast(byte motor)
{
	digitalWrite(motor == MOTOR_A ? motorAOut1 : motorBOut1, LOW);
	digitalWrite(motor == MOTOR_A ? motorAOut2 : motorBOut2, LOW);
}

void MotorBrake(byte motor)
{
	digitalWrite(motor == MOTOR_A ? motorAOut1 : motorBOut1, HIGH);
	digitalWrite(motor == MOTOR_A ? motorAOut2 : motorBOut2, HIGH);
}

void resetState(byte motor)
{
	pulses[motor] = 0;
	state[motor] = COAST;
	printMotorCmd(motor, "state", state[motor]);
}

void initEncoders()
{
	pulses[0] = 0;
	pulses[1] = 0;
	encoderAstatus = false;
	encoderBstatus = false;

	pciSetup(encoderA);
	pciSetup(encoderB);
}

// Install pin change interrupt for a pin

void pciSetup(byte pin)
{
	pinMode(pin, INPUT_PULLUP);
    *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR(PCINT1_vect) // handle pin change interrupt for A0 to A5 here
{
	bool encA = digitalRead(encoderA);

	if(!encoderAstatus && encA) {
		pulses[0]++;
		encoderAstatus = true;
	} else if(!encA) {
		encoderAstatus = false;
	}

	bool encB = digitalRead(encoderB);

	if(!encoderBstatus && encB) {
		pulses[1]++;
		encoderBstatus = true;
	} else if(!encB) {
		encoderBstatus = false;
	}
}

// -----------------------------------------------------------------------------
