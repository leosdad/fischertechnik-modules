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
#define ATHOME(m)	(!digitalRead(m == MOTOR_1 ? motor1Home : motor2Home))
#define ONEND(m)	(!digitalRead(m == MOTOR_1 ? motor1End : motor2End))

// Arduino pins

#define motor1OutA	3
#define motor1OutB	9
#define motor2OutA	5
#define motor2OutB	6

#define servo1		11
#define servo2		10

#define encoder1	A0
#define encoder2	A1

#define messagePin	13

#define motor1Home	8
#define motor2Home	7
#define motor1End	A2
#define motor2End	A3

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
	MOTOR_1 = 0,
	MOTOR_2,
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
byte prevState[2] = {255, 255};
byte speed[2] = {255, 255};
byte mode[2] = {DIRECT, DIRECT};
uint pulses[2] = {0, 0};
uint targetPulses[2] = {0, 0};
bool encoder1status = false;	// TODO: Array here
bool encoder2status = false;

// -----------------------------------------------------------------------------

void setup()
{
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(requestEvent);
	Wire.onReceive(receiveEvent);

	Serial.begin(BAUD_RATE);
	Serial.println("Arduino reset");

	pinMode(motor1Home, INPUT_PULLUP);
	pinMode(motor1End, INPUT_PULLUP);
	pinMode(motor2Home, INPUT_PULLUP);
	pinMode(motor2End, INPUT_PULLUP);

	pinMode(messagePin, OUTPUT);
	digitalWrite(messagePin, LOW);

	initMotors();
	initEncoders();
}

// -----------------------------------------------------------------------------

void loop()
{
	processMotor(MOTOR_1);
	processMotor(MOTOR_2);
}

// Test loop

void __loop()
{
	byte motor = MOTOR_1;

	MotorCCW(motor);
	Serial.println("CCW");
	delay(1000);

	MotorBrake(motor);
	Serial.println("Brake");
	delay(1000);

	MotorCW(motor);
	Serial.println("CW");
	delay(1000);

	MotorBrake(motor);
	Serial.println("Brake");
	delay(1000);
}

// -----------------------------------------------------------------------------

void processMotor(byte motor)
{
	if(state[motor] != prevState[motor]) {
		Serial.print("State: ");
		Serial.println(state[motor]);
		prevState[motor] = state[motor];
	}

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
				sendMessage();
				pulses[motor] = 0;
				state[motor] = BRAKE;
				Serial.println("State = BRAKE");
			}
			break;

		case CW:
		case CCW:

			// Serial.println(mode[motor] == PULSES && pulses[motor] >= targetPulses[motor]);

			if((mode[motor] == PULSES && pulses[motor] >= targetPulses[motor]) ||
				(mode[motor] == ENDSWITCH && ONEND(motor))) {
				MotorBrake(motor);
				sendMessage();
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
			MotorCoast(MOTOR_1);
			MotorCoast(MOTOR_2);
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

	data[0] = pulses[MOTOR_1] & 0xff;
	data[1] = (pulses[MOTOR_1] & 0xff00) >> 8;
	data[2] = pulses[MOTOR_2] & 0xff;
	data[3] = (pulses[MOTOR_2] & 0xff00) >> 8;

	Wire.write(data, 4);

	for(int motor = MOTOR_1; motor < 2; motor++) {
		printMotorCmd(motor, "pulses", pulses[motor]);
	}
}

void sendMessage()
{
	// Signals to the micro:bit that an operation was completed

	digitalWrite(messagePin, HIGH);
	delay(10);
	digitalWrite(messagePin, LOW);
	delay(10);
}

void initMotors()
{
	// TODO: set the PWM frequency 

	pinMode(motor1OutA, OUTPUT);
	pinMode(motor1OutB, OUTPUT);
	pinMode(motor2OutA, OUTPUT);
	pinMode(motor2OutB, OUTPUT);

	resetState(MOTOR_1);
	resetState(MOTOR_2);
}

void MotorCCW(byte motor)
{
	digitalWrite(motor == MOTOR_1 ? motor1OutA : motor2OutA, HIGH);
	analogWrite(motor == MOTOR_1 ? motor1OutB : motor2OutB, 255 - speed[motor]);
}

void MotorCW(byte motor)
{
	analogWrite(motor == MOTOR_1 ? motor1OutA : motor2OutA, 255 - speed[motor]);
	digitalWrite(motor == MOTOR_1 ? motor1OutB : motor2OutB, HIGH);
}

void MotorCoast(byte motor)
{
	digitalWrite(motor == MOTOR_1 ? motor1OutA : motor2OutA, LOW);
	digitalWrite(motor == MOTOR_1 ? motor1OutB : motor2OutB, LOW);
}

void MotorBrake(byte motor)
{
	digitalWrite(motor == MOTOR_1 ? motor1OutA : motor2OutA, HIGH);
	digitalWrite(motor == MOTOR_1 ? motor1OutB : motor2OutB, HIGH);
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
	encoder1status = false;
	encoder2status = false;

	pciSetup(encoder1);
	pciSetup(encoder2);
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
	bool enc1 = digitalRead(encoder1);

	if(!encoder1status && enc1) {
		pulses[0]++;
		// Serial.println(pulses[0]);
		encoder1status = true;
	} else if(!enc1) {
		encoder1status = false;
	}

	bool enc2 = digitalRead(encoder2);

	if(!encoder2status && enc2) {
		pulses[1]++;
		encoder2status = true;
	} else if(!enc2) {
		encoder2status = false;
	}
}

// -----------------------------------------------------------------------------
