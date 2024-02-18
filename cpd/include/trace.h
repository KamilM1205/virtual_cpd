#pragma once

#include <ntddk.h>
#include <wdf.h>

/**
 * @brief Trace without formatting.
 * 
 * Writting message to Windows kernel log.
 * 
 * @param[in] msg message that must be written to kernel log.
*/
#define TRACEI(msg)     \
    DbgPrint(msg "\n")


/**
 * @brief Trace with log level and formatting.
 * 
 * Writtin formatted message with trace level to kernel log.
 * 
 * @param[in] level Level of trace message.
 * @param[in] _fmt_ Formatted string.
 * @param[in] vargs Variables represented in the format string.
*/
#define TRACE(level, _fmt_, ...)    \
    DbgPrintEx(DPFLTR_DEFAULT_ID, level, _fmt_ "\n", __VA_ARGS__)

#define TRACE_LEVEL_INFO    DPFLTR_INFO_LEVEL
#define TRACE_LEVEL_ERROR   DPFLTR_ERROR_LEVEL