function toggleOnOff () {
    on = 1 - on
    showData()
}
function Change_speed () {
    if (speed == 255) {
        speed = 192
    } else if (speed == 192) {
        speed = 128
    } else if (speed == 128) {
        speed = 64
    } else {
        speed = 255
    }
    makerbit.showStringOnLcd1602("" + (speed), makerbit.position1602(LcdPosition1602.Pos14), 3)
}
input.onButtonPressed(Button.A, function () {
    fischertechnikTests.sendMotorCommand("Sp", 0, 255)
    fischertechnikTests.sendMotorCommand("Sp", 1, 255)
})
function init () {
    led.enable(false)
    motorDriver1Address = 8
    ultrasoundSensorAddress = 10
    LCDAddress = 39
    on = 0
    speed = 255
    makerbit.connectLcd(LCDAddress)
    makerbit.showStringOnLcd1602("Speed", makerbit.position1602(LcdPosition1602.Pos8), 5)
    makerbit.showStringOnLcd1602("A       B", makerbit.position1602(LcdPosition1602.Pos17), 9)
    showData()
    fischertechnikTests.init(motorDriver1Address)
    fischertechnikTests.sendMotorCommand("Md", 0, 1)
    fischertechnikTests.sendMotorCommand("Md", 1, 1)
}
function showData () {
    fischertechnikTests.readData(motorDriver1Address)
    makerbit.showStringOnLcd1602("" + (on), makerbit.position1602(LcdPosition1602.Pos1), 1)
    makerbit.showStringOnLcd1602("" + (speed), makerbit.position1602(LcdPosition1602.Pos14), 3)
    makerbit.showStringOnLcd1602("" + (fischertechnikTests.pulses(0)), makerbit.position1602(LcdPosition1602.Pos19), 5)
    makerbit.showStringOnLcd1602("" + (fischertechnikTests.pulses(1)), makerbit.position1602(LcdPosition1602.Pos27), 5)
}
input.onButtonPressed(Button.B, function () {
    Change_speed()
})
pins.onPulsed(DigitalPin.P12, PulseValue.Low, function () {
    fischertechnikTests.readData(motorDriver1Address)
    if (fischertechnikTests.command().compare("CCW") == 0) {
        basic.pause(100)
        fischertechnikTests.sendMotorCommand("Sp", 0, 255)
    }
})
let LCDAddress = 0
let ultrasoundSensorAddress = 0
let motorDriver1Address = 0
let speed = 0
let on = 0
init()
basic.forever(function () {
	
})
