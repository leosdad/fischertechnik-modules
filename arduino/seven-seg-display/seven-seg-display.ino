/*

Multiplexed 7-segment displays
Based on: https://github.com/gbhug5a/7-Segment-Displays-Multiplex-by-Segment

TODO:

* Scroll string
- Display integer
- Left pad integer with zeros
- Flash display
- Dim display (must use PWM pins for CACC)
- Animations

*/

#include <Arduino.h>
#include <Wire.h>
#include <Ftmodules.h>

#pragma region Constants / variables -------------------------------------------

#define BAUD_RATE			57600
#define SLAVE_ADDRESS		0x09

// Main constants

const byte SEGMENTS = 7;	// Number of segments. 8 if using decimal point
const byte DIGITS = 6;		// Number of displays used
const byte Refresh = 1;		// Number of millis changes between segments
const byte MAXCHARS = 40;	// TODO: passar para ftmodules

// Define the pins used for the common segments - need not be consecutive

const byte SEGApin = A2;
const byte SEGBpin = A1;
const byte SEGCpin = 2;
const byte SEGDpin = 3;
const byte SEGEpin = 4;
const byte SEGFpin = A3;
const byte SEGGpin = 9;

// Array allows pins to be addressed in A-G sequence regardless of pin numbers

byte SEGARRAY[] = {SEGApin, SEGBpin, SEGCpin, SEGDpin, SEGEpin, SEGFpin, SEGGpin};

// Define pins used by common anodes or common cathodes - add others as needed

const byte CACC0pin = 11;
const byte CACC1pin = 12;
const byte CACC2pin = A0;
const byte CACC3pin = 6;
const byte CACC4pin = 7;
const byte CACC5pin = 5;

// Array allows using any number of digits - add others as needed

byte CACCpin[] = {CACC0pin, CACC1pin, CACC2pin, CACC3pin, CACC4pin, CACC5pin}; // The digit's pin number
byte DIGIT[DIGITS];			// And its displayed character
byte tempDigits[DIGITS];			// Used for animations

// Use these defs for common anode displays

const byte SEGON = LOW;
const byte SEGOFF = HIGH;
const byte CACCON = HIGH;
const byte CACCOFF = LOW;

// enums

// I²C commands

enum
{
	mdDefault = 0,
	mdFlash,
	mdRotate
};

// The bit value of each segment

const byte A = bit(0);
const byte B = bit(1);
const byte C = bit(2);
const byte D = bit(3);
const byte E = bit(4);
const byte F = bit(5);
const byte G = bit(6);
//const byte H  = bit(7);          // The decimal point if used

// Segment patterns of the characters

const byte cNA = A | D | G;
const byte cSpace = 0;
const byte cQuote = F | B;
const byte cAphos = F;
const byte cComma = E;
const byte cHyphen = G;
const byte cEqual = G | D;
const byte cLBrck = A | D | E | F;
const byte cRBrck = A | B | C | D;
const byte cUnder = D;

const byte cDegre = A | B | G | F;			// *
const byte cOverl = A;						// @
const byte cDash2 = A | D;					// #

const byte c0 = A | B | C | D | E | F;
const byte c1 = B | C;
const byte c2 = A | B | D | E | G;
const byte c3 = A | B | C | D | G;
const byte c4 = B | C | F | G;
const byte c5 = A | C | D | F | G;
const byte c6 = A | C | D | E | F | G;
const byte c7 = A | B | C;
const byte c8 = A | B | C | D | E | F | G;
const byte c9 = A | B | C | D | F | G;

const byte cA = A | B | C | E | F | G;
const byte cC = A | D | E | F;
const byte cE = A | D | E | F | G;
const byte cF = A | E | F | G;
const byte cG = A | C | D | E | F;
const byte cH = B | C | E | F | G;
const byte cI = E | F;
const byte cJ = B | C | D | E;
const byte cL = D | E | F;
const byte cO = A | B | C | D | E | F;
const byte cP = A | B | E | F | G;
const byte cS = A | C | D | F | G;
const byte cU = B | C | D | E | F;

const byte cb = C | D | E | F | G;
const byte cc = D | E | G;
const byte cd = B | C | D | E | G;
const byte cg = A | B | C | D | F | G;
const byte ch = C | E | F | G;
const byte ci = C;
const byte cl = B | C;
const byte cn = C | E | G;
const byte co = C | D | E | G;
const byte cq = A | B | C | F | G;
const byte cr = E | G;
const byte ct = D | E | F | G;
const byte cu = C | D | E;
const byte cy = B | C | D | F | G;

// Array links a value to its character

byte ascii[] = {
	cSpace, cNA, cQuote, cDash2, cS, cNA, cNA, cAphos, cNA, cNA, cDegre, cNA, cComma, cHyphen, cNA, cNA,
	c0, c1, c2, c3, c4, c5, c6, c7,	c8, c9, cNA, cNA, cNA, cEqual, cNA, cNA,
	cOverl, cA, cb, cC, cd, cE, cF, cG, cH, cI, cJ, cNA, cL, cNA, cn, cO,
	cP, cq, cr, cS, ct, cU, cNA, cNA, cNA, cy, c2, cLBrck, cNA, cRBrck, cNA, cUnder,
	cAphos, cA, cb, cc, cd, cE, cF, cg, ch, ci, cJ, cNA, cl, cNA, cn, co,
	cP, cq, cr, cS, ct, cu, cNA, cNA, cNA, cy, c2, cNA, cI, cNA, cNA, cNA
};

//OLA DANIELA, LEONARDO E LETICIA ";

unsigned long PREVmillis;
unsigned long CURmillis;

byte SEGCOUNT;			// Segment counter - count up to SEGMENTS value
byte CURSEG;			// Current segment bit position
byte milliCount = 0;	// Number of millis changes so far
byte i;

unsigned long ms = 0;	// Used for animations
byte auxPos = 0;		// Used in rotateString, flash
byte textSize = 0;		// Used in rotateString
byte mode = mdDefault;
unsigned int flashPeriod = 0;
unsigned int rotatePeriod = 0;

char str[MAXCHARS + 1];

#pragma endregion-- ------------------------------------------------------------

void setup()
{
	// Serial.begin(BAUD_RATE);
	// Serial.println("Display started");

	Wire.begin(SLAVE_ADDRESS);
	Wire.onReceive(receiveEvent);

	// Initialize segment pins to OUTPUT, off

	for(i = 0; i < SEGMENTS; i++) {
		pinMode(SEGARRAY[i], OUTPUT);
		digitalWrite(SEGARRAY[i], SEGOFF);
	}

	// Same for CACC pins

	for(i = 0; i < DIGITS; i++) {
		pinMode(CACCpin[i], OUTPUT);
		digitalWrite(CACCpin[i], CACCOFF);
	}

	clearString();

	// Set all digits to blank

	blank();

	// Initialize so first refresh will re-init everything

	SEGCOUNT = SEGMENTS - 1;	// Segments counter - set to end
	CURSEG = bit(SEGMENTS - 1); // Bit position of last segment

	PREVmillis = millis();
}

void loop()
{
	CURmillis = millis();

	if(CURmillis != PREVmillis) {
		milliCount++;
		PREVmillis = CURmillis;
	}

	if(milliCount == Refresh) {
		milliCount = 0;

		// Turn the current segment OFF while making changes - prevents ghosting

		digitalWrite(SEGARRAY[SEGCOUNT], SEGOFF);

		//This section selects the next segment

		CURSEG = CURSEG << 1; // shift left to next bit position
		SEGCOUNT++;			  // used as index into SEGARRAY

		if(SEGCOUNT == SEGMENTS) {		// If done with last segment, start over
			SEGCOUNT = 0; //re-initialize
			CURSEG = 1;
		}

		// This section turns the CA or CC pins on/off per the patterns of the characters
		// If the CURSEG bit of the DIGIT[n] segment pattern is 1, turn on the CACCpin[n]

		for(i = 0; i < DIGITS; ++i) {
			if(DIGIT[i] & CURSEG) {
				digitalWrite(CACCpin[i], CACCON);
			} else {
				analogWrite(CACCpin[i], CACCOFF);
			}
		}

		// Now turn the new segment driver ON

		digitalWrite(SEGARRAY[SEGCOUNT], SEGON);
	}

	switch(mode) {
		case mdFlash:
			flash(flashPeriod);
			break;
		case mdRotate:
			rotate(rotatePeriod);
			break;
		}
}

// Receives data from master

void receiveEvent(int nBytes)
{
	byte motor;
	char cmd[MAXCHARS];

	for(int count = 0; count < MAXCHARS; count++) {
		cmd[count] = Wire.available() ? Wire.read() : '\x0';
	}

	// Serial.println("Received");

	// Received commands

	switch((byte)cmd[0]) {

		case Ftmodules::SevenSegDisplay::cmdBlank:
			blank();
			break;

		case Ftmodules::SevenSegDisplay::cmdTest:
			testSegments();
			break;

		case Ftmodules::SevenSegDisplay::cmdDisplay:
			displayString(cmd + 1);
			break;

		case Ftmodules::SevenSegDisplay::cmdFlash:
			flashPeriod = (cmd[2] << 8) + (unsigned char)cmd[1];
			mode = mdFlash;
			break;

		case Ftmodules::SevenSegDisplay::cmdRotate:
			// rotateString(cmd + 2, (unsigned int)cmd[1]);
			rotatePeriod = (cmd[2] << 8) + (unsigned char)cmd[1];
			mode = mdRotate;
			break;

		case Ftmodules::SevenSegDisplay::cmdStop:
			mode = mdDefault;
			break;

		default:
			displayString("E201");	// Error 201
			// Serial.print("Unknown command: \"");
			// Serial.print(cmd[0]);
			// Serial.println("\"");
			break;
	}
}

void blank()
{
	for(i = 0; i < DIGITS; ++i) {
		DIGIT[i] = cSpace;
	}
}

void testSegments()
{
	for(i = 0; i < DIGITS; ++i) {
		DIGIT[i] = c8;
	}
}

void displayString(char *text)
{
	textSize = strlen(text);
	clearString();
	// Serial.println(textSize);
	strcpy(str, text);

	for(i = 0; i < DIGITS; ++i) {
		DIGIT[i] = i < textSize ? ascii[text[i] - 0x20] : cSpace;
	}
}

void flash(unsigned int animPeriod)
{
	if(CURmillis >= ms + animPeriod) {
		if(auxPos == LOW) {
			for(i = 0; i < DIGITS; i++) {
				tempDigits[i] = DIGIT[i];
				DIGIT[i] = cSpace;
			}
		} else {
			for(i = 0; i < DIGITS; i++) {
				DIGIT[i] = tempDigits[i];
			}
		}
		auxPos = auxPos ? LOW : HIGH;
		ms = CURmillis;
	}
}

void rotate(unsigned int rotatePeriod)
{
	if(CURmillis >= ms + rotatePeriod) {
		for(i = 0; i < DIGITS; ++i) {
			byte offset = auxPos < textSize - i ? auxPos + i : auxPos - textSize + i;
			DIGIT[i] = ascii[str[offset] - 0x20];
		}
		auxPos = auxPos < textSize - 1 ? auxPos + 1 : 0;
		ms = CURmillis;
	}
}

void clearString()
{
	for(i = 0; i < MAXCHARS; i++) {
		str[i] = '\x0';
	}
}

// -----------------------------------------------------------------------------
