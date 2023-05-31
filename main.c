/*
 * Hop packing extract velocity measurement device
 *
 * Vhop_measure_PIC18F25K80
 *     
 *  2023/050/30
 * 
 *  clock: Xtal 16MHz x 4 -- 64MHz
 * 
 *  sensor1 -- pin22 RB0/C1INA - T1G gate toggle ----> timer1 start
 *  sensor2 -- comparator NJM2403 - pin18 RC7/CCP4 --> timer1 capture
 *  sensor3 -- comparator NJM2403 - pin26 RB5/CCP5 --> timer1 capture
 * 
 *      comparator IN+ -- pin2 RA0/CVREF
 *      DEBUGger --pin17 RC6/TX1 9600bps
 * 
 * 
 * 
 * 
 * 
 * 
*/


#include "mcc_generated_files/mcc.h"
#include "header.h"

//タイマ換算値
#define TIMER1GAIN  (float)(4*4*1000/64000000)  // msec/bit   clock 64MHz FOSC/4 prescaler1:4
//センサ位置
#define D0  9-3 // mm 玉待機位置～センサ1 玉半径3mm先がオンする位置
#define D1  7   // mm センサ1～2距離
#define D2  7   // mm センサ2～3距離


//global
bool sensor1flag = 0;
bool sensor2flag = 0;
bool sensor3flag = 0;
uint16_t ontime2;
uint16_t ontime3;

//local
float   dt1;    // msec センサ1～2時間
float   dt2;    // msec センサ2～3時間
float   v1;     // m/sec センサ1～2平均速度
float   v2;     // m/sec センサ2～3平均速度
uint32_t cnt;

/*
                         Main application
 */
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();

    TMR1_SetGateInterruptHandler(sensor1on);
    CCP4_SetCallBack(sensor2on);
    CCP5_SetCallBack(sensor3on);
    
    
    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    printf("*******************\n");
    printf(" Velocity  HOP out \n");
    printf("*******************\n");

    TMR1_WriteTimer(0xffff);
    TMR1_StartTimer();
    TMR1_StartSinglePulseAcquisition();
    cnt = 0;
    
    while (1)
    {
        // Add your application code
        if (sensor1flag == 1){
            //measurement start
            printf("meas\n");
            while(!sensor3flag){
                //wait
                cnt++;
                if (cnt > 0x2000){
                    printf("break\n");
                    break;
                }
            }
            TMR1_StopTimer();
            
            printf("timer2: %4x\n", ontime2);
            printf("timer3: %4x\n", ontime3);
            dt1 = (float)(0xffff - ontime2) * 0.00025; // msec
            dt2 = (float)(ontime2 - ontime3) * 0.00025; // msec
            v1 = (float)D1 / dt1;  // m/sec
            v2 = (float)D2 / dt2;  // m/sec
            printf("s12 %5.2fmsec\n", dt1);
            printf("    %5.3fm/sec\n", v1);
            printf("s23 %5.2fmsec\n", dt2);
            printf("    %5.3fm/sec\n", v2);
            printf("\n");
            
            __delay_ms(2000);
            
            //init
            cnt = 0;
            sensor1flag = 0;
            TMR1_WriteTimer(0xffff);
            TMR1_StartTimer();
            TMR1_StartSinglePulseAcquisition();
            
        }
        
    }
}


void sensor1on(void){
    sensor1flag = 1;
    
}

void sensor2on(uint16_t a){   //ontime2 - global
    sensor2flag = 1;
    ontime2 = a;
}

void sensor3on(uint16_t a){   //ontime3 - global
    sensor3flag = 1;
    ontime3 = a;
}

/**
 End of File
*/