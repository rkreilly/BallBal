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

volatile int loopcount = 0;
volatile int xLoc = 2047;
volatile int yLoc = 2047;
volatile float dt=0.02, kpx=45/40.0, kix=8/40.0, kdx=8/40.0, ex=0, edotx=0, eoldx=0, eintx=0, taux=0, posxDes=2047;
volatile float kpy=45/40.0, kiy=8/40.0, kdy=8/40.0, ey=0, edoty=0, eoldy=0, einty=0, tauy=0, posyDes=2047;
// for e=cm reading and tau=ms (cc out)


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

    if (
        xLoc < 1){xLoc = 2047;}
        else {xLoc = xLoc;}

    i2c_write_blocking(i2c0, SCRN_ADD, sndData2, 1, false);
    i2c_read_blocking(i2c0, SCRN_ADD, recData2, 2, false);
    yLoc= (recData2[0] << 4 | recData2[1] >> 4);

    if (
        yLoc > 4094){yLoc = 2047;}
        else {yLoc = yLoc;}


   // printf("X: %u\r\n", xLoc);
    //printf("Y: %u\r\n", yLoc);
    
    
   
    
     //printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData[0], recData[0], recData[1]);
     //printf("sndD:%b   recD1: %b   recD2: %b\r\n", sndData2[0], recData2[0], recData2[1]);
    // sleep_ms(200);

}


void pushyShovey(){

//MOTOR STUFF
    // so if we have 45° min/max, over a 4095 range of screen
    // we'll need to convert size of tau to size of motor-screen angle somhow. ±2047u = ±45° = ±928
    //  = 2.21 multiplier

    hw_clear_bits(&timer_hw->intr, 1u << CONTROL_ALARM_NUM);
    timer_hw->alarm[CONTROL_ALARM_NUM] += CONTROL_CALLBACK_TIME;


        ex = posxDes - xLoc + 0.0001;
        edotx = (ex - eoldx) / dt;
        eintx += ex*dt;
        taux = kpx*ex + kdx*edotx + kix*eintx;
        eoldx = ex;

        ey= posyDes - yLoc + 0.0001;
        edoty = (ey - eoldy) / dt;
        einty += ey*dt;
        tauy = kpy*ey + kdy*edoty + kiy*einty;
        eoldy = ey;

    if(eintx > 20000){
        eintx = 20000;
    }
    if(eintx < -20000){
        eintx = -20000;
    }
    if(einty > 20000){
        einty = 20000;
    }
    if(einty < -20000){
        einty = -20000;
    }

    // VERIFY POS/NEG compared to the screen axes!
    // add caps on motor movement to 45°
    // set kp/kd/ki numbers to >0
	// Think this was our issue!!!!!! : on the -taus, we're making pos in the inner if (when bigger than 2047), 
	// BUT, smaller ones are left as itself, which is neg. but since we subtract in the gpio-level-set, we want tau to be pos. 
	// So changing the tau = tau / 2.21 to add the (-).
	
	
	//THE DIVISOR IS SUPPOSED TO BE 2.21 in order to change the max screen distance from center (±2047) into the ±45° pwm of ±928. 
	// BUT, setting it to 3 makes the device less jumpy and work better, but all this does is reduce the max angle range. 
	// Does not fix the issue where any error seems to go to max fix angle all the time. 
	
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
            taux = -taux / 2.21;
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
            tauy = -tauy / 2.21;
        }
        pwm_set_gpio_level(MOTORY_PIN, 4909 + tauy);
    }else{
        //pwm_set_gpio_level(MOTORY_PIN, MCENT);
    }

          if(loopcount>30){
              printf("X: %u\r\n", xLoc);
              printf("Y: %u\r\n", yLoc);
              printf("tauy: %f taux: %f posxDes: %f posyDes: %f \r\n\r\n", tauy, taux, posxDes, posyDes);
              printf("ex: %f edotx: %f eintx: %f eoldx: %f \r\n\r\n", kpx*ex, kdx*edotx, kix*eintx, eoldx);
              printf("kpx: %f kdx: %f kix: %f\r\n\r\n", kpx, kdx, kix);
              loopcount= 0;
          }
          loopcount++;



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
    uart_init(uart0, 38400);
    gpio_set_function(btTX, GPIO_FUNC_UART);
    gpio_set_function(btRX, GPIO_FUNC_UART);
//    for (int i = 0; i<10; ++i)
//    {
//         printf("%d",i);
//         uart_puts(uart0, "AT\r\n");
//       uartFlush();
//         sleep_ms(1000);
//    }
        //uart_puts(uart0, "AT+ORGL\r\n");
        // uartFlush();
        //sleep_ms(1000);
        // uart_puts(uart0, "AT+NAME=BallBal\r\n");
        // uartFlush();
        // sleep_ms(1000);
        // uart_puts(uart0, "AT+PSWD=1111\r\n");
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
	pwm_set_wrap(sliceX_num, 6873*5);
    pwm_set_wrap(sliceY_num, 6873*5);
	pwm_set_enabled(sliceX_num, true);
    pwm_set_enabled(sliceY_num, true);
    pwm_set_gpio_level(MOTORX_PIN, 4909);
    pwm_set_gpio_level(MOTORY_PIN, 4909);

   
    //pwm_set_gpio_level(MOTORX_PIN, 5900);
   
    //pwm_set_gpio_level(MOTORY_PIN, 5900);
   
    //pwm_set_gpio_level(MOTORX_PIN, 3900);
    
    //pwm_set_gpio_level(MOTORY_PIN, 3900);
   
    //pwm_set_gpio_level(MOTORX_PIN, 4909);
    //pwm_set_gpio_level(MOTORY_PIN, 4909);
    

    irq_set_exclusive_handler(TOUCH_IRQ, touchyFeely);
    hw_set_bits(&timer_hw->inte, 1u << TOUCH_ALARM_NUM);
    irq_set_enabled(TOUCH_IRQ, true);
    irq_set_exclusive_handler(CONTROL_IRQ, pushyShovey);
    hw_set_bits(&timer_hw->inte, 1u << CONTROL_ALARM_NUM);
    irq_set_enabled(CONTROL_IRQ, true);
    uint t = time_us_32();
    timer_hw->alarm[TOUCH_ALARM_NUM] = t + 5000;
    timer_hw->alarm[CONTROL_ALARM_NUM] = t + 2500 + CONTROL_CALLBACK_TIME;

    sleep_ms(30000);
}



void bluLoop()
{
    char input1 = uart_getc(uart0);
   if(input1 == 'X')
   {
    char c[4];
    int j,k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    c[3] = uart_getc(uart0);
    uart_getc(uart0);
    j = ((int)(c[0])-48)*1000;
    k = ((int)(c[1])-48)*100;
    l = ((int)(c[2])-48)*10;
    m = ((int)(c[3])-48)*1;
    posxDes = j+k+l+m;
   }
   else if(input1 == 'Y')
   {
    char c[4];
    int j,k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    c[3] = uart_getc(uart0);
    uart_getc(uart0);
    j = ((int)(c[0])-48)*1000;
    k = ((int)(c[1])-48)*100;
    l = ((int)(c[2])-48)*10;
    m = ((int)(c[3])-48)*1;
    posyDes = j+k+l+m;
   }
	//  COMMENTOUT out because otherwise the X/Y set above didn't want to work, X just went to 40180 or something, Y wouldn;t change...
   else if(input1 == 'P'){
	if(uart_getc(uart0) == 'X'){
    char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kpx = (k+l+m)/40.0;
	   }else{
	char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kpy = (k+l+m)/40.0;
   }
   }
   else if(input1 == 'D'){
    if(uart_getc(uart0) == 'X'){
    char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kdx = (k+l+m)/40.0;
    }else{
	    char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kdy = (k+l+m)/40.0;    
    }
   }
   else if(input1 == 'I'){
    if(uart_getc(uart0) == 'X'){
    char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kix = (k+l+m)/40.0;
   }else{
	char c[3];
    int k,l,m;
    c[0] = uart_getc(uart0);
    c[1] = uart_getc(uart0);
    c[2] = uart_getc(uart0);
    uart_getc(uart0);
    k = ((int)(c[0])-48)*100;
    l = ((int)(c[1])-48)*10;
    m = ((int)(c[2])-48)*1;
    kiy = (k+l+m)/40.0;      
   }}
   
   else{
        uart_getc(uart0);
   }
}


int main()
 {
     setup();    
    int loopcount= 0;
     while (true)
     {
         bluLoop();
         /* if(loopcount>200){
              printf("X: %u\r\n", xLoc);
              printf("Y: %u\r\n", yLoc);
              printf("tauy: %f taux: %f posxDes: %f posyDes: %f ", tauy, taux, posxDes, posyDes);
              loopcount= 0;
          }
          loopcount++;
        */
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
