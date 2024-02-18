#pragma once

/**
*   
*   Заголовок объявления callback функций
*
*/
#include <wdm.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;

EVT_WDF_DRIVER_UNLOAD DriverUnload;