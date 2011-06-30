//Author: LabJack
//March 2, 2007
//This examples demonstrates how to read from analog inputs (AIN) and digital inputs(FIO),
//set analog outputs (DAC) and digital outputs (FIO), and how to configure and enable
//timers and counters and read input timers and counters values using the "easy" functions.

#include "ue9.h"
#include <unistd.h>


int main(int argc, char **argv)
{
    HANDLE hDevice;
    ue9CalibrationInfo caliInfo;
    int localID, i;
    long error;


    //Open first found UE9 over USB
    localID = -1;
    if( (hDevice = openUSBConnection(localID)) == NULL )
        goto done;

    //Get calibration information from UE9
    if( getCalibrationInfo(hDevice, &caliInfo) < 0 )
        goto close;


    //Set DAC0 to 3.1 volts.
    printf("Calling eDAC to set DAC0 to 3.1 V\n");
    if( (error = eDAC(hDevice, &caliInfo, 0, 3.1, 0, 0, 0)) != 0 )
        goto close;


    //Read the voltage from AIN3 using 0-5 volt range at 12 bit resolution
    printf("Calling eAIN to read voltage from AIN3\n");
    double dblVoltage;
    if( (error = eAIN(hDevice, &caliInfo, 3, 0, &dblVoltage, LJ_rgUNI5V, 12, 0, 0, 0, 0)) != 0 )
        goto close;
    printf("\nAIN3 value = %.3f\n", dblVoltage);


    //Set FIO3 to output-high
    printf("\nCalling eDO to set FIO3 to output-high\n");
    if((error = eDO(hDevice, 3, 1)) != 0)
        goto close;


    //Read state of FIO2
    printf("\nCalling eDI to read the state of FIO2\n");
    long lngState;
    if( (error = eDI(hDevice, 2, &lngState)) != 0 )
        goto close;
    printf("FIO2 state = %ld\n", lngState);


    //Enable and configure 1 output timer and 1 input timer, and enable counter0
    printf("\nCalling eTCConfig to enable and configure 1 output timer (Timer0) and 1 input timer (Timer1), and enable counter0\n");
    long alngEnableTimers[6] = {1, 1, 0, 0, 0, 0};  //Enable Timer0-Timer1
    long alngTimerModes[6] = {LJ_tmPWM8, LJ_tmRISINGEDGES32, 0, 0, 0, 0};  //Set timer modes
    double adblTimerValues[6] = {16384, 0, 0, 0, 0, 0};  //Set PWM8 duty-cycles to 75%
    long alngEnableCounters[2] = {1, 0};  //Enable Counter0
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 0, LJ_tc750KHZ, 3, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
        goto close;

    printf("\nWaiting for 1 second...\n");
    sleep(1);


    //Read and reset the input timer (Timer1), read and reset Counter0, and update the
    //value (duty-cycle) of the output timer (Timer0)
    printf("\nCalling eTCValues to read and reset the input Timer1 and Counter0, and update the value (duty-cycle) of the output Timer0\n");
    long alngReadTimers[6] = {0, 1, 0, 0, 0, 0};  //Read Timer1
    long alngUpdateResetTimers[6] = {1, 0, 0, 0, 0, 0};  //Update timer0
    long alngReadCounters[2] = {1, 0};  //Read Counter0
    long alngResetCounters[2] = {0, 0};  //Reset Counter0
    double adblCounterValues[2] = {0, 0};
    adblTimerValues[0] = 32768;  //Change Timer0 duty-cycle to 50%
    if( (error = eTCValues(hDevice, alngReadTimers, alngUpdateResetTimers, alngReadCounters, alngResetCounters, adblTimerValues, adblCounterValues, 0, 0)) != 0 )
        goto close;
    printf("Timer1 value = %.0f\n", adblTimerValues[1]);
    printf("Counter0 value = %.0f\n", adblCounterValues[0]);


    //Disable all timers and counters
    for(i = 0; i < 6; i++)
        alngEnableTimers[i] = 0;
    alngEnableCounters[0] = 0;
    alngEnableCounters[1] = 0;
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 0, 0, 0, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
        goto close;
    printf("\nCalling eTCConfig to disable all timers and counters\n");


close:
    if( error > 0 )
        printf("Received an error code of %ld\n", error);
    closeUSBConnection(hDevice);
done:
    return 0;
}
