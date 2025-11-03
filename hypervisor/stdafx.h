#pragma once

#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include <stdarg.h>
#include <stddef.h>
#include <intrin.h>

// common macros
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#include "includes/hv_ioctl.h"
#include "includes/hv_logger.h"
#include "includes/hv_driver.h"
#include "includes/hv_vmx.h"
#include "includes/hv_device.h"
#include "includes/hv_ept.h"
#include "includes/hv_sandbox.h"