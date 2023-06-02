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
 * 2023.06.02   ver.0.01    first version
 * 
 * 
*/


#include "mcc_generated_files/mcc.h"
#include "header.h"

//タイマ換算値
#define PRESCALER   1   // prescaler
#define TIMER1GAIN  (float)1/_XTAL_FREQ*4*PRESCALER*1000  // msec/bit  FOSC/4
//センサ位置
#define D01  9.0 - 3.0  // mm 玉待機位置～センサ1 玉半径3mm先がオンする位置
#define D12  7.0        // mm センサ1～2距離
#define D23  7.0        // mm センサ2～3距離

//timer
const uint16_t ontime1 = 0x0000;
uint16_t ontime2;
uint16_t ontime3;
//
const float   t1 = (float)ontime1 * TIMER1GAIN; // msec
float   t2;     // msec センサ1～2時間
float   t3;     // msec センサ1～3時間
float   dt2;    // msec センサ1～2時間
float   dt3;    // msec センサ2～3時間
float   v12;    // m/sec センサ1～2平均速度
float   v23;    // m/sec センサ2～3平均速度
uint16_t cnt;   // time over count

typedef enum n {
    IDLE,
    SENSOR1ON,
    SENSOR2ON,
    SENSOR3ON,
    TIMEOUT,
    ERROR,
    MEASURE_STATUS_NUM,        
} measure_status_t;

measure_status_t status;

typedef enum e {
    OK,
    ERR,
    MEASURE_ERROR_NUM,        
} measure_error_t;

measure_error_t ans;

/*
                         Main application
 */
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();
    CCP4_SetCallBack(sensor2on);
    CCP5_SetCallBack(sensor3on);
    
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    printf("\n");
    printf("********************\n");
    printf("HOPout Velocity Meas\n");
    printf(" d01: %3.1fmm\n", (float)D01);
    printf(" d12: %3.1fmm\n", (float)D12);
    printf(" d23: %3.1fmm\n", (float)D23);
    printf("********************\n");
    printf("\n");

    meas_tmr1_init();
    
    while (1)
    {
        // main loop
        ans = v_measure();
    }
}


// ***** SUB *******************************************************************
    
uint8_t v_measure(void){
    //measure main
    uint8_t ans = OK;
    
    switch (status){
        case IDLE:
            //idle
        case SENSOR1ON: 
            //timer start
        case SENSOR2ON:
            cnt = TMR1_ReadTimer();
            if (cnt > 0xff00){
                status = TIMEOUT;
                ans = ERR;
                return ans;
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
                //error
                status = ERROR;
                ans = ERR;
                return ans;
                break;
            }

            t2 = (float)ontime2 * TIMER1GAIN; // msec
            t3 = (float)ontime3 * TIMER1GAIN; // msec
            dt2 = t2 - t1;
            dt3 = t3 - t2;          // msec
            v12 = (float)D12 / dt2;    // m/sec
            v23 = (float)D23 / dt3;   // m/sec
            printf("t2:  %6.4f msec\n", t2);
            printf("v12: %6.2f m/s\n", v12);
            printf("t3:  %6.4f msec\n", t3);
            printf("v23: %6.2f m/s\n", v23);
            printf("\n");
            __delay_ms(100);
            meas_tmr1_init();
            return ans;
            break;

        case TIMEOUT:
            TMR1_StopTimer();                
            printf("time over\n");
            __delay_ms(100);
            meas_tmr1_init();
            ans = ERR;
            return ans;
            break;

         case ERROR:
            TMR1_StopTimer();                
            printf("measurement error\n");
            __delay_ms(100);
            meas_tmr1_init();
            ans = ERR;
            return ans;
            break;

        default:
            TMR1_StopTimer();                
            printf("unexcepted error\n");
            __delay_ms(100);
            meas_tmr1_init();
            ans = ERR;
            return ans;
    }
    return ans;
}


void meas_tmr1_init(void){
    cnt = 0;
    status = IDLE;
    TMR1_WriteTimer(ontime1);
    TMR1_StartTimer();
    TMR1_StartSinglePulseAcquisition();
}


void sensor2on(uint16_t a){   //ontime2 - global
    if (IDLE == status){
        status = SENSOR2ON;
        ontime2 = a;
    }
}

void sensor3on(uint16_t a){   //ontime3 - global
    if (SENSOR2ON == status){
        status = SENSOR3ON;
        ontime3 = a;
    }
}

/**
 End of File
*/