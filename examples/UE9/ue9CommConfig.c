//Author: LabJack
//June 23, 2009
//This example program sends a CommConfig low-level command and reads the 
//various configuration settings associated with the Comm processor.

#include "ue9.h"


int commConfig_example(HANDLE hDevice);

int main(int argc, char **argv)
{
    HANDLE hDevice;

    //Opening first found UE9 over USB
    if( (hDevice = openUSBConnection(-1)) == NULL)
        return 1;

    commConfig_example(hDevice);
    closeUSBConnection(hDevice);

    return 0;
}

//Sends a CommConfig low-level command to read the configuration settings 
//associated with the Comm chip. 
int commConfig_example(HANDLE hDevice)
{
    uint8 sendBuff[38], recBuff[38];
    int sendChars, recChars;
    int i;
    uint16 checksumTotal;

    sendBuff[1] = (uint8)(0x78);  //command byte
    sendBuff[2] = (uint8)(0x10);  //number of data words
    sendBuff[3] = (uint8)(0x01);  //extended command number

    //WriteMask, LocalID, PowerLevel, etc. are all passed a value of
    //zero since we only want to read Comm configuration settings, 
    //not change them
    for(i = 6; i < 38; i++)
        sendBuff[i] = (uint8)(0x00);

    extendedChecksum(sendBuff,38);

    //Sending command to UE9
    sendChars = LJUSB_BulkWrite(hDevice, UE9_PIPE_EP1_OUT, sendBuff, 38);
    if(sendChars < 38)
    {
        if(sendChars == 0)
            printf("Error : write failed\n");
        else
            printf("Error : did not write all of the buffer\n");
        return -1;
    }

    //Reading response from UE9
    recChars = LJUSB_BulkRead(hDevice, UE9_PIPE_EP1_IN, recBuff, 38);
    if(recChars < 38)
    {
        if(recChars == 0)
            printf("Error : read failed\n");
        else
            printf("Error : did not read all of the buffer\n");
        return -1;
    }

    checksumTotal = extendedChecksum16(recBuff, 38);
    if( (uint8)((checksumTotal / 256) & 0xff) != recBuff[5])
    {
        printf("Error : read buffer has bad checksum16(MSB)\n");
        return -1;
    }

    if( (uint8)(checksumTotal & 0xff) != recBuff[4])
    {
        printf("Error : read buffer has bad checksum16(LBS)\n");
        return -1;
    }

    if( extendedChecksum8(recBuff) != recBuff[0])
    {
        printf("Error : read buffer has bad checksum8\n");
        return -1;
    }


    if( recBuff[1] != (uint8)(0x78) || recBuff[2] != (uint8)(0x10) || recBuff[3] != (uint8)(0x01) )
    {
        printf("Error : read buffer has wrong command bytes \n");
        return -1;
    }

    printf("LocalID (byte 8): %d\n", recBuff[8]);
    printf("PowerLevel (byte 9): %d\n", recBuff[9]);
    printf("ipAddress (bytes 10-13): %d.%d.%d.%d\n", recBuff[13], recBuff[12], recBuff[11], recBuff[10]);
    printf("Gateway (bytes 14 - 17): %d.%d.%d.%d\n", recBuff[17], recBuff[16], recBuff[15], recBuff[14]);
    printf("Subnet (bytes 18 - 21): %d.%d.%d.%d\n", recBuff[21], recBuff[20], recBuff[19], recBuff[18]);
    printf("PortA (bytes 22 - 23): %d\n", recBuff[22] + (recBuff[23] * 256 ));
    printf("PortA (bytes 24 - 25): %d\n", recBuff[24] + (recBuff[25] * 256 ));
    printf("DHCPEnabled (byte 26): %d\n", recBuff[26]);
    printf("ProductID (byte 27): %d\n", recBuff[27]);
    printf("MACAddress (bytes 28 - 33): ");

    for(i = 5; i >= 0  ; i--)
    {
        printf("%02X", recBuff[i+28]);

        if(i > 2)
            printf(".");

        if(i == 2)
            printf(" ");
    }

    printf("\nHWVersion (bytes 34-35): %.3f\n", (unsigned int)recBuff[35]  + (double)recBuff[34]/100.0);
    printf("CommFWVersion (bytes 36-37): %.3f\n\n", (unsigned int)recBuff[37] + (double)recBuff[36]/100.0);

    return 0;
}
