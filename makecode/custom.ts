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
    let _motorDriver1: number;

    /**
     * Initialization
     * @param motorDriver1 I²C motor driver 1 address, eg: 8
     */
    //% block
    export function init(motorDriver1: number)
    {
        _cmd = Commands.COAST;
        _pulses0 = 0;
        _pulses1 = 0;
        _motorDriver1 = motorDriver1;
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
     * Reads a single integer from Arduino board via I²C.
     * @param arduino Arduino board address, eg: 10
     */
    //% block
    export function readInt(arduino: number)
    {
        let value = pins.i2cReadNumber(arduino, NumberFormat.UInt16LE, false);
        return value;
    }

    /**
     * Sends a command to the motor drive module.
     * @param cmd Command, eg: "Sp"
     * @param motor Motor number, eg: 0
     * @param param Parameters, eg: 0
     */
    //% block
    export function sendMotorCommand(cmd: string, motor: number = 0, param: number = 0)
    {
        if(_motorDriver1) {
            sendCommand(_motorDriver1, cmd, motor, param);
        } else {
            console.log('_motorDriver1 not defined');
        }
    }

    /**
     * Sends a command to a module via I²C.
     * @param address Target module address, eg: 8
     * @param cmd Command, eg: "Sp"
     * @param motor Motor number, eg: 0
     * @param param Parameters, eg: 0
     */
    //% block
    export function sendCommand(address: number, cmd: string, motor: number = 0, param: number = 0)
    {
        let bufr = pins.createBuffer(8);
        bufr.setNumber(NumberFormat.Int16LE, 0, cmd.charCodeAt(0) | cmd.charCodeAt(1) << 8);
        bufr.setNumber(NumberFormat.Int8LE, 2, motor);
        bufr.setNumber(NumberFormat.Int16LE, 3, param);


        pins.i2cWriteBuffer(_motorDriver1, bufr);


        // if(param !== null) {
        // bufr.setNumber(NumberFormat.Int8LE, 0, cmd.charCodeAt(0));
        // bufr.setNumber(NumberFormat.Int8LE, 1, cmd.charCodeAt(1));
        // pins.i2cWriteNumber(
        //     arduino,
        //     cmd.charCodeAt(0) | cmd.charCodeAt(1) << 8 | (motor & 0x01) << 12 | (param & 0xffff) << 16,
        //     NumberFormat.UInt32LE,
        //     false
        // )
        // } else {
        //     pins.i2cWriteNumber(
        //         arduino,
        //         str.charCodeAt(0) | str.charCodeAt(1) << 8,
        //         NumberFormat.UInt16LE,
        //         false
        //     )
        // }
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
}
