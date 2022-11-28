#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

#define btTX 0
#define btRX 1

#define TOUCH_ALARM_NUM 0
#define TOUCH_IRQ TIMER_IRQ_0
#define TOUCH_CALLBACK_TIME 5000
#define CONTROL_ALARM_NUM 1
#define CONTROL_IRQ TIMER_IRQ_1
#define CONTROL_CALLBACK_TIME 20000

#define SCRN_ADD 0b1001000      // 48 in hex, 72 in dec
#define i2cSCLpin 9
#define i2cSDApin 8
#define MOTORX_PIN 14
#define MOTORY_PIN 15
#define MMIN 3927
#define MMAX 5891
#define MCENT 4909

volatile int xLoc = 2047;
volatile int yLoc = 2047;
volatile float dt=0.02, kpx=40, kix=5, kdx=15, ex=0, edotx=0, eoldx=0, eintx=0, taux=0, posxDes=2047;
volatile float kpy=40, kiy=5, kdy=15, ey=0, edoty=0, eoldy=0, einty=0, tauy=0, posyDes=2047;
// for e=cm reading and tau=ms (cc out), try p=40, d=10, i=2
// maybe try x400 since we're reading in pixels. 
// or perhaps /400, who really knows. 

void touchyFeely(){

    hw_clear_bits(&timer_hw->intr, 1u << TOUCH_ALARM_NUM);
    timer_hw->alarm[TOUCH_ALARM_NUM] += TOUCH_CALLBACK_TIME;

    uint8_t recData[2] = {0b00000000, 0b00000000};
    uint8_t recData2[2] = {0b00000000, 0b00000000};
    uint8_t sndData[1] = {0b11000000};
    uint8_t sndData2[1] = {0b11010000};

    i2c_write_blocking(i2c0, SCRN_ADD, sndData, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recData, 2, false);
    xLoc= recData[0] << 4 | recData[1] >> 4;

    i2c_write_blocking(i2c0, SCRN_ADD, sndData2, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recData2, 2, false);
    yLoc= recData2[0] << 4 | recData2[1] >> 4;

    // printf("X: %u\r\n", xLoc);
     //printf("Y: %u\r\n", yLoc);
     //printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData[0], recData[0], recData[1]);
     //printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData2[0], recData2[0], recData2[1]);

}


void pushyShovey(){

//MOTOR STUFF
    // so if we have 45° min/max, over a 4095 range of screen
    // we'll need to convert size of tau to size of motor-screen angle somhow. ±2047u = ±45° = ±928
    //  = 2.21 multiplier
    // FOR NOW, just 1:1 tau is how much we'll move the motors. 

    hw_clear_bits(&timer_hw->intr, 1u << CONTROL_ALARM_NUM);
    timer_hw->alarm[CONTROL_ALARM_NUM] += CONTROL_CALLBACK_TIME;

    ex = posxDes - xLoc;
    edotx = (ex - eoldx) / dt;
    eintx += ex*dt;
    taux = kpx*ex + kdx*edotx + kix*eintx;
    eoldx = ex;

    ey= posyDes - yLoc;
    edoty = (ey - eoldy) / dt;
    einty += ey*dt;
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
    // add caps on motor movement to 45°
    // set kp/kd/ki numbers to >0

    if(taux > 0){
        if(taux > 2047){
            taux = 2047 / 2.21;
        }else{
            taux = taux / 2.21;
        }
        pwm_set_gpio_level(MOTORX_PIN, 4909 + taux);
    }else if(taux < 0){
        if(taux < -2047){
            taux = 2047 / 2.21;
        }else{
            taux = taux / 2.21;
        }
        pwm_set_gpio_level(MOTORX_PIN, 4909 - taux);
    }else{
        //pwm_set_gpio_level(MOTORX_PIN, MCENT);
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
        //pwm_set_gpio_level(MOTORY_PIN, MCENT);
    }
}
 void uartFlush()
 {
 while(uart_is_readable_within_us(uart0,1000))
    { 
        char c = uart_getc(uart0);
        printf("%c",c);
    }
 }

void setup()
{
    stdio_init_all();

    //sleep_ms(20000);
    uart_init(uart0, 38400);
    gpio_set_function(btTX, GPIO_FUNC_UART);
    gpio_set_function(btRX, GPIO_FUNC_UART);
//    for (int i = 0; i<10; ++i)
//    {
//         printf("%d",i);
//         uart_puts(uart0, "AT\r\n");
//         uartFlush();
//         sleep_ms(1000);
//    }
   //uart_puts(uart0, "AT+ORGL\r\n");
        //uartFlush();
        //sleep_ms(1000);
        // uart_puts(uart0, "AT+NAME=Ballin\r\n");
        // uartFlush();
        // sleep_ms(1000);
        // uart_puts(uart0, "AT+PSWD=4444\r\n");
        // uartFlush();
        // sleep_ms(1000);
        // uart_puts(uart0, "AT+UART=38400,0,0\r\n");
        // uartFlush();
        // sleep_ms(1000);
   

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

   
    pwm_set_gpio_level(MOTORX_PIN, 5900);
   
    pwm_set_gpio_level(MOTORY_PIN, 5900);
   
    pwm_set_gpio_level(MOTORX_PIN, 3900);
    
    pwm_set_gpio_level(MOTORY_PIN, 3900);
   
    pwm_set_gpio_level(MOTORX_PIN, 4909);
    pwm_set_gpio_level(MOTORY_PIN, 4909);
    

    irq_set_exclusive_handler(TOUCH_IRQ, touchyFeely);
    hw_set_bits(&timer_hw->inte, 1u << TOUCH_ALARM_NUM);
    irq_set_enabled(TOUCH_IRQ, true);
    irq_set_exclusive_handler(CONTROL_IRQ, pushyShovey);
    hw_set_bits(&timer_hw->inte, 1u << CONTROL_ALARM_NUM);
    irq_set_enabled(CONTROL_IRQ, true);
    uint t = time_us_32();
    timer_hw->alarm[TOUCH_ALARM_NUM] = t + 5000;
    timer_hw->alarm[CONTROL_ALARM_NUM] = t + 2500 + CONTROL_CALLBACK_TIME;

}



int loop()
{
   char c = uart_getc(uart0);
       printf("%c", c);
    return 1;   
}


 int main()
 {
     setup();    
    int loopcount= 0;
     while (true){
         loop();
            if(loopcount>200){
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
