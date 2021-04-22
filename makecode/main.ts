input.onButtonPressed(Button.A, function () {
    fischertechnikTests.sendCommand(motorDriver1, "Mt", 1)
    fischertechnikTests.sendCommand(motorDriver1, "Md", 1)
    fischertechnikTests.sendCommand(motorDriver1, "Fw", null)
})
function init () {
    led.enable(false)
    pins.setPull(DigitalPin.P12, PinPullMode.PullUp)
    makerbit.connectLcd(LCDAddress)
    makerbit.showStringOnLcd1602("Hello  speed", makerbit.position1602(LcdPosition1602.Pos1), 16)
    fischertechnikTests.sendCommand(motorDriver1, "Hi", null)
    showData()
}
function showData () {
    fischertechnikTests.readData(motorDriver1)
    makerbit.showStringOnLcd1602("" + (fischertechnikTests.pulses()), makerbit.position1602(LcdPosition1602.Pos17), 4)
    makerbit.showStringOnLcd1602(fischertechnikTests.command(), makerbit.position1602(LcdPosition1602.Pos22), 7)
}
input.onButtonPressed(Button.B, function () {
    Set_speed(0, 0)
})
function Set_speed (newSpeed: number, motor: number) {
    if (newSpeed > 0) {
        Speed = newSpeed
    } else {
        if (Speed > 63) {
            Speed += -64
        } else {
            Speed = 255
        }
    }
    selectedMotor = fischertechnikTests.currentMotor()
    fischertechnikTests.sendCommand(motorDriver1, "Mt", motor)
    fischertechnikTests.sendCommand(motorDriver1, "Sp", Speed)
    fischertechnikTests.sendCommand(motorDriver1, "Mt", fischertechnikTests.currentMotor())
    makerbit.showStringOnLcd1602("" + (Speed), makerbit.position1602(LcdPosition1602.Pos14), 3)
}
pins.onPulsed(DigitalPin.P12, PulseValue.Low, function () {
    fischertechnikTests.readData(motorDriver1)
    if (fischertechnikTests.command().compare("CCW") == 0) {
        basic.pause(100)
        fischertechnikTests.sendCommand(motorDriver1, "Ho", null)
        waitForLimitSwitch = true
    }
})
let distance = 0
let waitForLimitSwitch = false
let selectedMotor = 0
let Speed = 0
let LCDAddress = 0
let motorDriver1 = 0
motorDriver1 = 8
let ultrasoundSensor = 10
LCDAddress = 39
init()
fischertechnikTests.sendCommand(motorDriver1, "Go", 400)
basic.forever(function () {
    let ultrasoundTest = 0
    if (ultrasoundTest) {
        distance = fischertechnikTests.readInt(10)
        if (distance < 20) {
            fischertechnikTests.sendCommand(motorDriver1, "Fw", null)
        }
        makerbit.showStringOnLcd1602("" + (distance), makerbit.position1602(LcdPosition1602.Pos30), 3)
        basic.pause(100)
    }
})
