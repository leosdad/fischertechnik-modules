/**
 * Use this file to define custom functions and blocks.
 * Read more at https://makecode.microbit.org/blocks/custom
 */

/* TODO:

- Create preset speeds (1-8?)
- Higher-level commands: instead of sendCommand(ard1, "Mt", 0); sendCommand(ard1, "Fw", null), use Forward(motorNumber),
    where motorNumber also incudes the Arduino address (e.g. 8).

*/

enum Commands {
    //% block="Coast"
    COAST = 0,
    //% block="Forward"
    FORWARD,
    //% block="Backwards"
    BACKWARDS,
    //% block="Home"
    HOME,
    //% block="Counterclockwise"
    CCW,
    //% block="Clockwise"
    CW,
    //% block="Go home"
    GOHOME,
    //% block="Brake"
    BRAKE,
    //% block="Reset"
    RESET,
};

/**
 * Custom blocks
 */

//% block="ft/RP" weight=100 color=#ff8000 icon="\uf1b2"
namespace fischertechnikTests
{
    let _cmd: Commands = Commands.COAST;
    let _pulses0: number;
    let _pulses1: number;
    // let _motor: number;

    /**
     * Initialization
     */
    //% block
    export function init()
    {
        _cmd = Commands.COAST;
        _pulses0 = 0;
        _pulses1 = 0;
    }

    /**
     * Reads data from Arduino board via I2C.
     * @param arduino Arduino board address, eg: 8
     */
    //% block
    export function readData(arduino: number)
    {
        let value = pins.i2cReadNumber(arduino, NumberFormat.UInt32LE, false);
        _pulses0 = value & 0xFFFF;
        _pulses1 = (value & 0xFFFF0000) >> 16;
    }

    /**
     * Reads a single integer from Arduino board via I2C.
     * @param arduino Arduino board address, eg: 10
     */
    //% block
    export function readInt(arduino: number)
    {
        let value = pins.i2cReadNumber(arduino, NumberFormat.UInt16LE, false);
        return value;
    }

    /**
     * Sends a command to Arduino via I2C.
     * @param arduino Arduino board address, eg: 8
     * @param str Arduino command, eg: "St"
     * @param param Parameters, eg: null
     */
    //% block
    export function sendCommand(arduino: number, str: string, param: number = null)
    {
        if(param !== null) {
            pins.i2cWriteNumber(
                arduino,
                str.charCodeAt(0) | str.charCodeAt(1) << 8 | (param & 0xffff) << 16,
                NumberFormat.UInt32LE,
                false
            )
        // } else if(str.length > 2) {
        //     pins.i2cWriteNumber(
        //         arduino,
        //         str.charCodeAt(0) | str.charCodeAt(1) << 8 | str.charCodeAt(2) << 16 | str.charCodeAt(3) << 24,
        //         NumberFormat.UInt32LE,
        //         false
        //     )
        } else {
            pins.i2cWriteNumber(
                arduino,
                str.charCodeAt(0) | str.charCodeAt(1) << 8,
                NumberFormat.UInt16LE,
                false
            )
        }
    }

    /**
     * Gets last command from Arduino.
     */
    //% block
    export function command(): string
    {
        switch(_cmd) {
            case Commands.COAST:
                return "Idle";
            case Commands.FORWARD:
                return "Forward";
            case Commands.BACKWARDS:
                return "Back";
            case Commands.HOME:
                return "Home";
            case Commands.CCW:
                return "CCW";
            case Commands.CW:
                return "CW";
            case Commands.GOHOME:
                return "Go home";
            case Commands.BRAKE:
                return "Brake";
            case Commands.RESET:
                return "Reset";
            default:
                return "[?]";
        }
        // return cmd as Commands;
    }

    /**
     * Gets last pulse count from Arduino.
     */
    //% block
    export function pulses(motor: number): number
    {
        return motor == 1 ? _pulses1 : _pulses0;
    }

    /**
     * Gets the number of the current motor.
     */
    //% block
    // export function currentMotor(): number
    // {
    //     return _motor;
    // }
}
