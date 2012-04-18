//Author: LabJack
//April 7, 2008
//This examples demonstrates how to read from analog inputs (AIN) and digital
//inputs(FIO), set analog outputs (DAC) and digital outputs (FIO), and how to
//configure and enable timers and counters and read input timers and counters
//values using the "easy" functions.

#include "u3.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    HANDLE hDevice;
    u3CalibrationInfo caliInfo;
    int localID;
    long DAC1Enable, error;

    //Open first found U3 over USB
    localID = -1;
    if( (hDevice = openUSBConnection(localID)) == NULL )
        goto done;

    //Get calibration information from U3
    if( getCalibrationInfo(hDevice, &caliInfo) < 0 )
        goto close;


    /* Note: The eAIN, eDAC, eDI, and eDO "easy" functions have the ConfigIO
       parameter.  If calling, for example, eAIN to read AIN3 in a loop, set the
       ConfigIO parameter to 1 (True) on the first iteration so that the
       ConfigIO low-level function is called to ensure that channel 3 is set to
       an analog input.  For the rest of the iterations in the loop, set the
       ConfigIO parameter to 0 (False) since the channel is already set as
       analog. */


    //Set DAC0 to 2.1 volts.
    printf("Calling eDAC to set DAC0 to 2.1 V\n");
    if( (error = eDAC(hDevice, &caliInfo, 0, 0, 2.1, 0, 0, 0)) != 0 )
        goto close;

    sleep(1);


    /* Note: The eAIN "easy" function has the DAC1Enable parameter that needs to
       be set to calculate the correct voltage.  In addition to the earlier
       note, if running eAIN in a loop, set ConfigIO to 1 (True) on the first
       iteration to also set the output of the DAC1Enable parameter with the
       current setting on the U3.  For the rest of the iterations, set ConfigIO
       to 0 (False) and use the outputted DAC1Enable parameter from the first
       interation from then on.  If DAC1 is enabled/disabled from a later eDAC
       or ConfigIO low-level call, change the DAC1Enable parameter accordingly
       or make another eAIN call with the ConfigIO parameter set to 1. */

    //Read the single-ended voltage from AIN3
    printf("\nCalling eAIN to read voltage from AIN3\n");
    double dblVoltage;
    if( (error = eAIN(hDevice, &caliInfo, 1, &DAC1Enable, 3, 31, &dblVoltage, 0, 0, 0, 0, 0, 0)) != 0 )
        goto close;
    printf("AIN3 value = %.3f\n", dblVoltage);


    //Set FIO5 to output-high
    printf("\nCalling eDO to set FIO5 to output-high\n");
    if( (error = eDO(hDevice, 1, 5, 1)) != 0 )
        goto close;


    //Read state of FIO4
    printf("\nCalling eDI to read the state of FIO4\n");
    long lngState;
    if( (error = eDI(hDevice, 1, 4, &lngState)) != 0 )
        goto close;
    printf("FIO4 state = %ld\n", lngState);


    //Enable and configure 1 output timer and 1 input timer, and enable counter0
    printf("\nCalling eTCConfig to enable and configure 1 output timer (Timer0) and 1 input timer (Timer1), and enable counter0\n");
    long alngEnableTimers[2] = {1, 1};  //Enable Timer0-Timer1
    long alngTimerModes[2] = {LJ_tmPWM8, LJ_tmRISINGEDGES32};  //Set timer modes
    double adblTimerValues[2] = {16384, 0};  //Set PWM8 duty-cycles to 75%
    long alngEnableCounters[2] = {1, 0};  //Enable Counter0
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 4, LJ_tc48MHZ, 0, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
        goto close;

    printf("\nWaiting for 1 second...\n");
    sleep(1);


    //Read and reset the input timer (Timer1), read and reset Counter0, and
    //update the value (duty-cycle) of the output timer (Timer0)
    printf("\nCalling eTCValues to read and reset the input Timer1 and Counter0, and update the value (duty-cycle) of the output Timer0\n");
    long alngReadTimers[2] = {0, 1};  //Read Timer1
    long alngUpdateResetTimers[2] = {1, 0};  //Update timer0
    long alngReadCounters[2] = {1, 0};  //Read Counter0
    long alngResetCounters[2] = {0, 0};  //Reset no Counters
    double adblCounterValues[2] = {0, 0};
    adblTimerValues[0] = 32768;  //Change Timer0 duty-cycle to 50%
    if( (error = eTCValues(hDevice, alngReadTimers, alngUpdateResetTimers, alngReadCounters, alngResetCounters, adblTimerValues, adblCounterValues, 0, 0)) != 0 )
       goto close;
    printf("Timer1 value = %.0f\n", adblTimerValues[1]);
    printf("Counter0 value = %.0f\n", adblCounterValues[0]);


    //Disable all timers and counters
    alngEnableTimers[0] = 0;
    alngEnableTimers[1] = 0;
    alngEnableCounters[0] = 0;
    alngEnableCounters[1] = 0;
    if( (error = eTCConfig(hDevice, alngEnableTimers, alngEnableCounters, 4, 0, 0, alngTimerModes, adblTimerValues, 0, 0)) != 0 )
       goto close;
    printf("\nCalling eTCConfig to disable all timers and counters\n");


close:
    if( error > 0 )
        printf("Received an error code of %ld\n", error);
    closeUSBConnection(hDevice);
done:
    return 0;
}
