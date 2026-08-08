#ifndef BYBYTE_CORE_H
#define BYBYTE_CORE_H

// ByByteLib internal foundation umbrella.
//
// Pulled in by public module headers to get the shared platform foundation
// (Types, PinCapabilities, platform auto-detection, per-board pin defaults
// and product/kit types). Internal (lives under src/core/); users include the
// public per-module headers or the aggregate <ByByteLib.h> instead.
//
// Includes use the relative "..." form so they resolve from this file's
// location regardless of -I paths, which keeps Arduino IDE and PlatformIO
// resolver behavior identical.

#include "Types.h"
#include "PinCapabilities.h"
#include "core/configs/PlatformDetect.h"
#include "core/configs/ByByteConfig.h"
#include "core/configs/ByByteProduct.h"

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	#include "core/configs/BbPinsByByteMega.h"
#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
	#include "core/configs/BbPinsByByteNano.h"
#endif

#endif // BYBYTE_CORE_H
