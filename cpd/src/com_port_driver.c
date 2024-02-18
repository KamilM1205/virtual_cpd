#include <wdm.h>
#include <wdf.h>
#include <initguid.h>

#include <stdio.h>

#include "com_port_driver.h"
#include "trace.h"
#include "virtual_device.h"

/**
 * @brief Driver unload callback.
 * 
 * That callback function executing when windows unloading driver.
 * 
 * @param[in] driver Handle to a driver.
*/
VOID 
DriverUnload(_In_ WDFDRIVER driver) {
	UNREFERENCED_PARAMETER(driver);

	TRACEI("Driver was unloaded");	
}

/**
 * @brief Driver entry function.
 * 
 * That callback function executing when windows loading driver.
 * 
 * @param[in] driver_object Pointer to driver object.
 * @param[in] registry_path Path to the driver's parameters key in the registry.
 * 
 * @return Status of function execution
*/
NTSTATUS 
DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) {
	WDF_DRIVER_CONFIG config;
	NTSTATUS status;
	WDF_OBJECT_ATTRIBUTES attributes;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	WDF_DRIVER_CONFIG_INIT(&config, NULL);

	status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	TRACEI("Driver was initialized");

	return STATUS_SUCCESS;
}

/**
 * @brief Device add callback.
 * 
 * That callback function executing when PnP dispatcher detected new device.
 * 
 * @param[in] driver Handle of to a driver.
 * @param[in] device_init The initializing struct. Used for wdk functions.
 * 
 * @return Status of function execution
*/
NTSTATUS 
EvtDeviceAdd(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT device_init) {
	NTSTATUS 			status;
	p_vdevice_context_t	device_context;

	status = device_create(driver, device_init, &device_context);
	if (!NT_SUCCESS(status)) {
		goto error;
	}

	status = device_configure(device_context);
	if (!NT_SUCCESS(status)) {
		goto error;
	}

	return status;

	error:
	TRACEI("Errors occured while execute evt_device_add callback.");
	return status;
}