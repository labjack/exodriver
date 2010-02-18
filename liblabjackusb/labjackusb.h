//----------------------------------------------------------------------
//
//  labjackusb.h
//
//  Header file for the labjackusb library.
//
//  support@labjack.com
//
//----------------------------------------------------------------------
//
//  Linux Version History
//
//  0.90 - Initial release (LJUSB_AbortPipe not supported)
//
//  1.00 - Added LJUSB_SetBulkReadTimeout
//
//  1.10 - Changed the HANDLE to a void * (previously int)
//       - Added LJUSB_GetLibraryVersion
//       - Removed UE9_PIPE_EP2_OUT
//       - changed the values of the pipes (incremented by 1)
//       - removed function LJUSB_SetBulkReadTimeout
//       - changed LJUSB_LINUX_DRIVER_VERSION define name to
//         LJUSB_LINUX_LIBRARY_VERSION
//
//  2.00 - Re-implemented library using libusb 1.0.  No longer
//         requires LabJack kernel module.
//       - Replaced define names U3_PIPE_EP1_IN and U3_PIPE_EP2_IN
//         with U3_PIPE_EP2_IN and U3_PIPE_EP3_IN
//       - Added U6 support
//
//  2.01 - Added U12 support
//       - Added Wireless Bridge support
//----------------------------------------------------------------------
//

#ifndef _LABJACKUSB_H_
#define _LABJACKUSB_H_

#define LJUSB_LINUX_LIBRARY_VERSION 2.0f

#include <stdbool.h>

typedef void * HANDLE;
typedef unsigned int UINT;
typedef unsigned char BYTE;

//Product IDs
#define UE9_PRODUCT_ID    9
#define U3_PRODUCT_ID     3
#define U6_PRODUCT_ID     6
#define U12_PRODUCT_ID    1
#define BRIDGE_PRODUCT_ID 0x0501
#define UNUSED_PRODUCT_ID -1

//UE9 pipes to read/write through
#define UE9_PIPE_EP1_OUT  1
#define UE9_PIPE_EP1_IN   0x81
#define UE9_PIPE_EP2_IN   0x82 //Stream Endpoint

//U3 pipes to read/write through
#define U3_PIPE_EP1_OUT   1
#define U3_PIPE_EP2_IN    0x82
#define U3_PIPE_EP3_IN    0x83  //Stream Endpoint

//U6 pipes to read/write through
#define U6_PIPE_EP1_OUT   1
#define U6_PIPE_EP2_IN    0x82
#define U6_PIPE_EP3_IN    0x83 //Stream Endpoint

//U12 pipes to read/write through
#define U12_PIPE_EP1_IN    0x81
#define U12_PIPE_EP2_OUT   2

//Wireless bridge pipes to read/write through
#define BRIDGE_PIPE_EP1_IN    0x81
#define BRIDGE_PIPE_EP1_OUT   1

#ifdef __cplusplus
extern "C"{
#endif


float LJUSB_GetLibraryVersion(void);
//Returns the labjackusb library version number


unsigned long LJUSB_GetDevCount(unsigned long ProductID);
//Returns the total number of LabJack USB devices connected.
//ProductID = The product ID of the devices you want to get the count of.


HANDLE LJUSB_OpenDevice(UINT DevNum, unsigned int dwReserved, unsigned long ProductID);
//Obtains a handle for a LabJack USB device.  Returns NULL if there is an
//error.  If the device is already open, NULL is returned and errno is set to
//EBUSY.
//DevNum = The device number of the LabJack USB device you want to open.  For
//         example, if there is one device connected, set DevNum = 1.  If you
//         have two devices connected, then set DevNum = 1, or DevNum = 2.
//dwReserved = Not used, set to 0.
//ProductID = The product ID of the LabJack USB device.  Currently the U3, U6,
//            and UE9 are supported.

unsigned long LJUSB_Write(HANDLE hDevice, BYTE *pBuff, unsigned long count);
// Writes to a device. Returns the number of bytes written, or -1 on error.
// hDevice = The handle for your device
// pBuff = The buffer to be written to the device.
// count = The number of bytes to write.
// This function replaces the deprecated LJUSB_BulkWrite, which required the endpoint

unsigned long LJUSB_Read(HANDLE hDevice, BYTE *pBuff, unsigned long count);
// Reads from a device. Returns the number of bytes read, or -1 on error.
// hDevice = The handle for your device
// pBuff = The buffer to filled in with bytes from the device.
// count = The number of bytes expected to be read.
// This function replaces the deprecated LJUSB_BulkRead, which required the endpoint

unsigned long LJUSB_Stream(HANDLE hDevice, BYTE *pBuff, unsigned long count);
// Reads from a device's stream interface. 
// Returns the number of bytes read, or -1 on error.
// hDevice = The handle for your device
// pBuff = The buffer to filled in with bytes from the device.
// count = The number of bytes expected to be read.
// This function replaces the deprecated LJUSB_BulkRead, which required the (stream) endpoint


void LJUSB_CloseDevice(HANDLE hDevice);
//Closes the handle of a LabJack USB device.

bool LJUSB_IsHandleValid(HANDLE hDevice);
//Returns true if the handle is valid; this is, it is still connected to a
//device on the system.

bool LJUSB_AbortPipe(HANDLE hDevice, unsigned long Pipe);
//Not supported under Linux and will return false.
//Pipes will timeout after LJ_LIBUSB_TIMEOUT, which is set by default to 1 second.

//Note:  For all function errors, use errno to retrieve system error numbers.

/* --------------- DEPRECATED Functions --------------- */

unsigned long LJUSB_BulkRead(HANDLE hDevice, unsigned char endpoint, BYTE *pBuff, unsigned long count);
//Reads from a bulk endpoint.  Returns the count of the number of bytes read,
//or 0 on error (and sets errno).  If there is no response within a certain
//amount of time (LJ_LIBUSB_TIMEOUT in labjackusb.c), the read will timeout.
//hDevice = Handle of the LabJack USB device.
//endpoint = The pipe you want to read your data through (xxx_PIPE_xxx_IN).
//*pBuff = Pointer a buffer that will be read from the device.
//count = The size of the buffer to be read from the device.


unsigned long LJUSB_BulkWrite(HANDLE hDevice, unsigned char endpoint, BYTE *pBuff, unsigned long count);
//Writes to a bulk endpoint.  Returns the count of the number of bytes wrote,
//or 0 on error and sets errno.
//hDevice = Handle of the LabJack USB device.
//endpoint = The pipe you want to write your data through (xxx_PIPE_xxx_OUT).
//*pBuff = Pointer to the buffer that will be written to the device.
//count = The size of the buffer to be written to the device.

#ifdef __cplusplus
}
#endif

#endif
