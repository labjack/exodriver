//Author : LabJack
//April 7, 2008
//This example calls the ConfigU3 low-level function and reads back
//configuration settings.

#include "u3.h"


int configU3_example(HANDLE hDevice);

int main(int argc, char **argv)
{
    HANDLE hDevice;

    //Opening first found U3 over USB
    if( (hDevice = openUSBConnection(-1)) == NULL)
        return 1;

    configU3_example(hDevice);
    closeUSBConnection(hDevice);

    return 0;
}

//Sends a ConfigU3 low-level command to read back configuration settings
int configU3_example(HANDLE hDevice)
{
    uint8 sendBuff[26];
    uint8 recBuff[38];
    uint16 checksumTotal;
    int sendChars, recChars, i;

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = (uint8)(0x0A);  //Number of data words
    sendBuff[3] = (uint8)(0x08);  //Extended command number

    //Setting all bytes to zero since we only want to read back the U3
    //configuration settings
    for(i = 6; i < 26; i++)
        sendBuff[i] = 0;

    /* The commented out code below sets the U3's local ID to 3.  After setting
     *  the local ID, reset the device for this change to take effect. */

    //sendBuff[6] = 8;   //WriteMask : setting bit 3
    //sendBuff[8] = 3;   //LocalID : setting local ID to 3

    extendedChecksum(sendBuff, 26);

    //Sending command to U3
    if( (sendChars = LJUSB_BulkWrite(hDevice, U3_PIPE_EP1_OUT, sendBuff, 26)) < 26)
    {
        if(sendChars == 0)
            printf("ConfigU3 error : write failed\n");
        else
            printf("ConfigU3 error : did not write all of the buffer\n");
        return -1;
    }

    //Reading response from U3
    if( (recChars = LJUSB_BulkRead(hDevice, U3_PIPE_EP2_IN, recBuff, 38)) < 38)
    {
        if(recChars == 0)
            printf("ConfigU3 error : read failed\n");
        else
            printf("ConfigU3 error : did not read all of the buffer\n");
        return -1;
    }

    checksumTotal = extendedChecksum16(recBuff, 38);
    if( (uint8)((checksumTotal / 256) & 0xff) != recBuff[5])
    {
        printf("ConfigU3 error : read buffer has bad checksum16(MSB)\n");
        return -1;
    }
 
    if( (uint8)(checksumTotal & 0xff) != recBuff[4])
    {
        printf("ConfigU3 error : read buffer has bad checksum16(LBS)\n");
        return -1;
    }

    if( extendedChecksum8(recBuff) != recBuff[0])
    {
        printf("ConfigU3 error : read buffer has bad checksum8\n");
        return -1;
    }

    if( recBuff[1] != (uint8)(0xF8) || recBuff[2] != (uint8)(0x10) || recBuff[3] != (uint8)(0x08) )
    {
        printf("ConfigU3 error : read buffer has wrong command bytes\n");
        return -1;
    }

    if( recBuff[6] != 0)
    {
        printf("ConfigU3 error : read buffer received errorcode %d\n", recBuff[6]);
        return -1;
    }

    printf("U3 Configuration Settings:\n");
    printf("FirmwareVersion: %.3f\n", recBuff[10] + recBuff[9]/100.0);
    printf("BootloaderVersion: %.3f\n", recBuff[12] + recBuff[11]/100.0);
    printf("HardwareVersion: %.3f\n", recBuff[14] + recBuff[13]/100.0);
    printf("SerialNumber: %u\n", recBuff[15] + recBuff[16]*256 + recBuff[17]*65536 + recBuff[18]*16777216);
    printf("ProductID: %d\n", recBuff[19] + recBuff[20]*256);
    printf("LocalID: %d\n", recBuff[21]);
    printf("FIOAnalog (FIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[23]&1, (recBuff[23]/2)&1,
        (recBuff[23]/4)&1, (recBuff[23]/8)&1, (recBuff[23]/16)&1, (recBuff[23]/32)&1,
        (recBuff[23]/64)&1, (recBuff[23]/128)&1);
    printf("FIODirection (FIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[24]&1, (recBuff[24]/2)&1,
        (recBuff[24]/4)&1, (recBuff[24]/8)&1, (recBuff[24]/16)&1, (recBuff[24]/32)&1,
        (recBuff[24]/64)&1, (recBuff[24]/128)&1);
    printf("FIOState (FIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[25]&1, (recBuff[25]/2)&1,
        (recBuff[25]/4)&1, (recBuff[25]/8)&1, (recBuff[25]/16)&1, (recBuff[25]/32)&1,
        (recBuff[25]/64)&1, (recBuff[25]/128)&1);
    printf("EIOAnalog (EIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[26]&1, (recBuff[26]/2)&1,
        (recBuff[26]/4)&1, (recBuff[26]/8)&1, (recBuff[26]/16)&1, (recBuff[26]/32)&1,
        (recBuff[26]/64)&1, (recBuff[26]/128)&1);
    printf("EIODirection (EIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[27]&1, (recBuff[27]/2)&1,
        (recBuff[27]/4)&1, (recBuff[27]/8)&1, (recBuff[27]/16)&1, (recBuff[27]/32)&1,
        (recBuff[27]/64)&1, (recBuff[27]/128)&1);
    printf("EIOState (EIO0-7): %d %d %d %d %d %d %d %d\n", recBuff[28]&1, (recBuff[28]/2)&1,
        (recBuff[28]/4)&1, (recBuff[28]/8)&1, (recBuff[28]/16)&1, (recBuff[28]/32)&1,
        (recBuff[28]/64)&1, (recBuff[28]/128)&1);
    printf("CIODirection (CIO0-3): %d %d %d %d\n", recBuff[29]&1, (recBuff[29]/2)&1, (recBuff[29]/4)&1,
        (recBuff[29]/8)&1);
    printf("CIOState (CIO0-3): %d %d %d %d\n", recBuff[30]&1, (recBuff[30]/2)&1, (recBuff[30]/4)&1,
        (recBuff[30]/8)&1);
    printf("DAC1Enable: %d\n", recBuff[31]);
    printf("DAC0 (in byte form): %d\n", recBuff[32]);
    printf("DAC1 (in byte form): %d\n", recBuff[33]);

    printf("\nU3 Hardware Version >= 1.21 Configuration Settings:\n");
    printf("Hardware Variations: %d\n", recBuff[37]);
    printf("  U3b (bit 0): %d\n", (recBuff[37]&1));
    printf("  U3c (bit 1): %d\n", ((recBuff[37]/2)&1));
    printf("  High Voltage (bit 4): %d\n", ((recBuff[37]/16)&1));

    printf("\nU3 Hardware Version 1.30 Configuration Settings:\n");
    printf("TimerCounter Clock Source: %d\n", recBuff[34]);
    printf("TimerCounter Divisor: %d\n", recBuff[35]);
    printf("Compatibility Options: %d\n", recBuff[36]);
    printf("  Disable Timer Counter Pin Offset Errors (bit 0): %d\n", (recBuff[36]&1));

    return 0;
}
