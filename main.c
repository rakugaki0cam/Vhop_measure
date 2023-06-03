/*
 * Hop packing extract velocity measurement device
 *
 * Vhop_measure_PIC18F25K80
 *     
 *  2023/050/30
 * 
 *  clock: Xtal 16MHz x 4 -- 64MHz
 * 
 *  sensor1 -- pin22 RB0/C1INA - T1G gate toggle ----> timer1(incremental) start
 *  sensor2 -- comparator NJM2403 - pin18 RC7/CCP4 --> timer1 capture &isr
 *  sensor3 -- comparator NJM2403 - pin26 RB5/CCP5 --> timer1 capture &isr
 * 
 *      comparator IN+ -- pin2 RA0/CVREF...cvr:7(5bit)  (5V-0V)/32 * 7 = 1.09V
 *      DEBUGger --pin17 RC6/TX1 9600bps
 * 
 * 
 * 2023.06.02   ver.0.02    first version
 * 2023.06.03   ver.1.00    OK,Fail LED追加
 * 
 * 
*/


#include "mcc_generated_files/mcc.h"
#include "header.h"

//タイマ換算値
#define PRESCALER   1   // prescaler
#define TIMER1GAIN  (float)1/_XTAL_FREQ*4*PRESCALER*1000  // msec/bit  FOSC/4
//センサ位置
#define D01  9.0-3.0    // mm 玉待機位置～センサ1 玉半径3mm先がオンする位置
#define D12  7.0        // mm センサ1～2距離
#define D23  7.0        // mm センサ2～3距離

//timer
const uint16_t  ontime1 = 0x0000;
uint16_t    ontime2;
uint16_t    ontime3;
//
const float t1 = (float)ontime1 * TIMER1GAIN; // msec
float   t2;     // msec センサ1～2時間
float   t3;     // msec センサ1～3時間
float   dt2;    // msec センサ1～2時間
float   dt3;    // msec センサ2～3時間
float   v12;    // m/sec センサ1～2平均速度
float   v23;    // m/sec センサ2～3平均速度

uint16_t timeout_cnt;   // time over count


typedef enum n {
    IDLE,
    SENSOR1ON,
    SENSOR2ON,
    SENSOR3ON,
    MEASURE_DONE,
    ERROR,
    CLEAR,
    MEASURE_STATUS_NUM,        
} measure_status_t;
measure_status_t status;


typedef enum e {
    OK,
    TIMEOUT,        
    TIME_ERR,
    UNEXCEP_ERR,
    MEASURE_ERROR_NUM,        
} measure_error_t;
measure_error_t re;


/*
                         Main application
 */
void main(void)
{
    uint8_t i;

    // Initialize the device
    SYSTEM_Initialize();
    CCP4_SetCallBack(sensor2on);
    CCP5_SetCallBack(sensor3on);
    TMR0_SetInterruptHandler(led_off_1sec);
    
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    __delay_ms(1000);
    printf("\n");
    printf("********************\n");
    printf("HOPout Velocity Meas\n");
    printf(" d01: %3.1fmm\n", (float)D01);
    printf(" d12: %3.1fmm\n", (float)D12);
    printf(" d23: %3.1fmm\n", (float)D23);
    printf("********************\n");
    printf("\n");
    LED_BLUE_SetLow();
    LED_RED_SetLow();
    __delay_ms(200);
    for(i = 0; i < 2; i++){
        LED_BLUE_SetHigh();
        LED_RED_SetHigh();
        __delay_ms(100);
        LED_BLUE_SetLow();
        LED_RED_SetLow();
        __delay_ms(100);
    }

    meas_tmr1_init();
    
    while (1)
    {
        // main loop
        re = v_measure();
    }
}


// ***** SUB *******************************************************************
    
uint8_t v_measure(void){
    //measure main
    static measure_error_t ans = OK;
    
    switch (status){
        case IDLE:
            //idle
        case SENSOR1ON: 
            //TMR1 start
        case SENSOR2ON:
            timeout_cnt = TMR1_ReadTimer();
            if (timeout_cnt > 0xfff0){
                status = ERROR;
                ans = TIMEOUT;
                break;
            }
            break;

        case SENSOR3ON:
            TMR1_StopTimer();
#define DEBUG_no
#ifdef DEBUG
            printf("timer2: %4x\n", ontime2);
            printf("timer3: %4x\n", ontime3);
#endif
            if (((ontime2 - ontime1) < 0x100) || ((ontime3 - ontime2) < 0x100)){
                //time error
                status = ERROR;
                ans = TIME_ERR;
                break;
            }
            status = MEASURE_DONE;
            break;
            
        case MEASURE_DONE:
            led_on(BLUE);
            t2 = (float)ontime2 * TIMER1GAIN; // msec
            t3 = (float)ontime3 * TIMER1GAIN; // msec
            dt2 = t2 - t1;
            dt3 = t3 - t2;          // msec
            v12 = (float)D12 / dt2;    // m/sec
            v23 = (float)D23 / dt3;   // m/sec
            printf("\n");
            printf("t2:  %6.4f msec\n", t2);
            printf("v12: %6.2f m/s\n", v12);
            printf("t3:  %6.4f msec\n", t3);
            printf("v23: %6.2f m/s\n", v23);

            status = CLEAR;
            break;

        case ERROR:
        default:
            led_on(RED);
            TMR1_StopTimer();
            if (TIME_ERR == ans){
                printf("measurement error\n");
            }else if (TIMEOUT == ans){
                printf("timeout\n");
            }else{
                printf("unexcepted error\n");
                ans = UNEXCEP_ERR;
            }
            
            status = CLEAR;
            break;
            
        case CLEAR:
            __delay_ms(500);
            meas_tmr1_init();
            ans = OK;
            break;

    }
    
    return ans;
}


void meas_tmr1_init(void){
    timeout_cnt = 0;
    status = IDLE;
    TMR1_WriteTimer(ontime1);
    TMR1_StartTimer();
    TMR1_StartSinglePulseAcquisition();
}


void sensor2on(uint16_t timer_value){   //ontime2 - global
    if (IDLE == status){
        status = SENSOR2ON;
        ontime2 = timer_value;
    }
}


void sensor3on(uint16_t timer_value){   //ontime3 - global
    if (SENSOR2ON == status){
        status = SENSOR3ON;
        ontime3 = timer_value;
    }
}

void led_on(led_color_t color){
    if (BLUE == color){
        LED_BLUE_SetHigh();
    }else if (RED == color){
        LED_RED_SetHigh();
    }else{
        return;
    }
    TMR0_Reload();
    TMR0_StartTimer();
}

void led_off_1sec(void){
    LED_BLUE_SetLow();
    LED_RED_SetLow();
    TMR0_StopTimer();
}

/**
 End of File
*/