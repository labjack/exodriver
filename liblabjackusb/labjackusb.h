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

#define LJUSB_LINUX_LIBRARY_VERSION 2.0

typedef void * HANDLE;
typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef int BOOL;
typedef unsigned int DWORD;

//Product IDs
#define UE9_PRODUCT_ID    9
#define U3_PRODUCT_ID     3
#define U6_PRODUCT_ID     6
#define U12_PRODUCT_ID    1
#define UNUSED_PRODUCT_ID -1

//UE9 pipes to read/write through
#define UE9_PIPE_EP1_OUT  1
#define UE9_PIPE_EP1_IN   2
#define UE9_PIPE_EP2_IN   4 //Stream Endpoint

//U3 pipes to read/write through
#define U3_PIPE_EP1_OUT   1
#define U3_PIPE_EP2_IN    2
#define U3_PIPE_EP3_IN    4  //Stream Endpoint

//U6 pipes to read/write through
#define U6_PIPE_EP1_OUT   1
#define U6_PIPE_EP2_IN    2
#define U6_PIPE_EP3_IN    4 //Stream Endpoint

//U12 pipes to read/write through
#define U12_PIPE_EP1_IN    0x81
#define U12_PIPE_EP2_OUT   2

#ifdef __cplusplus
extern "C"{
#endif


float LJUSB_GetLibraryVersion();
//Returns the labjackusb library version number


ULONG LJUSB_GetDevCount(ULONG ProductID);
//Returns the total number of LabJack USB devices connected.
//ProductID = The product ID of the devices you want to get the count of.


HANDLE LJUSB_OpenDevice(UINT DevNum, DWORD dwReserved, ULONG ProductID);
//Obtains a handle for a LabJack USB device.  Returns NULL if there is an
//error.  If the device is already open, NULL is returned and errno is set to
//EBUSY.
//DevNum = The device number of the LabJack USB device you want to open.  For
//         example, if there is one device connected, set DevNum = 1.  If you
//         have two devices connected, then set DevNum = 1, or DevNum = 2.
//dwReserved = Not used, set to 0.
//ProductID = The product ID of the LabJack USB device.  Currently the U3, U6,
//            and UE9 are supported.


ULONG LJUSB_IntRead(HANDLE hDevice, ULONG Pipe, BYTE *pBuff, ULONG Count);
//Reads from an interrupt endpoint.  Returns the count of the number of bytes read,
//or 0 on error (and sets errno).  If there is no response within a certain
//amount of time (LJ_LIBUSB_TIMEOUT in labjackusb.c), the read will timeout.
//hDevice = Handle of the LabJack USB device.
//Pipe = The pipe you want to read your data through (xxx_PIPE_xxx_IN).
//*pBuff = Pointer a buffer that will be read from the device.
//Count = The size of the buffer to be read from the device.

ULONG LJUSB_IntWrite(HANDLE hDevice, ULONG Pipe, BYTE *pBuff, ULONG Count);
//Writes to an interrupt endpoint.  Returns the count of the number of bytes wrote,
//or 0 on error and sets errno.
//hDevice = Handle of the LabJack USB device.
//Pipe = The pipe you want to write your data through (xxx_PIPE_xxx_OUT).
//*pBuff = Pointer to the buffer that will be written to the device.
//Count = The size of the buffer to be written to the device.


ULONG LJUSB_BulkRead(HANDLE hDevice, ULONG Pipe, BYTE *pBuff, ULONG Count);
//Reads from a bulk endpoint.  Returns the count of the number of bytes read,
//or 0 on error (and sets errno).  If there is no response within a certain
//amount of time (LJ_LIBUSB_TIMEOUT in labjackusb.c), the read will timeout.
//hDevice = Handle of the LabJack USB device.
//Pipe = The pipe you want to read your data through (xxx_PIPE_xxx_IN).
//*pBuff = Pointer a buffer that will be read from the device.
//Count = The size of the buffer to be read from the device.


ULONG LJUSB_BulkWrite(HANDLE hDevice, ULONG Pipe, BYTE *pBuff, ULONG Count);
//Writes to a bulk endpoint.  Returns the count of the number of bytes wrote,
//or 0 on error and sets errno.
//hDevice = Handle of the LabJack USB device.
//Pipe = The pipe you want to write your data through (xxx_PIPE_xxx_OUT).
//*pBuff = Pointer to the buffer that will be written to the device.
//Count = The size of the buffer to be written to the device.


void LJUSB_CloseDevice(HANDLE hDevice);
//Closes the handle of a LabJack USB device.

BOOL LJUSB_IsHandleValid(HANDLE hDevice);
//Returns true if the handle is valid; this is, it is still connected to a
//device on the system.

BOOL LJUSB_AbortPipe(HANDLE hDevice, ULONG Pipe);
//Not supported under Linux and will return false (0).
//Pipes will timeout after LJ_LIBUSB_TIMEOUT, which is set by default to 1 second.

//Note:  For all function errors, use errno to retrieve system error numbers.

#ifdef __cplusplus
}
#endif

#endif
