#pragma once

#include <wdm.h>
#include <ntddser.h>

#include "serial.h"
#include "trace.h"

#define SYMBOLIC_LINK_NAME_LENGTH   32
#define SYMBOLIC_LINK_NAME_PREFIX   L"\\DosDevices\\Global\\"
#define REG_PATH_DEVICEMAP          L"HARDWARE\\DEVICEMAP"
#define SERIAL_DEVICE_MAP           L"SERIALCOMM"
#define REG_VALUENAME_PORTNAME      L"PortName"
#define REG_PATH_SERIAL_COMM        REG_PATH_DEVICEMAP L"\\" SERIAL_DEVICE_MAP

/**
 * @brief Struct that contains device parameters.
 * 
 * Virtual device context - struct that contains virtual device params and data.
*/
typedef struct vdevice_context {
    /**
     * @brief A handle of device object.
     * 
     * It's a handle to a device object.
    */
    WDFDEVICE   device;

    /**
     * @brief Baud rate. Not using.
     * 
     * Not using. Need only to emulation.
    */
    ULONG               baudrate;

    ULONG               modem_control_register;

    ULONG               fifo_control_register;

    ULONG               line_control_register;

    UCHAR               valid_data_mask;

    serial_timeouts_t   timeouts;

    BOOLEAN             is_lhk_created;

    /**
     * @brief Physical device object name.
     * 
     * Represents phycical device name(It seems like a device ID).
    */
    PWSTR       pdo_name;
} vdevice_context_t, *p_vdevice_context_t;

/** Here we create getter for device context. More here: https://learn.microsoft.com/ru-ru/windows-hardware/drivers/wdf/wdf-declare-context-type-with-name */
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(vdevice_context_t, get_vdevice_context);


/**
 * @brief Create and initialize an instance of the device.
 * 
 * Creating and initializing an instance of the cpd's device callback object.
 * 
 * @param[in] driver The handle of driver.
 * @param[in] device_init The initializing struct. Is used for wdk functions.
 * @param[out] device_context Pointer to device context.
 * 
 * @return Status of function execution.
*/
NTSTATUS
device_create(
    _In_    WDFDRIVER               driver,
    _In_    PWDFDEVICE_INIT         device_init,
    _Out_   p_vdevice_context_t*    device_context
);

/**
 * @brief Function device configuring.
 * 
 * This function is called after the device callback object has been initialized
 * and returned to the driver. It would setup the device's queues and their
 * corresponding callback objects.
 * 
 * @param[in] device_context Device data object for which we handling events.
 * 
 * @return Status of function execution.
*/
NTSTATUS
device_configure(
    _In_    p_vdevice_context_t device_context
);

/**
 * @brief Get physical device object name.
 * 
 * Get physical device object name and save it in device_context data structure.
 * 
 * @param[in] device_context Device data object.
 * 
 * @return Status of function execution.
*/
NTSTATUS
device_get_pdoname(
    _In_    p_vdevice_context_t device_context
);

/**
 * @brief Write using com port to registry.
 * 
 * Write hardware key to registry that contain pdo name and using com port.
 * 
 * @param[in] pdo_name Physical device object name.
 * @param[in] com_port Com port which we want to use.
 * @param[in] device Handle of device.
 * 
 * @return Status of function execution.
*/
NTSTATUS
device_write_legacy_hardwarekey(
    _In_    PWSTR       pdo_name,
    _In_    PWSTR       com_port,
    _In_    WDFDEVICE   device
);

/**
 * @brief Cleanup callback function.
 * 
 * Callback function that calling when device was removed.
 * 
 * @param[in] object represent object that must be removed. In our case WDFDEVICE.
*/
EVT_WDF_DEVICE_CONTEXT_CLEANUP evt_device_cleanup;

/**
 * @brief Returning baudrate value.
 * 
 * Returning baudrate value.
 * 
 * @param[in] device_context Device data object.
 * 
 * @return Baudrate.
*/
ULONG
get_baudrate(
    _In_    p_vdevice_context_t device_context
);

/**
 * @brief Set baudrate value
 * 
 * Set baudrate value.
 * 
 * @param[in] device_context Device data object.
 * @param[in] baudrate Baudrate value.
*/
VOID
set_baudrate(
    _In_    p_vdevice_context_t device_context,
    _In_    ULONG               baudrate
);

/**
 * @brief Get pointer to FCR.
 * 
 * FCR(First in, first out Control Register) - using to buffering and flow control between hardware and software.
 * See more: https://www.lookrs232.com/rs232/fcr.htm
 * 
 * @param[in] device_context Device data object.
 * 
 * @return Pointer to FCR.
*/
ULONG*
get_fifo_control_register_ptr(
    _In_    p_vdevice_context_t device_context
);

/**
 * @brief Get pointer to LCR
 * 
 * Line Control register(LCR) - sets the general connection parameters.
 * See more: https://www.lookrs232.com/rs232/lcr.htm
 * 
 * @param[in] device_context Device data object
 * 
 * @return Pointer to LCR
*/
ULONG*
get_line_control_register_ptr(
    _In_    p_vdevice_context_t device_context
);

/* TODO: add more info about data mask. */
/**
 * @brief Set valid data mask.
 * 
 * Add more info.
 * 
 * @param[in] device_context Device data object.
 * @param[in] mask Data mask.
*/
VOID
set_valid_data_mask(
    _In_    p_vdevice_context_t device_context,
    _In_    UCHAR               mask
);

/**
 * @brief Set port timeouts.
 * 
 * Set port's time to wait data.
 * 
 * @param[in] device_context Device data object.
 * @param[in] timeouts Object that contain timeouts.
*/
VOID
set_timeouts(
    _In_    p_vdevice_context_t device_context,
    _In_    serial_timeouts_t     timeouts
);

/**
 * @brief Get port timeouts.
 * 
 * Get port timeouts data.
 * 
 * @param[in] device_context Device data object.
 * @param[out] timeouts Pointer to struct that contains timeouts.
*/
VOID
get_timeouts(
    _In_    p_vdevice_context_t device_context,
    _Out_   serial_timeouts_t*  timeouts
);