/**
 * Use this file to define custom functions and blocks.
 * Read more at https://makecode.microbit.org/blocks/custom
 */

/* TODO:

- Create preset speeds (1-8?)
- Higher-level commands: instead of sendCommand(ard1, "Mt", 0); sendCommand(ard1, "Fw", null), use Forward(motorNumber),
    where motorNumber also incudes the Arduino address (e.g. 8).

*/

// https://ubershmekel.github.io/fourcc-to-text/

enum Commands {
    //% block="Backwards"
    Backwards = 0x7742,  // 'wB'
    //% block="Brake"
    Brake = 0x7242,      // 'rB'
    //% block="Coast"
    Coast = 0x6f43,      // 'oC'
    //% block="Forward"
    Forward = 0x7746,    // 'wF'
    //% block="Goal"
    Goal = 0x6f47,       // 'oG'
    //% block="Home"
    Home = 0x6f48,       // 'oH'
    //% block="Hello"
    Hello = 0x6948,      // 'iH'
    //% block="Mode"
    Mode = 0x644d,       // 'dM'
    //% block="Speed"
    Speed = 0x7053,      // 'pS'
};

enum Motors {
    //% block="Motor 1"
    MOTOR_1 = 0,
    //% block="Motor 2"
    MOTOR_2,
    //% block="Motor 3"
    MOTOR_3,
    //% block="Motor 4"
    MOTOR_4,
};

/**
 * Custom blocks
 */

//% block="ft/RP" weight=100 color=#ff8000 icon="\uf1b2"
namespace fischertechnikTests
{
    // let _cmd: Commands = Commands
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
        // _cmd = Commands.COAST;
        _pulses0 = 0;
        _pulses1 = 0;
        _motorDriver1 = motorDriver1;
    }

    /**
     * Reads data from a module via I²C.
     * @param address Target module address, eg: 8
     */
    //% block
    export function readData(address: number)
    {
        let value = pins.i2cReadNumber(address, NumberFormat.UInt32LE, false);
        // BACA
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
     * @param cmd Command, eg: Commands.Speed
     * @param Motors Motor, eg: Motors.MOTOR_1
     * @param param Parameters, eg: 0
     */
    //% block
    export function sendMotorCommand(cmd: Commands, motor: Motors = Motors.MOTOR_1, param: number = 0)
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
     * @param cmd Command, eg: Commands.Speed
     * @param param8 8-bit parameter number, eg: 0
     * @param param16 16-bit parameter, eg: 0
     */
    //% block
    export function sendCommand(address: number, cmd: Commands, param8: number = 0, param16: number = 0)
    {
        let bufr = pins.createBuffer(8);

        bufr.setNumber(NumberFormat.Int16LE, 0, cmd as number);
        bufr.setNumber(NumberFormat.Int8LE, 2, param8);
        bufr.setNumber(NumberFormat.Int16LE, 3, param16);

        pins.i2cWriteBuffer(address, bufr);
    }

    /**
     * Gets last pulse count from Arduino.
     * @param Motors Motor, eg: Motors.MOTOR_1
     */
    //% block
    export function pulses(motor: Motors): number
    {
        return motor == Motors.MOTOR_1 ? _pulses0 : _pulses1;
    }
}
