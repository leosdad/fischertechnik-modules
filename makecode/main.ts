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
    toggleOnOff()
})
function demo () {
    if (on) {
        fischertechnikTests.sendCommand(motorDriver1Address, "Sp", speed)
        fischertechnikTests.sendCommand(motorDriver1Address, "Mt", selectedMotor)
        fischertechnikTests.sendCommand(motorDriver1Address, "Fw", 1)
        basic.pause(500)
        fischertechnikTests.sendCommand(motorDriver1Address, "Br", 1)
        basic.pause(500)
        fischertechnikTests.sendCommand(motorDriver1Address, "Bw", 1)
        basic.pause(500)
        fischertechnikTests.sendCommand(motorDriver1Address, "Co", 1)
        basic.pause(500)
        selectedMotor = 1 - selectedMotor
        showData()
    }
}
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
    fischertechnikTests.init()
    showData()
    fischertechnikTests.sendCommand(motorDriver1Address, "Hi", null)
    fischertechnikTests.sendCommand(motorDriver1Address, "Mt", 1)
    fischertechnikTests.sendCommand(motorDriver1Address, "Sp", speed)
    fischertechnikTests.sendCommand(motorDriver1Address, "Mt", 0)
    fischertechnikTests.sendCommand(motorDriver1Address, "Sp", speed)
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
        fischertechnikTests.sendCommand(motorDriver1Address, "Ho", null)
        waitForLimitSwitch = true
    }
})
let waitForLimitSwitch = false
let LCDAddress = 0
let ultrasoundSensorAddress = 0
let selectedMotor = 0
let motorDriver1Address = 0
let speed = 0
let on = 0
init()
basic.forever(function () {
    demo()
})
