#pragma once

typedef struct serial_timeouts {
    ULONG   read_interval_timeout;
    ULONG   read_total_timeout_multiplier;
    ULONG   read_total_timeout_constant;
    ULONG   write_total_timeout_multiplier;
    ULONG   write_total_timeout_constant;
} serial_timeouts_t, *p_serial_timeouts_t;