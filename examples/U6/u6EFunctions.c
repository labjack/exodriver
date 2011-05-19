//Author: LabJack
//April 5, 2011
//This examples demonstrates how to read from analog inputs (AIN) and digital inputs(FIO),
//set analog outputs (DAC) and digital outputs (FIO), and how to configure and enable
//timers and counters and read input timers and counters values using the "easy" functions.

#include "u6.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    HANDLE hDevice;
    u6CalibrationInfo caliInfo;
    int localID;
    long error;

    //Open first found U6 over USB
    localID = -1;
    if( (hDevice = openUSBConnection(localID)) == NULL )
        goto done;

    //Get calibration information from U6
    if( getCalibrationInfo(hDevice, &caliInfo) < 0 )
        goto close;

    //Set DAC0 to 3.1 volts.
    printf("Calling eDAC to set DAC0 to 3.1 V\n");
    if( (error = eDAC(hDevice, &caliInfo, 0, 3.1, 0, 0, 0)) != 0 )
        goto close;


    //Read the single-ended voltage from AIN3
    printf("\nCalling eAIN to read voltage from AIN3\n");
    double dblVoltage;
    if( (error = eAIN(hDevice, &caliInfo, 3, 15, &dblVoltage, 0, 0, 0, 0, 0, 0)) != 0 )
        goto close;
    printf("AIN3 value = %.3f\n", dblVoltage);


    //Set FIO2 to output-high
    printf("\nCalling eDO to set FIO2 to output-high\n");
    if( (error = eDO(hDevice, 2, 1)) != 0 )
        goto close;

    //Read state of FIO3
    printf("\nCalling eDI to read the state of FIO3\n");
    long lngState;
    if( (error = eDI(hDevice, 3, &lngState)) != 0 )
        goto close;
    printf("FIO3 state = %ld\n", lngState);

    //Enable and configure 1 output timer and 1 input timer, and enable counter0
    printf("\nCalling eTCConfig to enable and configure 1 output timer (Timer0) and 1 input timer (Timer1), and enable counter0\n");
    long alngEnableTimers[4] = {1, 1, 0, 0};  //Enable Timer0-Timer1
    long alngTimerModes[4] = {LJ_tmPWM8, LJ_tmRISINGEDGES32, 0, 0};  //Set timer modes
    double adblTimerValues[4] = {16384, 0, 0, 0};  //Set PWM8 duty-cycles to 75%
    long alngEnableCounters[2] = {1, 0};  //Enable Counter0
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 0, LJ_tc48MHZ, 0, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
        goto close;

    printf("\nWaiting for 1 second...\n");
    sleep(1);

    //Read and reset the input timer (Timer1), read and reset Counter0, and update the
    //value (duty-cycle) of the output timer (Timer0)
    printf("\nCalling eTCValues to read and reset the input Timer1 and Counter0, and update the value (duty-cycle) of the output Timer0\n");
    long alngReadTimers[4] = {0, 1, 0, 0};  //Read Timer1
    long alngUpdateResetTimers[4] = {1, 0, 0, 0};  //Update timer0
    long alngReadCounters[2] = {1, 0};  //Read Counter0
    long alngResetCounters[2] = {1, 0};  //Reset Counter 1
    double adblCounterValues[2] = {0, 0};
    adblTimerValues[0] = 32768;  //Change Timer0 duty-cycle to 50%
    adblTimerValues[1] = 0;
    if( (error = eTCValues(hDevice, alngReadTimers, alngUpdateResetTimers, alngReadCounters, alngResetCounters, adblTimerValues, adblCounterValues, 0, 0)) != 0 )
        goto close;
    printf("Timer1 value = %.0f\n", adblTimerValues[1]);
    printf("Counter0 value = %.0f\n", adblCounterValues[0]);

    //Disable all timers and counters
    alngEnableTimers[0] = 0;
    alngEnableTimers[1] = 0;
    alngEnableTimers[2] = 0;
    alngEnableTimers[3] = 0;
    alngEnableCounters[0] = 0;
    alngEnableCounters[1] = 0;
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 0, LJ_tc48MHZ, 0, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
        goto close;
    printf("\nCalling eTCConfig to disable all timers and counters\n");

close:
    if( error > 0 )
        printf("Received an error code of %ld\n", error);
closeUSBConnection(hDevice);
    done:
    return 0;
}
