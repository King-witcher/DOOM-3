/*
===========================================================================

Minimal stand-in for the legacy DirectX SDK <DxErr.h>.

DxErr.h / DxErr.lib shipped with the old DirectX SDK (June 2010) and were
removed from the modern Windows SDK. Doom 3 only ever uses the DXTRACE_ERR
macro to surface a DirectSound HRESULT from win_snd.cpp, so we keep it as a
passthrough that returns the same hr (callers do `return DXTRACE_ERR(...)`
and `DXTRACE_ERR(...)` as a statement). The descriptive string is discarded.

===========================================================================
*/

#ifndef __DOOM3_DXERR_SHIM_H__
#define __DOOM3_DXERR_SHIM_H__

// Evaluate to the HRESULT, ignore the message. The string argument is always
// a literal here, so dropping it has no side effects.
#define DXTRACE_ERR( str, hr )		( hr )

#endif // __DOOM3_DXERR_SHIM_H__
