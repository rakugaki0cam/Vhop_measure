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

/*
                         Main application
 */
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();

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

    while (1)
    {
        // Add your application code
        
    }
}
/**
 End of File
*/