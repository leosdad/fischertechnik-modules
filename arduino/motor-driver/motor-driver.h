// -----------------------------------------------------------------------------

// fischertechnik driver for DC motors with encoder and servos
// Rubem Pechansky

// -----------------------------------------------------------------------------

// I2C commands

#define cmdHello		0x21

#define cmdOutput1		0x31
#define cmdOutput2		0x32
#define cmdOutput3		0x33
#define cmdOutput4		0x34

// #define cmdDigitalRead	0x31
// #define cmdAnalogRead	0x32
// #define cmdDigitalWrite	0x33
// #define cmdAnalogWrite	0x34

#define cmdMode			0x40
#define cmdSpeed		0x41
#define cmdCoast		0x42
#define cmdForward		0x43
#define cmdBackwards	0x44
#define cmdBrake		0x45
#define cmdHome			0x46
#define cmdTarget		0x49

// -----------------------------------------------------------------------------
