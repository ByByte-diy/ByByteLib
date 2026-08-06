#ifndef BYBYTE_CORE_PUBLIC_H
#define BYBYTE_CORE_PUBLIC_H

// Public foundation include for ByByteLib.
//
// Re-exports the internal core umbrella (src/core/ByByteCore.h) under a flat
// basename so sketches can do `#include <ByByteCore.h>` to pull the shared
// platform foundation (Types, PinCapabilities, platform auto-detection and
// the per-board pin defaults in ByByteConfig) without dragging in any module.
//
// Per-module public headers (<MotorDriver.h>, <Buzzer.h>, ...) already pull
// the foundation transitively, so most sketches don't need this directly.

#include "core/ByByteCore.h"

#endif // BYBYTE_CORE_PUBLIC_H