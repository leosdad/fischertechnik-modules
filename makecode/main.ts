input.onButtonPressed(Button.A, function () {
    fischertechnikTests.sendMotorCommand(Commands.Mode, Motors.MOTOR_1, 1)
    fischertechnikTests.sendMotorCommand(Commands.Goal, Motors.MOTOR_1, 300)
    fischertechnikTests.sendMotorCommand(Commands.Forward, Motors.MOTOR_1, 0)
    on = 1
    status = "forward"
    basic.showLeds(`
        . . # . .
        . . . # .
        # # # # #
        . . . # .
        . . # . .
        `)
})
function changeSpeed () {
    if (speed == 255) {
        speed = 192
    } else if (speed == 192) {
        speed = 128
    } else if (speed == 128) {
        speed = 64
    } else {
        speed = 255
    }
    fischertechnikTests.sendMotorCommand(Commands.Speed, Motors.MOTOR_1, speed)
    basic.showNumber(Math.round(speed / 64))
}
function init () {
    pins.setPull(DigitalPin.P16, PinPullMode.PullUp)
    motorDriver1Address = 8
    ultrasoundSensorAddress = 10
    LCDAddress = 39
    on = 0
    status = "idle"
    speed = 255
    fischertechnikTests.init(motorDriver1Address)
    fischertechnikTests.sendMotorCommand(Commands.Hello, Motors.MOTOR_1, 0)
    fischertechnikTests.sendMotorCommand(Commands.Speed, Motors.MOTOR_1, speed)
    fischertechnikTests.sendMotorCommand(Commands.Speed, Motors.MOTOR_2, speed)
}
input.onButtonPressed(Button.B, function () {
    changeSpeed()
})
pins.onPulsed(DigitalPin.P16, PulseValue.Low, function () {
    if (status == "forward") {
        basic.showLeds(`
            . . # . .
            . # . . .
            # # # # #
            . # . . .
            . . # . .
            `)
        basic.pause(80)
        fischertechnikTests.sendMotorCommand(Commands.Home, Motors.MOTOR_1, 0)
        on = 0
        status = "backwards"
    } else if (status == "backwards") {
        basic.clearScreen()
        basic.showIcon(IconNames.Happy)
    }
})
let LCDAddress = 0
let ultrasoundSensorAddress = 0
let motorDriver1Address = 0
let speed = 0
let on = 0
let status = ""
status = ""
init()
basic.showIcon(IconNames.Yes)
basic.forever(function () {
	
})
