#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/timer.h>

#define SCRN_ADD 0b1001000      // 48 in hex, 72 in dec
#define i2cSCLpin 1
#define i2cSDApin 0
#define MOTORX_PIN 14
#define MOTORY_PIN 15
#define MMIN 3927
#define MMAX 5891
#define MCENT 4909

volatile int xLoc = 2047;
volatile int yLoc = 2047;

void setup()
{
    stdio_init_all();

    gpio_set_function(i2cSCLpin, GPIO_FUNC_I2C);
    gpio_set_function(i2cSDApin, GPIO_FUNC_I2C);
    gpio_pull_up(i2cSCLpin);
    gpio_pull_up(i2cSDApin);
    i2c_init(i2c0, 100000);
    
    gpio_init(MOTORX_PIN);
    gpio_init(MOTORY_PIN);
    gpio_set_dir(MOTORX_PIN, false);
    gpio_set_dir(MOTORY_PIN, false);
    gpio_pull_down(MOTORX_PIN);
    gpio_pull_down(MOTORY_PIN);
    gpio_set_function(MOTORX_PIN, GPIO_FUNC_PWM);
    gpio_set_function(MOTORY_PIN, GPIO_FUNC_PWM);

    uint sliceX_num = pwm_gpio_to_slice_num(MOTORX_PIN);
    uint sliceY_num = pwm_gpio_to_slice_num(MOTORY_PIN);
	uint chanX_num = pwm_gpio_to_channel(MOTORX_PIN);
    uint chanY_num = pwm_gpio_to_channel(MOTORY_PIN);
	pwm_set_clkdiv_int_frac(sliceX_num, 38, 3);
    pwm_set_clkdiv_int_frac(sliceY_num, 38, 3);
	pwm_set_wrap(sliceX_num, 6873);
    pwm_set_wrap(sliceY_num, 6873);
	pwm_set_enabled(sliceX_num, true);
    pwm_set_enabled(sliceY_num, true);
    pwm_set_gpio_level(MOTORX_PIN, 4909);
    pwm_set_gpio_level(MOTORY_PIN, 4909);
        
    sleep_ms(1000);
}

void motorControl(){

//MOTOR STUFF
    // so if we have 45° min/max, over a 4095 range of screen
    // we'll need to convert size of tau to size of motor-screen angle somhow. ±2047u = ±45° = ±928
    //  = 2.21 multiplier
    // FOR NOW, just 1:1 tau is how much we'll move the motors. 

    volatile float kpx=0, kix=0, kdx=0, ex=0, edotx=0, eoldx=0, eintx=0, taux=0, posxDes=2047, dt=0.02;
    volatile float kpy=0, kiy=0, kdy=0, ey=0, edoty=0, eoldy=0, einty=0, tauy=0, posyDes=2047,

    ex = posxDes - xLoc;
    edotx = (ex - eoldx) / dt;
    eintx += eintx + ex*dt;
    taux = kpx*ex + kdx*edotx + kix*eintx;
    eoldx = ex;

    ey= posyDes - yLoc;
    edoty = (ey - eoldy) / dt;
    einty += einty + ey*dt;
    tauy = kpy*ey + kdy*edoty + kiy*einty;
    eoldy = ey;

    if(eintx > 511){
        eintx = 511;
    }
    if(eintx < -511){
        eintx = -511;
    }
    if(einty > 511){
        einty = 511;
    }
    if(einty < -511){
        einty = -511;
    }

    // VERIFY POS/NEG compared to the screen axes!
    
    if(taux > 0){
        if(taux > 2047){
            taux = 2047 / 2.21;
        }else{
            taux = taux / 2.21;
        }
        pwm_set_gpio_level(MOTORX_PIN, 4909 - taux);
    }else if(taux < 0){
        if(taux < -2047){
            taux = -2047 / 2.21;
        }else{
            taux = taux / 2.21;
        }
        pwm_set_gpio_level(MOTORX_PIN, 4909 + taux);
    }else{
        pwm_set_gpio_level(MOTORX_PIN, 4909);
    }
    
    if(tauy > 0){
        if(tauy > 2047){
            tauy = 2047 / 2.21;
        }else{
            tauy = tauy / 2.21;
        }
        pwm_set_gpio_level(MOTORY_PIN, 4909 - tauy);
    }else if(tauy < 0){
        if(tauy < -2047){
            tauy = 2047 / 2.21;
        }else{
            tauy = tauy / 2.21;
        }
        pwm_set_gpio_level(MOTORY_PIN, 4909 + tauy);
    }else{
        pwm_set_gpio_level(MOTORY_PIN, 4909);
    }
}


int loop()
{

//TOUCHSCREEN SIDE
    

    uint8_t recData[2] = {0b00000000, 0b00000000};
    uint8_t recData2[2] = {0b00000000, 0b00000000};
    uint8_t sndData[1] = {0b11000100};
    uint8_t sndData2[1] = {0b11010100};

    i2c_write_blocking(i2c0, SCRN_ADD, sndData, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recData, 2, false);
    xLoc= recData[0] << 4 | recData[1] >> 4;

    i2c_write_blocking(i2c0, SCRN_ADD, sndData2, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recData2, 2, false);
    yLoc= recData2[0] << 4 | recData2[1] >> 4;

    // printf("X: %u\r\n", xLoc);
    // printf("Y: %u\r\n", yLoc);
    // printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData[0], recData[0], recData[1]);
    // printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData2[0], recData2[0], recData2[1]);
    sleep_ms(200);

    return 1;
    
}

int main()
{
    setup();
    int loopcount= 0;
    while (true){
        loop();
        if(loopcount>10){
            printf("X: %u\r\n", xLoc);
            printf("Y: %u\r\n", yLoc);
            loopcount= 0;
        }
        loopcount++;
    }

}

//Commands
    // 0 or 01000000 for temp sensors
    // 11000100 for x-pos
    // 11010100 for y-pos
    // address msb-10010 A1 A0
    // 0x48, a1 and a0 are open. REQ SOLDERING to close/set differently. 
    // so screen address is 1001000. 

//Pins
    // Starting on Vlogic side:
    // int - interrupt pin. Not needed yet but will when wewant the touch detection!
    // sda - i2c0 sda = gpio0 = pin1
    // scl - i2c0 scl = gpio1 = pin2
    // gnd - gnd = pin 3,8,13,18,23,28,38
    // 3Vo - 
    // Vin - 3v3out = pin36
    // 
    // MotorX pwm is gpio 14 = pin19
    // MotorY pwm is gpio 15 = pin20


//Motor
    // 3-5V peak to peak sq wave
    // pulse is from 0.9 ms to 2.1 ms, 1.5 ms center.
    // pulse is at 50 Hz.
    // takes 4.8-6V input
    // BK- gnd; R- pwr; Y- signal

    // 4909 is 1.5ms is 0-center. 
    // 21.8222 levels/degree
    // ±928 is ±45°
    // 3927-5891
    // 6873 is our max 90°
    // 2945 is then the min -90°


//Clock
/*  DIV 38 + 3
    Res: .3055 us
    TOP: 6873
    cc: (900)2945 - (2100)6873
    1/2: 4909
    


*/


/*  TEMP stuff pulled out for conciseness
    uint8_t recTemp[2] = {0b00000000, 0b00000000};
    uint8_t recTemp2[2] = {0b00000000, 0b00000000};
    int T1= 0;
    int T2= 0;
    double T=0;
    double Tf=0;

    uint8_t sndTemp[1] = {0b00000000};
    i2c_write_blocking(i2c0, SCRN_ADD, sndTemp, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recTemp, 2, false);
    printf("%g ;;;; %g ;;;;;;\r\n", recTemp[0], recTemp[1]);
    T1= recTemp[0] << 4 | recTemp[1] >> 4;
    
    uint8_t sndTemp2[1] = {0b01000000};
    i2c_write_blocking(i2c0, SCRN_ADD, sndTemp2, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recTemp2, 2, false);
    printf("%g ;;;; %g ;;;;;;\r\n", recTemp2[0], recTemp2[1]);
    T2= recTemp2[0] << 4 | recTemp2[1] >> 4;
    
    T= 2.573*(T2-T1);
    Tf= (T-273)*9/5 + 32;
    printf("T1: %g ; T2: %g \r\n", T1, T2);
    printf("Temp: %g K   Temp: %g °F \r\n", T, Tf);
    */

    // Something ain't right. both recTemp are filling 0 in first, and same value in second.
    // Nothing in manual about how to determine ref temp for sensor 1 reading, 
    // but diffTemp for sensor 2 reading isn;t working because T1 is zero...?
    // So we get 220 recTemp2[1], is 370K = 210F. at room temp. wtf. 
    // Very odd because T1 is mathing to zero, while T2 maths to ~220, even though both are
    // pulling the same values from their respective recTemp arrays... 


