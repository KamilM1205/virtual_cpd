#include "virtual_device.h"

NTSTATUS
device_create(
    _In_    WDFDRIVER               driver,
    _In_    PWDFDEVICE_INIT         device_init,
    _Out_   p_vdevice_context_t*    device_context
) {
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   device_attr;
    WDFDEVICE               device;
    p_vdevice_context_t     device_ctx;
    UNREFERENCED_PARAMETER  (driver);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &device_attr,
        vdevice_context_t
    );

    device_attr.SynchronizationScope = WdfSynchronizationScopeDevice;
    device_attr.EvtCleanupCallback   = evt_device_cleanup;

    status = WdfDeviceCreate(
        &device_init,
        &device_attr,
        &device
        );
    if (!NT_SUCCESS(status)) {
        TRACE(TRACE_LEVEL_ERROR, "Error: WdfDeviceCreate failed 0x%x", status);
        return status;
    }

    device_ctx = get_vdevice_context(device);
    device_ctx->device = device;

    *device_context = device_ctx;

    return status;
}

NTSTATUS
device_configure(
    _In_    p_vdevice_context_t device_context
) {
    NTSTATUS    status;
    WDFDEVICE   device = device_context->device;
    WDFKEY      key;
    LPGUID      guid;
    errno_t     error_no;

    DECLARE_CONST_UNICODE_STRING(port_name,         REG_VALUENAME_PORTNAME);
    DECLARE_UNICODE_STRING_SIZE(com_port,           10);
    DECLARE_UNICODE_STRING_SIZE(symbolic_link_name, SYMBOLIC_LINK_NAME_LENGTH);

    guid = (LPGUID) &GUID_DEVINTERFACE_COMPORT;

    /* Create device interface */
    status = WdfDeviceCreateDeviceInterface(
        device,
        guid,
        NULL
    );
    if (!NT_SUCCESS(status)) {
        TRACE(TRACE_LEVEL_ERROR, "Error: Cannot create device interface: 0x%x.", status);
        goto exit;
    }

    /*
    * Read the COM port number from the registry, which has been automatically
    * created by "MsPorts!PortsClassInstaller" if INF file says "Class=Ports"
    */
    status = WdfDeviceOpenRegistryKey(
        device,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_QUERY_VALUE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &key
    );
    if (!NT_SUCCESS(status)) {
        TRACE(TRACE_LEVEL_ERROR, "Error: Failed to retrieve device hardware key root: 0x%x.", status);
        goto exit;
    }

    status = WdfRegistryQueryUnicodeString(
        key,
        &port_name,
        NULL,
        &com_port
    );
    if (!NT_SUCCESS(status)) {
        TRACE(TRACE_LEVEL_ERROR, "Error: Failed to read port name: 0x%x.", status);
        goto exit;
    }

    /*
    * Manually create the symbolic link name. Length is the length in
    * bytes not including the NULL terminator.
    *
    * 6054 and 26035 are code analysis warnings that comPort.Buffer might
    * not be NULL terminated, while we know that they are. 
    */
    #pragma warning(suppress: 6054 26035)
    symbolic_link_name.Length = (USHORT)((wcslen(com_port.Buffer) *sizeof(wchar_t))
        + sizeof(SYMBOLIC_LINK_NAME_PREFIX) - sizeof(UNICODE_NULL));
    
    if (symbolic_link_name.Length >= symbolic_link_name.MaximumLength) {
        TRACE(
            TRACE_LEVEL_ERROR, "Error: Buffer overflow when creating COM port name. Size"
            " is %d, buffer length is %d", symbolic_link_name.Length, symbolic_link_name.MaximumLength
        );
        status = STATUS_BUFFER_OVERFLOW;
        goto exit;
    }

    error_no = wcscat_s(symbolic_link_name.Buffer,
        SYMBOLIC_LINK_NAME_LENGTH,
        com_port.Buffer
    );

    if (error_no != 0) {
        TRACE(
            TRACE_LEVEL_ERROR,
            "Failed to copy %ws to buffer with error %d",
            com_port.Buffer, error_no
        );
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    /*
    * Create symbolic link
    */
    status = WdfDeviceCreateSymbolicLink(
        device,
        &symbolic_link_name
    );
    if (!NT_SUCCESS(status)) {
        TRACE(
            TRACE_LEVEL_ERROR,
            "Error: Cannot create symbolic link %ws",
            symbolic_link_name.Buffer
        );
        goto exit;
    }

    status = device_get_pdoname(device_context);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    status = device_write_legacy_hardwarekey(
        device_context->pdo_name,
        com_port.Buffer,
        device_context->device
    );
    if (NT_SUCCESS(status)) {
        device_context->is_lhk_created = TRUE;
    }

    /*status = queue_create(device_context);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }*/

    exit:
        return status;
}

NTSTATUS
device_get_pdoname(
    _In_ p_vdevice_context_t    device_context
) {
    NTSTATUS                status;
    WDFDEVICE               device = device_context->device;
    WDF_OBJECT_ATTRIBUTES   attr;
    WDFMEMORY               memory;

    WDF_OBJECT_ATTRIBUTES_INIT(&attr);
    attr.ParentObject = device;

    status = WdfDeviceAllocAndQueryProperty(
        device,
        DevicePropertyPhysicalDeviceObjectName,
        NonPagedPoolNx,
        &attr,
        &memory
    );
    if (!NT_SUCCESS(status)) {
        TRACE(
            TRACE_LEVEL_ERROR,
            "Error: Failed to query PDO name: 0x%x.",
            status     
        );
        goto exit;
    }

    device_context->pdo_name = (PWSTR) WdfMemoryGetBuffer(memory, NULL);
    TRACE(TRACE_LEVEL_INFO, "PDO name is %ws: ", device_context->pdo_name);

    exit:
        return status;
}

NTSTATUS
device_write_legacy_hardwarekey(
    _In_    PWSTR       pdo_name,
    _In_    PWSTR       com_port,
    _In_    WDFDEVICE   device
) {
    NTSTATUS        status;
    WDFKEY          key = NULL;
    UNICODE_STRING  pdo_string = {0};
    UNICODE_STRING  com_port_string = {0};

    DECLARE_CONST_UNICODE_STRING(device_subkey, SERIAL_DEVICE_MAP);

    RtlInitUnicodeString(&pdo_string, pdo_name);
    RtlInitUnicodeString(&com_port_string, com_port);

    status = WdfDeviceOpenDevicemapKey(
        device,
        &device_subkey,
        KEY_SET_VALUE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &key
    );
    if (!NT_SUCCESS(status)) {
        TRACE(
            TRACE_LEVEL_ERROR, 
            "Error: Failed to open DEVICEMAP\\SERIALCOMM key 0x%x",
            status
        );
        goto exit;
    }

    status = WdfRegistryAssignUnicodeString(
        key,
        &pdo_string,
        &com_port_string
    );
    if (NT_SUCCESS(status)) {
        TRACE(
            TRACE_LEVEL_ERROR,
            "Error: Failed to write to DEVICEMAP\\SERIALCOMM key 0x%x",
            status
        );
        goto exit;
    }

    exit:

        if (key != NULL) {
            WdfRegistryClose(key);
            key = NULL;
        }

        return status;
}

VOID
evt_device_cleanup(
    _In_    WDFOBJECT   object
) {
    WDFDEVICE           device = (WDFDEVICE) object;
    p_vdevice_context_t device_context = get_vdevice_context(device);
    NTSTATUS            status;
    WDFKEY              key = NULL;
    UNICODE_STRING      pdo_string = {0};

    DECLARE_CONST_UNICODE_STRING(device_subkey, SERIAL_DEVICE_MAP);

    if (device_context->is_lhk_created == TRUE) {
        RtlInitUnicodeString(&pdo_string, device_context->pdo_name);

        status = WdfDeviceOpenDevicemapKey(
            device,
            &device_subkey,
            KEY_SET_VALUE,
            WDF_NO_OBJECT_ATTRIBUTES,
            &key
        );
        if (!NT_SUCCESS(status)) {
            TRACE(
                TRACE_LEVEL_ERROR,
                "Error: Failed to open DEVICEMAP\\SERIALCOMM key 0x%x",
                status
            );
            goto exit;
        }

        status = WdfRegistryRemoveValue(
            key,
            &pdo_string
        );
        if (!NT_SUCCESS(status)) {
            TRACE(
                TRACE_LEVEL_ERROR,
                "Error: Failed to remove %S key, 0x%x",
                pdo_string.Buffer,
                status
            );
            goto exit;
        }

        status = WdfRegistryRemoveKey(key);
        if (!NT_SUCCESS(status)) {
            TRACE(
                TRACE_LEVEL_ERROR,
                "Error: Failed to remove %S, 0x%x",
                SERIAL_DEVICE_MAP,
                status
            );
            goto exit;
        }
    }

    exit:
        if (key != NULL) {
            WdfRegistryClose(key);
            key = NULL;
        }

        return;
}

ULONG
get_baudrate(
    _In_    p_vdevice_context_t device_context
) {
    return device_context->baudrate;
}

VOID
set_baudrate(
    _In_    p_vdevice_context_t device_context,
    _In_    ULONG               baudrate
) {
    device_context->baudrate = baudrate;
}

ULONG*
get_fifo_control_register_ptr(
    _In_    p_vdevice_context_t device_context
) {
    return &device_context->fifo_control_register;
}

ULONG*
get_line_control_register_ptr(
    _In_    p_vdevice_context_t device_context
) {
    return &device_context->line_control_register;
}

VOID
set_valid_data_mask(
    _In_    p_vdevice_context_t device_context,
    _In_    UCHAR               mask
) {
    device_context->valid_data_mask = mask;
}

VOID
set_timeouts(
    _In_    p_vdevice_context_t device_context,
    _In_    serial_timeouts_t   timeouts
) {
    device_context->timeouts = timeouts;
}

VOID
get_timeouts(
    _In_    p_vdevice_context_t device_context,
    _Out_   serial_timeouts_t*  timeouts
) {
    *timeouts = device_context->timeouts;
}