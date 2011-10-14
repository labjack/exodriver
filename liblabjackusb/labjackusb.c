//---------------------------------------------------------------------------
//
//  labjackusb.c
//
//    Library for accessing a U3, U6 and UE9 over USB.
//
//  support@labjack.com
//  
//----------------------------------------------------------------------
//

#include "labjackusb.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <errno.h>

#include <libusb-1.0/libusb.h>

#define LJ_LIBUSB_TIMEOUT       1000   // Milliseconds to wait on bulk transfers

// With a recent kernel, firmware and hardware checks aren't necessary
#define LJ_RECENT_KERNEL_MAJOR  2
#define LJ_RECENT_KERNEL_MINOR  6
#define LJ_RECENT_KERNEL_REV    28

#define MIN_UE9_FIRMWARE_MAJOR  1
#define MIN_UE9_FIRMWARE_MINOR  49

#define U3C_HARDWARE_MAJOR      1
#define U3C_HARDWARE_MINOR      30

#define MIN_U3C_FIRMWARE_MAJOR  1
#define MIN_U3C_FIRMWARE_MINOR  18

#define MIN_U6_FIRMWARE_MAJOR   0
#define MIN_U6_FIRMWARE_MINOR   81

#define DEBUG false

bool isLibUSBInitialized = false;
struct libusb_context *ljContext = NULL;

enum LJUSB_TRANSFER_OPERATION { LJUSB_WRITE, LJUSB_READ, LJUSB_STREAM };

struct LJUSB_FirmwareHardwareVersion 
{
    unsigned char firmwareMajor;
    unsigned char firmwareMinor;
    unsigned char hardwareMajor;
    unsigned char hardwareMinor;
};


static void LJUSB_U3_FirmwareHardwareVersion(HANDLE hDevice, struct LJUSB_FirmwareHardwareVersion * fhv)
{
    unsigned long i = 0, r;
    unsigned long epOut = U3_PIPE_EP1_OUT, epIn = U3_PIPE_EP2_IN;
    const unsigned long COMMAND_LENGTH = 26;
    const unsigned long RESPONSE_LENGTH = 38;
    BYTE command[COMMAND_LENGTH];
    BYTE response[RESPONSE_LENGTH];

    for (i = 0; i < COMMAND_LENGTH; i++) {
        command[i] = 0;
    }
    for (i = 0; i < RESPONSE_LENGTH; i++) {
        response[i] = 0;
    }

    //Checking firmware for U3.  Using ConfigU3 Low-level command
    command[0] = 11;
    command[1] = (BYTE)(0xF8);
    command[2] = (BYTE)(0x0A);
    command[3] = (BYTE)(0x08);

    LJUSB_BulkWrite(hDevice, epOut, command, COMMAND_LENGTH);

    if ((r = LJUSB_BulkRead(hDevice, epIn, response, RESPONSE_LENGTH)) < RESPONSE_LENGTH) {
        fprintf(stderr, "ConfigU3 response failed when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    if (response[1] != command[1] || response[2] != (BYTE)(0x10) || response[3] != command[3]) {
        fprintf(stderr, "Invalid ConfigU3 command bytes when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    fhv->firmwareMajor = response[10];
    fhv->firmwareMinor = response[9];
    fhv->hardwareMajor = response[14];
    fhv->hardwareMinor = response[13];

    return;
}


static void LJUSB_U6_FirmwareHardwareVersion(HANDLE hDevice, struct LJUSB_FirmwareHardwareVersion * fhv)
{
    unsigned long i = 0, r;
    unsigned long epOut = U6_PIPE_EP1_OUT, epIn = U6_PIPE_EP2_IN;
    const unsigned long COMMAND_LENGTH = 26;
    const unsigned long RESPONSE_LENGTH = 38;
    BYTE command[COMMAND_LENGTH];
    BYTE response[RESPONSE_LENGTH];

    for (i = 0; i < COMMAND_LENGTH; i++) {
        command[i] = 0;
    }
    for (i = 0; i < RESPONSE_LENGTH; i++) {
        response[i] = 0;
    }

    //Checking firmware for U6.  Using ConfigU3 Low-level command
    command[0] = 11;
    command[1] = (BYTE)(0xF8);
    command[2] = (BYTE)(0x0A);
    command[3] = (BYTE)(0x08);

    LJUSB_BulkWrite(hDevice, epOut, command, COMMAND_LENGTH);

    if ((r = LJUSB_BulkRead(hDevice, epIn, response, RESPONSE_LENGTH)) < RESPONSE_LENGTH) {
        fprintf(stderr, "ConfigU6 response failed when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    if (response[1] != command[1] || response[2] != (BYTE)(0x10) || response[3] != command[3]) {
        fprintf(stderr, "Invalid ConfigU6 command bytes when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    fhv->firmwareMajor = response[10];
    fhv->firmwareMinor = response[9];
    /* TODO: Add hardware major and minor */

    return;
}


static void LJUSB_UE9_FirmwareHardwareVersion(HANDLE hDevice, struct LJUSB_FirmwareHardwareVersion * fhv)
{
    unsigned long i = 0, r;
    unsigned long epOut = UE9_PIPE_EP1_OUT, epIn = UE9_PIPE_EP1_IN;
    const unsigned long COMMAND_LENGTH = 38;
    const unsigned long RESPONSE_LENGTH = 38;
    BYTE command[COMMAND_LENGTH];
    BYTE response[RESPONSE_LENGTH];

    for (i = 0; i < COMMAND_LENGTH; i++) {
        command[i] = 0;
    }
    for (i = 0; i < RESPONSE_LENGTH; i++) {
        response[i] = 0;
    }

    //Checking firmware for UE9.  Using CommConfig Low-level command
    command[0] = 137;
    command[1] = (BYTE)(0x78);
    command[2] = (BYTE)(0x10);
    command[3] = (BYTE)(0x01);

    LJUSB_BulkWrite(hDevice, epOut, command, COMMAND_LENGTH);

    if ((r = LJUSB_BulkRead(hDevice, epIn, response, RESPONSE_LENGTH)) < RESPONSE_LENGTH) {
        fprintf(stderr, "CommConfig response failed when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    if (response[1] != command[1] || response[2] != command[2] || response[3] != command[3]) {
        fprintf(stderr, "Invalid CommConfig command bytes when getting firmware and hardware versions\n");
        fprintf(stderr, "Response was:\n");
        for (i = 0; i < r; i++) {
            fprintf(stderr, "%d ", response[i]);
        }
        fprintf(stderr, "\n");
        return;
    }

    fhv->firmwareMajor = response[37];
    fhv->firmwareMinor = response[36];
    /* TODO: Add hardware major and minor */

    return;
}


static bool LJUSB_isNullHandle(HANDLE hDevice)
{
    if (hDevice == NULL) {
        // TODO: Consider different errno here and in LJUSB_isHandleValid
        errno = EINVAL;
        return true;
    }
    return false;
}


static int LJUSB_libusbError(int r)
{
    switch (r) {
    case LIBUSB_SUCCESS:
        // No error
        return 0;
        break;
    case LIBUSB_ERROR_IO:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_IO\n");
        errno = EIO;
        break;
    case LIBUSB_ERROR_INVALID_PARAM:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_INVALID_PARAM\n");
        errno = EINVAL;
        break;
    case LIBUSB_ERROR_ACCESS:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_ACCESS\n");
        errno = EACCES;
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_NO_DEVICE\n");
        errno = ENXIO;
        break;
    case LIBUSB_ERROR_NOT_FOUND:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_NOT_FOUND\n");
        errno = ENOENT;
        break;
    case LIBUSB_ERROR_BUSY:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_BUSY\n");
        errno = EBUSY;
        break;
    case LIBUSB_ERROR_TIMEOUT:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_TIMEOUT\n");
        errno = ETIMEDOUT;
        break;
    case LIBUSB_ERROR_OVERFLOW:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_OVERFLOW\n");
        errno = EOVERFLOW;
        break;
    case LIBUSB_ERROR_PIPE:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_PIPE\n");
        errno = EPIPE;
        break;
    case LIBUSB_ERROR_INTERRUPTED:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_INTERRUPTED\n");
        errno = EINTR;
        break;
    case LIBUSB_ERROR_NO_MEM:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_NO_MEM\n");
        errno = ENOMEM;
        break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_NOT_SUPPORTED\n");
        errno = ENOSYS;
        break;
    case LIBUSB_ERROR_OTHER:
        fprintf(stderr, "libusb error: LIBUSB_ERROR_OTHER\n");
        if (errno == 0) {
            errno = ENOSYS;
        }
        break;
    default:
        fprintf(stderr, "libusb error: Unexpected error code: %d.\n", r);
        if (errno == 0) {
            errno = ENOSYS;
        }
        break;
    }

    return -1;
}


static bool LJUSB_U3_isMinFirmware(struct LJUSB_FirmwareHardwareVersion * fhv)
{
    if (fhv->hardwareMajor == U3C_HARDWARE_MAJOR && fhv->hardwareMinor == U3C_HARDWARE_MINOR) {
        if (fhv->firmwareMajor > MIN_U3C_FIRMWARE_MAJOR || (fhv->firmwareMajor == MIN_U3C_FIRMWARE_MAJOR && fhv->firmwareMinor >= MIN_U3C_FIRMWARE_MINOR)) {
            return true;
        }
        else {
            fprintf(stderr, "Minimum U3 firmware not met is not met for this kernel.  Please update from firmware %d.%02d to firmware %d.%d or upgrade to kernel %d.%d.%d.\n", fhv->firmwareMajor, fhv->firmwareMinor, MIN_U3C_FIRMWARE_MAJOR, MIN_U3C_FIRMWARE_MINOR, LJ_RECENT_KERNEL_MAJOR, LJ_RECENT_KERNEL_MINOR, LJ_RECENT_KERNEL_REV);
            return false;
        }
    }
    else {
        fprintf(stderr, "Minimum U3 hardware version not met for this kernel.  This driver supports only hardware %d.%d and above.  Your hardware version is %d.%d.\n", U3C_HARDWARE_MAJOR, U3C_HARDWARE_MINOR, fhv->hardwareMajor, fhv->hardwareMinor);
        fprintf(stderr, "This hardware version is supported under kernel %d.%d.%d.\n", LJ_RECENT_KERNEL_MAJOR, LJ_RECENT_KERNEL_MINOR, LJ_RECENT_KERNEL_REV);
        return false;
    }

    return false;
}


static bool LJUSB_U6_isMinFirmware(struct LJUSB_FirmwareHardwareVersion * fhv)
{
    if (fhv->firmwareMajor > MIN_U6_FIRMWARE_MAJOR || (fhv->firmwareMajor == MIN_U6_FIRMWARE_MAJOR && fhv->firmwareMinor >= MIN_U6_FIRMWARE_MINOR)) {
        return true;
    }
    else {
        fprintf(stderr, "Minimum U6 firmware not met is not met for this kernel.  Please update from firmware %d.%d to firmware %d.%d or upgrade to kernel %d.%d.%d.\n", fhv->firmwareMajor, fhv->firmwareMinor, MIN_U6_FIRMWARE_MAJOR, MIN_U6_FIRMWARE_MINOR, LJ_RECENT_KERNEL_MAJOR, LJ_RECENT_KERNEL_MINOR, LJ_RECENT_KERNEL_REV);
        return false;
    }

    return false;
}


static bool LJUSB_UE9_isMinFirmware(struct LJUSB_FirmwareHardwareVersion * fhv)
{
    if (DEBUG) {
        fprintf(stderr, "In LJUSB_UE9_isMinFirmware\n");
    }

    if (fhv->firmwareMajor > MIN_UE9_FIRMWARE_MAJOR || (fhv->firmwareMajor == MIN_UE9_FIRMWARE_MAJOR && fhv->firmwareMinor >= MIN_UE9_FIRMWARE_MINOR)) {
        if (DEBUG) {
            fprintf(stderr, "Minimum UE9 firmware met. Version is %d.%d.\n", fhv->firmwareMajor, fhv->firmwareMinor);
        }
        return true;
    }
    else {
        fprintf(stderr, "Minimum UE9 firmware not met is not met for this kernel.  Please update from firmware %d.%d to firmware %d.%d or upgrade to kernel %d.%d.%d.\n", fhv->firmwareMajor, fhv->firmwareMinor, MIN_UE9_FIRMWARE_MAJOR, MIN_UE9_FIRMWARE_MINOR, LJ_RECENT_KERNEL_MAJOR, LJ_RECENT_KERNEL_MINOR, LJ_RECENT_KERNEL_REV);
        return false;
    }

    return false;
}


static bool LJUSB_isRecentKernel(void)
{
    unsigned long kernelMajor, kernelMinor, kernelRev;
    char * tok;
    struct utsname u;
    if (uname(&u) != 0) {
        fprintf(stderr, "Error calling uname(2).");
        return false;
    }

    // There are no known kernel-compatibility problems with Mac OS X.
    if (DEBUG) {
        fprintf(stderr, "LJUSB_recentKernel: sysname: %s.\n", u.sysname);
    }

    if (strncmp("Darwin", u.sysname, strlen("Darwin")) == 0) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_recentKernel: returning true on Darwin.\n");
        }
        return true;
    }

    if (DEBUG) {
        fprintf(stderr, "LJUSB_recentKernel: Kernel release: %s.\n", u.release);
    }
    tok = strtok(u.release, ".-");
    kernelMajor = strtoul(tok, NULL, 10);
    if (DEBUG) {
        fprintf(stderr, "LJUSB_recentKernel: tok: %s\n", tok);
        fprintf(stderr, "LJUSB_recentKernel: kernelMajor: %lu\n", kernelMajor);
    }
    tok = strtok(NULL, ".-");
    kernelMinor = strtoul(tok, NULL, 10);
    if (DEBUG) {
        fprintf(stderr, "LJUSB_recentKernel: tok: %s\n", tok);
        fprintf(stderr, "LJUSB_recentKernel: kernelMinor: %lu\n", kernelMinor);
    }
    tok = strtok(NULL, ".-");
    kernelRev = strtoul(tok, NULL, 10);
    if (DEBUG) {
        fprintf(stderr, "LJUSB_recentKernel: tok: %s\n", tok);
        fprintf(stderr, "LJUSB_recentKernel: kernelRev: %lu\n", kernelRev);
    }

    return (kernelMajor == LJ_RECENT_KERNEL_MAJOR && kernelMinor == LJ_RECENT_KERNEL_MINOR && kernelRev >= LJ_RECENT_KERNEL_REV) ||
           (kernelMajor == LJ_RECENT_KERNEL_MAJOR && kernelMinor > LJ_RECENT_KERNEL_MINOR) ||
           (kernelMajor > LJ_RECENT_KERNEL_MAJOR);
}


static bool LJUSB_isMinFirmware(HANDLE hDevice, unsigned long ProductID)
{
    struct LJUSB_FirmwareHardwareVersion fhv;

    // If we are running on a recent kernel, no firmware check is necessary.
    if (LJUSB_isRecentKernel()) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_isMinFirmware: LJUSB_isRecentKernel: true\n");
        }
        return true;
    }
    if (DEBUG) {
        fprintf(stderr, "LJUSB_isMinFirmware: LJUSB_isRecentKernel: false\n");
    }

    switch (ProductID) {
    case U3_PRODUCT_ID:
        LJUSB_U3_FirmwareHardwareVersion(hDevice, &fhv);
        return LJUSB_U3_isMinFirmware(&fhv);
    case U6_PRODUCT_ID:
        LJUSB_U6_FirmwareHardwareVersion(hDevice, &fhv);
        return LJUSB_U6_isMinFirmware(&fhv);
    case UE9_PRODUCT_ID:
        LJUSB_UE9_FirmwareHardwareVersion(hDevice, &fhv);
        return LJUSB_UE9_isMinFirmware(&fhv);
    case U12_PRODUCT_ID: //Add U12 stuff Mike F.
        return 1;
    case BRIDGE_PRODUCT_ID: //Add Wireless bridge stuff Mike F.
        return 1;
    default:
        fprintf(stderr, "Firmware check not supported for product ID %ld\n", ProductID);
        return false;
    }
}


static void LJUSB_libusb_exit(void)
{
    if (isLibUSBInitialized) {
        libusb_exit(ljContext);
        ljContext = NULL;
        isLibUSBInitialized = false;
    }
}


float LJUSB_GetLibraryVersion(void)
{
    return LJUSB_LINUX_LIBRARY_VERSION;
}


HANDLE LJUSB_OpenDevice(UINT DevNum, unsigned int dwReserved, unsigned long ProductID)
{
	(void)dwReserved;
	
    void * handle = NULL;
    struct libusb_device_handle *devh = NULL;
    libusb_device **devs;
    ssize_t cnt;
    int r = 1;
    libusb_device *dev;
    unsigned int i = 0;
    unsigned int ljFoundCount = 0;
    struct libusb_device_descriptor desc;

    if (!isLibUSBInitialized) {
        r = libusb_init(&ljContext);
        if (r < 0) {
            fprintf(stderr, "failed to initialize libusb\n");
            LJUSB_libusbError(r);
            return NULL;
        }
        isLibUSBInitialized = true;
    }

    cnt = libusb_get_device_list(ljContext, &devs);
    if (cnt < 0) {
        fprintf(stderr, "failed to get device list\n");
        LJUSB_libusbError(cnt);
        LJUSB_libusb_exit();
        return NULL;
    }

    while ((dev = devs[i++]) != NULL) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_OpenDevice: calling libusb_get_device_descriptor\n");
        }
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            LJUSB_libusbError(r);
            LJUSB_libusb_exit();
            return NULL;
        }

        if (LJ_VENDOR_ID == desc.idVendor && ProductID == desc.idProduct) {
            ljFoundCount++;
            if (ljFoundCount == DevNum) {            
                // Found the one requested
                r = libusb_open(dev, &devh);
                if (r < 0) {
                    LJUSB_libusbError(r);
                    return NULL;
                }

                // Test if the kernel driver has the device.
                // Should only be true for HIDs.
                if (libusb_kernel_driver_active(devh, 0)) {
                    if (DEBUG) {
                        fprintf(stderr, "Kernel Driver was active, detaching...\n");
                    }
                    
                    // Detaches U12s from kernel driver.
                    r = libusb_detach_kernel_driver(devh, 0);
                    
                    // Check the return value
                    if ( r != 0 ) {
                        fprintf(stderr, "failed to detach from kernel driver. Error Number: %i", r);
                        return NULL;
                    }
                }

                r = libusb_claim_interface(devh, 0);
                if (r < 0) {
                    LJUSB_libusbError(r);
                    libusb_close(devh);
                    return NULL;
                }
                handle = (void *) devh;
                if (DEBUG) {
                    fprintf(stderr, "LJUSB_OpenDevice: Found handle for product ID %ld\n", ProductID);
                }
                break;
            }
        }
    }
    libusb_free_device_list(devs, 1);

    if (handle != NULL) {
        //We foud a device, now check if it meets the minimum firmware requirement
        if (!LJUSB_isMinFirmware(handle, ProductID)) {
            //Does not meet the requirement.  Close device and return an invalid handle.
            LJUSB_CloseDevice(handle);
            return NULL;
        }
    }
    if (DEBUG) {
       fprintf(stderr, "LJUSB_OpenDevice: Returning handle\n");
    }
    return handle;
}


int LJUSB_OpenAllDevices(HANDLE* devHandles, UINT* productIds, UINT maxDevices)
{	
    void * handle = NULL;
    struct libusb_device_handle *devh = NULL;
    libusb_device **devs;
    ssize_t cnt;
    int r = 1;
    libusb_device *dev;
    unsigned int i = 0;
    unsigned int ljFoundCount = 0;
    struct libusb_device_descriptor desc;

    if (!isLibUSBInitialized) {
        r = libusb_init(&ljContext);
        if (r < 0) {
            fprintf(stderr, "failed to initialize libusb\n");
            LJUSB_libusbError(r);
            return -1;
        }
        isLibUSBInitialized = true;
    }

    cnt = libusb_get_device_list(ljContext, &devs);
    if (cnt < 0) {
        fprintf(stderr, "failed to get device list\n");
        LJUSB_libusbError(cnt);
        LJUSB_libusb_exit();
        return -1;
    }

    while ((dev = devs[i++]) != NULL) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_OpenDevice: calling libusb_get_device_descriptor\n");
        }
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            LJUSB_libusbError(r);
            LJUSB_libusb_exit();
            return -1;
        }

        if (LJ_VENDOR_ID == desc.idVendor) {
            // Found a LabJack device
            r = libusb_open(dev, &devh);
            if (r < 0) {
                LJUSB_libusbError(r);
                continue;
            }

            // Test if the kernel driver has the device.
            // Should only be true for HIDs.
            if (libusb_kernel_driver_active(devh, 0) ) {
                if (DEBUG) {
                    fprintf(stderr, "Kernel Driver was active, detaching...\n");
                }
                
                // Detaches U12s from kernel driver.
                r = libusb_detach_kernel_driver(devh, 0);
                
                // Check the return value
                if ( r != 0 ) {
                    fprintf(stderr, "failed to detach from kernel driver. Error Number: %i", r);
                    libusb_close(devh);
                    continue;
                }
            }

            r = libusb_claim_interface(devh, 0);
            if (r < 0) {
                //LJUSB_libusbError(r);
                libusb_close(devh);
                continue;
            }
            
            handle = (void *) devh;
            
            if (handle == NULL) {
                // Not a valid handle
                continue;
            }
            else if (ljFoundCount < maxDevices) {
                if (LJUSB_isMinFirmware(handle, desc.idProduct)) {
                    devHandles[ljFoundCount] = handle;
                    productIds[ljFoundCount] = desc.idProduct;
                    ljFoundCount++;
                } else {
                    // Not high enough firmware, keep moving.
                    libusb_close(devh);
                    ljFoundCount--;
                }
            } else {
                // Too many devices have been found.
                libusb_close(devh);
                break;
            }
        }
    }
    libusb_free_device_list(devs, 1);

    return ljFoundCount;
}


static int LJUSB_handleBulkTranferError(int r)
{
    return LJUSB_libusbError(r);
}


static unsigned long LJUSB_DoTransfer(HANDLE hDevice, unsigned char endpoint, BYTE *pBuff, unsigned long count, bool isBulk)
{
    int r;
    int transferred;

    if (DEBUG) {
        fprintf(stderr, "Calling LJUSB_DoTransfer with endpoint = 0x%x, count = %lu, and isBulk = %d.\n", endpoint, count, isBulk);
    }

    if (LJUSB_IsHandleValid(hDevice) == false) {
        if (DEBUG) {
            fprintf(stderr, "Calling LJUSB_DoTransfer returning -1 because handle is invalid. errno = %d.\n", errno);
        }
        return -1;
    }
    
    if (isBulk && endpoint != 1 && endpoint < 0x81 ) {
        fprintf(stderr, "LJUSB_DoTransfer warning: Got endpoint = %d, however this not a known endpoint. Please verify you are using the header file provided in /usr/local/include/labjackusb.h and not an older header file.\n", endpoint);
    }

    if (isBulk) {
        r = libusb_bulk_transfer(hDevice, endpoint, pBuff, count, &transferred, LJ_LIBUSB_TIMEOUT);
    }
    else {
        r = libusb_interrupt_transfer(hDevice, endpoint, pBuff, count, &transferred, LJ_LIBUSB_TIMEOUT);
    }

    if (r == LIBUSB_ERROR_TIMEOUT && !isBulk) {
        // We time out a lot using interrupt transfers, so there's no reason to
        // cry about it. Just set the errno, and move on.
        if (DEBUG) {
            fprintf(stderr, "LJUSB_DoTransfer: Interrupt transfer timed out. Returning.\n");
        }
        errno = ETIMEDOUT;
        return -1;
    }
    else if (r != 0) {
        return LJUSB_handleBulkTranferError(r);
    }

    if (DEBUG) {
        fprintf(stderr, "LJUSB_DoTransfer: returning transferred = %d.\n", transferred);
    }

    return transferred;
}


// Automatically uses the correct endpoint and transfer method (bulk or interrupt)
static unsigned long LJUSB_SetupTransfer(HANDLE hDevice, BYTE *pBuff, unsigned long count, enum LJUSB_TRANSFER_OPERATION operation)
{
    unsigned char endpoint;
    bool isBulk;
    int r;
    
    if (DEBUG) {
        fprintf(stderr, "Calling LJUSB_SetupTransfer with count = %lu and operation = %d.\n", count, operation);
    }
    
    libusb_device *dev;
    struct libusb_device_descriptor desc;

    //First determine the device from handle.
    dev = libusb_get_device(hDevice);
    r = libusb_get_device_descriptor(dev, &desc);
    
    if (r < 0) {
        LJUSB_libusbError(r);
        return -1;
    }
    
    
    switch (desc.idProduct) {

    /* These devices use bulk transfers */
    case UE9_PRODUCT_ID:
        isBulk = true;
        switch (operation) {
        case LJUSB_WRITE:
            endpoint = UE9_PIPE_EP1_OUT;
            break;
        case LJUSB_READ:
            endpoint = UE9_PIPE_EP1_IN;
            break;
        case LJUSB_STREAM:
            endpoint = UE9_PIPE_EP2_IN;
            break;
        default:
            errno = EINVAL;
            return -1;
        }
        break;
    case U3_PRODUCT_ID:
        isBulk = true;
        switch(operation) {
        case LJUSB_WRITE:
            endpoint = U3_PIPE_EP1_OUT;
            break;
        case LJUSB_READ:
            endpoint = U3_PIPE_EP2_IN;
            break;
        case LJUSB_STREAM:
            endpoint = U3_PIPE_EP3_IN;
            break;
        default:
            errno = EINVAL;
            return -1;
        }
        break;
    case U6_PRODUCT_ID:
        isBulk = true;
        switch (operation) {
        case LJUSB_WRITE:
            endpoint = U6_PIPE_EP1_OUT;
            break;
        case LJUSB_READ:
            endpoint = U6_PIPE_EP2_IN;
            break;
        case LJUSB_STREAM:
            endpoint = U6_PIPE_EP3_IN;
            break;
        default:
            errno = EINVAL;
            return -1;
        }
        break;

    /* These devices use interrupt transfers */
    case U12_PRODUCT_ID:
        isBulk = false;
        switch(operation) {
        case LJUSB_READ:
            endpoint = U12_PIPE_EP1_IN;
            break;
        case LJUSB_WRITE:
            endpoint = U12_PIPE_EP2_OUT;
            break;
        case LJUSB_STREAM:
        default:
            // U12 has no streaming interface
            errno = EINVAL;
            return -1;
        }
        break;
    case BRIDGE_PRODUCT_ID:
        isBulk = false;
        switch (operation) {
        case LJUSB_READ:
            endpoint = BRIDGE_PIPE_EP1_IN;
            break;
        case LJUSB_WRITE:
            endpoint = BRIDGE_PIPE_EP1_OUT;
            break;
        case LJUSB_STREAM:
        default:
            // SkyMote has no streaming interface
            errno = EINVAL;
            return -1;
        }
        break;

    default:
        // Error, not a labjack device
        errno = EINVAL;
        return -1;
    }

    return LJUSB_DoTransfer(hDevice, endpoint, pBuff, count, isBulk);
}


// Deprecated: Kept for backwards compatibility
unsigned long LJUSB_BulkRead(HANDLE hDevice, unsigned char endpoint, BYTE *pBuff, unsigned long count)
{
    return LJUSB_DoTransfer(hDevice, endpoint, pBuff, count, true);
}


// Deprecated: Kept for backwards compatibility
unsigned long LJUSB_BulkWrite(HANDLE hDevice, unsigned char endpoint, BYTE *pBuff, unsigned long count)
{
    return LJUSB_DoTransfer(hDevice, endpoint, pBuff, count, true);
}


unsigned long LJUSB_Write(HANDLE hDevice, BYTE *pBuff, unsigned long count)
{
    if (DEBUG) {
        fprintf(stderr, "LJUSB_Write: calling LJUSB_Write.\n");
    }
    return LJUSB_SetupTransfer(hDevice, pBuff, count, LJUSB_WRITE);
}


unsigned long LJUSB_Read(HANDLE hDevice, BYTE *pBuff, unsigned long count)
{
    if (DEBUG) {
        fprintf(stderr, "LJUSB_Read: calling LJUSB_Read.\n");
    }
    return LJUSB_SetupTransfer(hDevice, pBuff, count, LJUSB_READ);
}


unsigned long LJUSB_Stream(HANDLE hDevice, BYTE *pBuff, unsigned long count)
{
    if (DEBUG) {
        fprintf(stderr, "LJUSB_Stream: calling LJUSB_Stream.\n");
    }
    return LJUSB_SetupTransfer(hDevice, pBuff, count, LJUSB_STREAM);
}


void LJUSB_CloseDevice(HANDLE hDevice)
{
    if (DEBUG) {
        fprintf(stderr, "LJUSB_CloseDevice\n");
    }

    if (LJUSB_isNullHandle(hDevice)) {
        return;
    }

    //Release
    libusb_release_interface(hDevice, 0);

    //Close
    libusb_close(hDevice);
    if (DEBUG) {
        fprintf(stderr, "LJUSB_CloseDevice: closed\n");
    }
}


unsigned int LJUSB_GetDevCount(unsigned long ProductID)
{
    libusb_device **devs;
    ssize_t cnt;
    int r = 1;

    if (!isLibUSBInitialized) {
        r = libusb_init(&ljContext);
        if (r < 0) {
            fprintf(stderr, "failed to initialize libusb\n");
            LJUSB_libusbError(r);
            return 0;
        }
        isLibUSBInitialized = true;
    }

    cnt = libusb_get_device_list(ljContext, &devs);
    if (cnt < 0) {
        fprintf(stderr, "failed to get device list\n");
        LJUSB_libusbError(cnt);
        LJUSB_libusb_exit();
        return 0;
    }

    libusb_device *dev;
    unsigned int i = 0;
    unsigned int ljFoundCount = 0;

    // Loop over all USB devices and count the ones with the LabJack
    // vendor ID and the passed in product ID.
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            LJUSB_libusbError(r);
            LJUSB_libusb_exit();
            return 0;
        }
        if (LJ_VENDOR_ID == desc.idVendor && ProductID == desc.idProduct) {
            ljFoundCount++;
        }
    }
    libusb_free_device_list(devs, 1);

    return ljFoundCount;
}


int LJUSB_GetDevCounts(UINT *productCounts, UINT * productIds, UINT n)
{
    libusb_device **devs;
    ssize_t cnt;
    int r = 1;

    if (!isLibUSBInitialized) {
        r = libusb_init(&ljContext);
        if (r < 0) {
            fprintf(stderr, "failed to initialize libusb\n");
            LJUSB_libusbError(r);
            return 0;
        }
        isLibUSBInitialized = true;
    }

    cnt = libusb_get_device_list(ljContext, &devs);
    if (cnt < 0) {
        fprintf(stderr, "failed to get device list\n");
        LJUSB_libusbError(cnt);
        LJUSB_libusb_exit();
        return 0;
    }

    libusb_device *dev;
    UINT i = 0;
    UINT u3ProductCount = 0;
    UINT u6ProductCount = 0;
    UINT ue9ProductCount = 0;
    UINT u12ProductCount = 0;
    UINT bridgeProductCount = 0;

    // Loop over all USB devices and count the ones with the LabJack
    // vendor ID and the passed in product ID.
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor\n");
            LJUSB_libusbError(r);
            LJUSB_libusb_exit();
            return 0;
        }
        if (LJ_VENDOR_ID == desc.idVendor) {
            switch (desc.idProduct) {
            case U3_PRODUCT_ID:
                u3ProductCount++;
                break;
            case U6_PRODUCT_ID:
                u6ProductCount++;
                break;
            case UE9_PRODUCT_ID:
                ue9ProductCount++;
                break;
            case U12_PRODUCT_ID:
                u12ProductCount++;
                break;
            case BRIDGE_PRODUCT_ID:
                bridgeProductCount++;
                break;
            }
        }
    }
    libusb_free_device_list(devs, 1);

    for (i = 0; i < n; i++) {
        switch (i) {
        case 0:
            productCounts[i] = u3ProductCount;
            productIds[i] = U3_PRODUCT_ID;
            break;
        case 1:
            productCounts[i] = u6ProductCount;
            productIds[i] = U6_PRODUCT_ID;
            break;
        case 2:
            productCounts[i] = ue9ProductCount;
            productIds[i] = UE9_PRODUCT_ID;
            break;
        case 3:
            productCounts[i] = u12ProductCount;
            productIds[i] = U12_PRODUCT_ID;
            break;
        case 4:
            productCounts[i] = bridgeProductCount;
            productIds[i] = BRIDGE_PRODUCT_ID;
            break;
        }
    }

    return 0;
}


bool LJUSB_IsHandleValid(HANDLE hDevice)
{
    uint8_t config = 0;
    int r = 1;

    if (LJUSB_isNullHandle(hDevice)) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_IsHandleValid: returning 0. hDevice is NULL.\n");
        }
        // TODO: Consider different errno here and in LJUSB_isNullHandle
        errno = EINVAL;
        return false;
    }

    // If we can call get configuration without getting an error,
    // the handle is still valid.
    // Note that libusb_get_configuration() will return a cached value,
    // so we replace this call
    // r = libusb_get_configuration(hDevice, &config);
    // to the actual control tranfser, from the libusb source
    r = libusb_control_transfer(hDevice, LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_CONFIGURATION, 0, 0, &config, 1, 1000);
    if (r < 0) {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_IsHandleValid: returning 0. Return value from libusb_get_configuration was: %d\n", r);
        }
        // TODO: Consider different errno here and in LJUSB_isNullHandle
        errno = EINVAL;
        return false;
    } else {
        if (DEBUG) {
            fprintf(stderr, "LJUSB_IsHandleValid: returning 1.\n");
        }
        return true;
    }
}


//not supported
bool LJUSB_AbortPipe(HANDLE hDevice, unsigned long Pipe)
{
	(void)hDevice;
	(void)Pipe;

    errno = ENOSYS;
    return false;
}
