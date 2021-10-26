/*

Multiplexed 7-segment displays
Based on: https://github.com/gbhug5a/7-Segment-Displays-Multiplex-by-Segment

*/

#include <Arduino.h>
#include <Wire.h>
#include <Ftmodules.h>
#include "Simpletypes.h"

#pragma region Hardware constants ----------------------------------------------

#define BAUD_RATE			57600
#define ADDRESS				0x09

// Main constants

const byte SEGMENTS = 7;	// Number of segments. 8 if using decimal point
const byte DIGITS = 6;		// Number of displays used
const byte Refresh = 1;		// Number of millis changes between segments

// Define the pins used for the common segments - need not be consecutive

const byte SEGApin = A2;
const byte SEGBpin = A1;
const byte SEGCpin = 2;
const byte SEGDpin = 3;
const byte SEGEpin = 4;
const byte SEGFpin = A3;
const byte SEGGpin = 9;

// Array allows pins to be addressed in A-G sequence regardless of pin numbers

byte segArray[] = {SEGApin, SEGBpin, SEGCpin, SEGDpin, SEGEpin, SEGFpin, SEGGpin};

// Define pins used by common anodes or common cathodes - add others as needed

const byte CACC0pin = 11;
const byte CACC1pin = 12;
const byte CACC2pin = A0;
const byte CACC3pin = 6;
const byte CACC4pin = 7;
const byte CACC5pin = 5;

// Use these defs for common anode displays

const byte SEGON = LOW;
const byte SEGOFF = HIGH;
const byte CACCON = HIGH;
const byte CACCOFF = LOW;

// The bit value of each segment

const byte A = bit(0);
const byte B = bit(1);
const byte C = bit(2);
const byte D = bit(3);
const byte E = bit(4);
const byte F = bit(5);
const byte G = bit(6);
//const byte H  = bit(7);          // The decimal point if used

#pragma endregion --------------------------------------------------------------

#pragma region Display constants -----------------------------------------------

// enums

const byte MAXCHARS = 80;	// TODO: passar para ftmodules

#pragma endregion --------------------------------------------------------------

#pragma region Enums -----------------------------------------------------------

// I²C commands

enum class i2cCommands
{
	Default = 0,
	Flash,
	Rotate
};

#pragma endregion --------------------------------------------------------------

#pragma region Variables -------------------------------------------------------

// Array allows using any number of digits - add others as needed

// The digit's pin number
byte CACCpin[] = {CACC0pin, CACC1pin, CACC2pin, CACC3pin, CACC4pin, CACC5pin};
byte DIGIT[DIGITS];					// And its displayed character
byte tempDigits[DIGITS];			// Used for animations

unsigned long prevMs;
unsigned long currentMs;

char str[MAXCHARS + 1];

byte segmentCounter;			// Segment counter - count up to SEGMENTS value
byte currentSegment;			// Current segment bit position
byte milliCount = 0;			// Number of millis changes so far

unsigned long ms = 0;			// Used for animations
byte auxPos = 0;				// Used in rotateString, flash
byte textSize = 0;				// Used in rotateString
i2cCommands mode = i2cCommands::Default;
unsigned int flashPeriod = 0;
unsigned int rotatePeriod = 0;

unsigned long timeout = 0;

#pragma endregion --------------------------------------------------------------

#pragma region Character patterns ----------------------------------------------

// Segment patterns of the characters

const byte cNA = A | D | G;

const byte cSpace = 0;						// space
const byte cQuote = F | B;					// "
const byte cAphos = B;						// '
const byte cBTick = F;						// `
const byte cComma = E;						// ,
const byte cHyphen = G;						// -
const byte cEqual = G | D;					// =
const byte cLBrck = A | D | E | F;			// [
const byte cRBrck = A | B | C | D;			// ]
const byte cAster = A | B | G | F;			// * (°)
const byte cOverl = A;						// @ (overline)
const byte cHash = A | D;					// # (||)
const byte cSlash = B | E | G;				// /
const byte cBSlash = D | F | G;				// Backslash
const byte cUnder = D;						// _
const byte cQuest = A | B | E | G;			// ?
const byte cCaret = A | B | F;				// ^

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
const byte cK = A | C | E | F | G;
const byte cL = D | E | F;
const byte cM = A | C | E;
const byte cN = A | B | C | E | F;
const byte cO = A | B | C | D | E | F;
const byte cP = A | B | E | F | G;
const byte cQ = A | B | D | F | G;
const byte cR = A | B | E | F;
const byte cS = A | C | D | F | G;
const byte cU = B | C | D | E | F;
const byte cV = B | D | F;
const byte cX = C | F;
const byte cW = B | D | F;

const byte cb = C | D | E | F | G;
const byte cc = D | E | G;
const byte cd = B | C | D | E | G;
const byte cg = A | B | C | D | F | G;
const byte ch = C | E | F | G;
const byte ci = C;
const byte cl = B | C;
const byte cm = C | E;
const byte cn = C | E | G;
const byte co = C | D | E | G;
const byte cq = A | B | C | F | G;
const byte cr = E | G;
const byte ct = D | E | F | G;
const byte cu = C | D | E;
const byte cy = B | C | D | F | G;

// Array links a value to its character

byte ascii[] = {
	cSpace, cNA, cQuote, cHash, cS, cNA, cNA, cAphos,
	cLBrck, cRBrck, cAster, cNA, cComma, cHyphen, cNA, cSlash,
	c0, c1, c2, c3, c4, c5, c6, c7,	c8, c9, cNA, cNA, cLBrck, cEqual, cRBrck, cQuest,
	cOverl, cA, cb, cC, cd, cE, cF, cG, cH, cI, cJ, cK, cL, cM, cN, cO,
	cP, cQ, cR, cS, ct, cU, cV, cW, cX, cy, c2, cLBrck, cBSlash, cRBrck, cCaret, cUnder,
	cBTick, cA, cb, cc, cd, cE, cF, cg, ch, ci, cJ, cK, cl, cm, cn, co,
	cP, cq, cr, cS, ct, cu, cV, cW, cX, cy, c2, cLBrck, cI, cRBrck, cHyphen, cNA
};

#pragma endregion --------------------------------------------------------------

#pragma region Setup -----------------------------------------------------------

void setup()
{
	Serial.begin(BAUD_RATE);
	Serial.println("Display started");

	Wire.begin(ADDRESS);
	Wire.onReceive(receiveEvent);

	// Initialize segment pins to OUTPUT, off

	for(int i = 0; i < SEGMENTS; i++) {
		pinMode(segArray[i], OUTPUT);
		digitalWrite(segArray[i], SEGOFF);
	}

	// Same for CACC pins

	for(int i = 0; i < DIGITS; i++) {
		pinMode(CACCpin[i], OUTPUT);
		digitalWrite(CACCpin[i], CACCOFF);
	}

	clearString();

	blankDisplay();

	// Initialize so first refresh will re-init everything

	segmentCounter = SEGMENTS - 1;	// Segments counter - set to end
	currentSegment = bit(SEGMENTS - 1); // Bit position of last segment

	prevMs = millis();

	// testAllChars();
}

#pragma endregion --------------------------------------------------------------

#pragma region Main loop -------------------------------------------------------

void loop()
{
	segmentDriver();

	switch(mode) {
		case i2cCommands::Flash:
			flash(flashPeriod);
			break;
		case i2cCommands::Rotate:
			rotate(rotatePeriod);
			break;
	}
}

#pragma endregion --------------------------------------------------------------

#pragma region Functions -------------------------------------------------------

void segmentDriver()
{
	currentMs = millis();

	if(currentMs != prevMs) {
		milliCount++;
		prevMs = currentMs;
	}

	if(milliCount == Refresh) {
		milliCount = 0;

		// Turn the current segment OFF while making changes - prevents ghosting

		digitalWrite(segArray[segmentCounter], SEGOFF);

		// This section selects the next segment

		currentSegment = currentSegment << 1;	// Shift left to next bit position
		segmentCounter++;						// Used as index into segArray

		if(segmentCounter == SEGMENTS) {		// If done with last segment, start over
			segmentCounter = 0; //re-initialize
			currentSegment = 1;
		}

		// This section turns the CA or CC pins on/off per the patterns of the
		// characters. If the currentSegment bit of the DIGIT[n] segment pattern
		// is 1, turn on the CACCpin[n]

		for(int i = 0; i < DIGITS; i++) {
			if(DIGIT[i] & currentSegment) {
				digitalWrite(CACCpin[i], CACCON);
			} else {
				analogWrite(CACCpin[i], CACCOFF);
			}
		}

		// Now turn the new segment driver ON

		digitalWrite(segArray[segmentCounter], SEGON);
	}
}

// Receives data from master
void receiveEvent(int nBytes)
{
	byte motor;
	char cmd[MAXCHARS];
	ulong ms = millis();

	for(int count = 0; count < MAXCHARS; count++) {
		cmd[count] = Wire.available() ? Wire.read() : '\x0';
	}

	if(ms <= timeout) {
		Serial.print("Not yet: ");
		Serial.print(ms);
		Serial.print(" / ");
		Serial.print(timeout);
		Serial.println("");
		return;
	} else {
		timeout = 0;
	}

	// Serial.println("Received");

	// Received commands

	switch((byte)cmd[0]) {

		case Ftmodules::SevenSegDisplay::cmdBlank:
			blankDisplay();
			break;

		case Ftmodules::SevenSegDisplay::cmdTest:
			testAllSegments();
			break;

		case Ftmodules::SevenSegDisplay::cmdDisplay:
			Serial.println(cmd + 1);
			displayString(cmd + 1);
			break;

		case Ftmodules::SevenSegDisplay::cmdHold:
			timeout = ms + (cmd[2] << 8) + (unsigned char)cmd[1];
			Serial.print("Set: ");
			Serial.print(ms);
			Serial.print(" / ");
			Serial.print(timeout);
			Serial.println(" -----------------");
			break;

		case Ftmodules::SevenSegDisplay::cmdFlash:
			flashPeriod = (cmd[2] << 8) + (unsigned char)cmd[1];
			mode = i2cCommands::Flash;
			break;

		case Ftmodules::SevenSegDisplay::cmdRotate:
			rotatePeriod = (cmd[2] << 8) + (unsigned char)cmd[1];
			mode = i2cCommands::Rotate;
			break;

		case Ftmodules::SevenSegDisplay::cmdStop:
			mode = i2cCommands::Default;
			break;

		default:
			// Command not found
			displayString("Err 01");
			break;
	}
}

// Sets all digits to blank
void blankDisplay()
{
	// if(currentMs <= timeout) {
	// 	return;
	// }

	for(int i = 0; i < DIGITS; i++) {
		DIGIT[i] = cSpace;
	}
}

void displayString(char *text)
{
	// if(currentMs <= timeout) {
	// 	return;
	// }

	textSize = strlen(text);
	clearString();
	strcpy(str, text);

	for(int i = 0; i < DIGITS; i++) {
		DIGIT[i] = i < textSize ? ascii[text[i] - 0x20] : cSpace;
	}
}

void flash(unsigned int animPeriod)
{
	// if(currentMs <= timeout) {
	// 	return;
	// }

	if(currentMs >= ms + animPeriod) {
		if(auxPos == LOW) {
			for(int i = 0; i < DIGITS; i++) {
				tempDigits[i] = DIGIT[i];
				DIGIT[i] = cSpace;
			}
		} else {
			for(int i = 0; i < DIGITS; i++) {
				DIGIT[i] = tempDigits[i];
			}
		}
		auxPos = auxPos ? LOW : HIGH;
		ms = currentMs;
	}
}

void rotate(unsigned int rotatePeriod)
{
	// if(currentMs <= timeout) {
	// 	return;
	// }

	if(currentMs >= ms + rotatePeriod) {
		for(int i = 0; i < DIGITS; i++) {
			byte offset = auxPos < textSize - i ? auxPos + i : auxPos - textSize + i;
			DIGIT[i] = ascii[str[offset] - 0x20];
		}
		auxPos = auxPos < textSize - 1 ? auxPos + 1 : 0;
		ms = currentMs;
	}
}

void clearString()
{
	for(int i = 0; i < MAXCHARS; i++) {
		str[i] = '\x0';
	}
}

#pragma endregion --------------------------------------------------------------

#pragma region Test functions --------------------------------------------------

void testAllSegments()
{
	for(int i = 0; i < DIGITS; i++) {
		DIGIT[i] = c8;
	}
}

void testAllChars()
{
	displayString(
		// "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG "
		// "PACK MY BOX WITH FIVE DOZEN LIQUOR JUGS "
		"SPHINX OF BLACK QUARTZ, JUDGE MY VOW "
	);
	rotatePeriod = 200;
	mode = i2cCommands::Rotate;
}

#pragma endregion --------------------------------------------------------------
