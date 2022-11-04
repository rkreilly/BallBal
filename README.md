# BallBal
3185 PRJ


ADD PINOUTS, ETC

// Touchscreen board:
    // Starting on Vlogic side:
    // int -                                  interrupt pin. Not needed yet but will if wewant the touch detection! (NOT REQ FOR PRJ)
    // sda - (i2c0 sda = gpio0) = pin1
    // scl - (i2c0 scl = gpio1) = pin2
    // gnd - gnd = pin 3                      GND also pins 8,13,18,23,28,38
    // 3Vo -                                  Not gonna use this one
    // Vin - 3v3out = pin36



//MOTOR
    // MotorX pwm is gpio 14 = pin19
    // MotorY pwm is gpio 15 = pin20
    // these will connect to the 'signal', or yellow cords for each motor. 
    // Both BKs go to (-) on the sideboard
    // Both RDs go to (+) on the sideboard
