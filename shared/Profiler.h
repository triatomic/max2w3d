#pragma once

#ifndef __TT_SHARED_PROFILER_H_INCLUDED__
#define __TT_SHARED_PROFILER_H_INCLUDED__

#ifdef TT_PROFILE
// This is just a wrapper around Tracy. (https://github.com/wolfpld/tracy)
// Done in this way to allow us to easily replace it with different instrumentation if we need to

#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"
#include "tracy/TracyD3D11.hpp"

// Dance required to get the C preprocessor to actually expand concatenated macro values... like __LINE__
#define TT_PROFILER_CAT_(a, b) a ## b
#define TT_PROFILER_CAT(a, b) TT_PROFILER_CAT_(a, b)

// Delimits "frame" iterations. This can be any conceptually repeating task
#define TT_PROFILER_FRAME FrameMark

// Delimits "frame" iterations. This can be any conceptually repeating task
// WARNING: If using the named variant the same string must be given to each call.
// Do not rely on the compiler pooling literals, make a variable for it.
#define TT_PROFILER_FRAME_N(name) FrameMarkNamed(name)

// Delimits "frame" iterations. This can be any conceptually repeating task
// WARNING: If using the named variant the same string must be given to each call.
// Do not rely on the compiler pooling literals, make a variable for it.
#define TT_PROFILER_FRAME_BEGIN(name) FrameMarkStart(name)

// Delimits "frame" iterations. This can be any conceptually repeating task
// WARNING: If using the named variant the same string must be given to each call.
// Do not rely on the compiler pooling literals, make a variable for it.
#define TT_PROFILER_FRAME_END(name) FrameMarkStart(name)

// Delimits an automatically named scoped zone
#define TT_PROFILER_SCOPE ZoneNamed(TT_PROFILER_CAT(__profiler_scope, __LINE__), true)

// Delimits an automatically named scope with a color.
// Color should be in the format 0xBBGGRR
#define TT_PROFILER_SCOPE_C(color) ZoneNamedC(TT_PROFILER_CAT(__profiler_scope, __LINE__), color, true)

// Delimits a scoped zone with a custom display name
#define TT_PROFILER_SCOPE_N(name) ZoneNamedN(TT_PROFILER_CAT(__profiler_scope, __LINE__), name, true)

// Delimits a scoped zone with a color
// Color should be in the format 0xBBGGRR
#define TT_PROFILER_SCOPE_NC(name, color) ZoneNamedNC(TT_PROFILER_CAT(__profiler_scope, __LINE__), #name, color, true)

// Delimits a named profiler scope
#define TT_PROFILER_SCOPE_NAMED(varname) ZoneNamed(varname, true)

// Delimits a named profiler scope with a custom variable name and a custom display name
#define TT_PROFILER_SCOPE_NAMED_N(varname, display_name) ZoneNamedN(varname, display_name, true)

// Delimits a named profiler scope with a color
// Color should be in the format 0xBBGGRR
#define TT_PROFILER_SCOPE_NAMED_C(varname, color) ZoneNamedC(varname, color, true)

// Delimits a profiler scope with a custom display name and a custom color
// Color should be in the format 0xBBGGRR
#define TT_PROFILER_SCOPE_NAMED_NC(varname, display_name, color) ZoneNamedNC(varname, display_name, color, true)

// Delimits the start of a zone
#define TT_PROFILER_SCOPE_START(varname, display_name) TracyCZoneN(varname, display_name, true)

// Delimits the end of a zone. Must be in a scope enclosed by the starting counterpart
#define TT_PROFILER_SCOPE_STOP(varname) TracyCZoneEnd(varname)

// Assign associated text to a named active scope
#define TT_PROFILER_SCOPE_TEXT(varname, text, text_length) ZoneTextV(varname, text, text_length)

// Assign associated text to a named active scope. strlen will be called for you
#define TT_PROFILER_SCOPE_TEXT_AUTO(varname, text) ZoneTextV(varname, text, strlen(text))

// Assign associated text to a named active scope, using a string literal with a compile time size
#define TT_PROFILER_SCOPE_TEXT_L(varname, text, text_length)  { static constexpr size_t _profile_scope_text_len##varname = strlen(text); ZoneTextV(name, text, _profile_scope_text_len##name) }

// Assign a numeric value to a named scope
#define TT_PROFILER_SCOPE_VALUE(varname, value) ZoneValueV(varname, value)

// Plot a data point to a configured plot
// Warning: the name string provided to each plot call must have an identical address. Use a variable, do not rely on the compiler folding literals!
#define TT_PROFILER_PLOT(name, value) TracyPlot(name, int64_t(value))

// Configure a numeric plot
// Color should be in the format 0xBBGGRR
// Warning: the name string provided to each plot call must have an identical address. Use a variable, do not rely on the compiler folding literals!
#define TT_PROFILER_CONFIGURE_NUMERIC_PLOT(name, isStepped, isFilled, color) TracyPlotConfig(name, ::tracy::PlotFormatType::Number, isStepped, isFilled, color)

// Configure a percentile plot
// Color should be in the format 0xBBGGRR
// Warning: the name string provided to each plot call must have an identical address. Use a variable, do not rely on the compiler folding literals!
#define TT_PROFILER_CONFIGURE_PERCENT_PLOT(name, isStepped, isFilled, color) TracyPlotConfig(name, ::tracy::PlotFormatType::Percentage, isStepped, isFilled, color)

// Configure a memory plot
// Color should be in the format 0xBBGGRR
// Warning: the name string provided to each plot call must have an identical address. Use a variable, do not rely on the compiler folding literals!
#define TT_PROFILER_CONFIGURE_MEMORY_PLOT(name, isStepped, isFilled, color) TracyPlotConfig(name, ::tracy::PlotFormatType::Memory, isStepped, isFilled, color)

// Note: Type erasing tracy::D3D11Ctx to void* to try and stop tracy types from leaking into the codebase.
// Unfortunately the context needs to be passed around, which means it needs to be held somewhere.

// Create a new profiling context for a device and device_context pair.
// Note: This value later needs to be cleaned up with TT_PROFILER_DESTROY_D3D111_CONTEXT
#define TT_PROFILER_CREATE_D3D11_CONTEXT(context, device, device_context)  { auto typedContext = TracyD3D11Context(device, device_context); context = reinterpret_cast<void*>(typedContext); }

// Destroy a previously created D3D11 profiling context
#define TT_PROFILER_DESTROY_D3D11_CONTEXT(context) TracyD3D11Destroy(reinterpret_cast<::tracy::D3D11Ctx*>(context))

// Give a display name to a created D3D11 profiling context
#define TT_PROFILER_CREATE_D3D11_CONTEXT_NAME(context, name, nameLength) TracyD3D11ContextName(reinterpret_cast<::tracy::D3D11Ctx*>(context), name, nameLength)

// Define a D3D11 profiling scope. Commands issued while this zone is active will be attributed to it
// Note: Name must be a static const string
#define TT_PROFILER_D3D11_SCOPE(context, name) TracyD3D11NamedZone(reinterpret_cast<::tracy::D3D11Ctx*>(context), TT_PROFILER_CAT(__profile_scope_, __LINE__), name, true)

// Define a D3D11 profiling scope, with a specific color. Commands issued while this zone is active will be attributed to it
// Note: Name must be a static const string
// Color should be in 0xBBGGRR format
#define TT_PROFILER_D3D11_SCOPE_C(context, name, color) TracyD3D11NamedZoneC(reinterpret_cast<::tracy::D3D11Ctx*>(context), TT_PROFILER_CAT(__profile_scope_, __LINE__), name, color, true)

// Call to collect profiling events from the GPU. Must be called periodically
#define TT_PROFILER_D3D11_COLLECT_EVENTS(context) TracyD3D11Collect(reinterpret_cast<::tracy::D3D11Ctx*>(context))

// Call to the windows API to set the thread name.
#define TT_PROFILER_SET_THREAD_NAME(name) tracy::SetThreadName(name)
#define TT_PROFILER_SET_THREAD_NAME_GROUP_HINT(name, group_hint) tracy::SetThreadNameWithHint(name, group_hint)

#else

#define TT_PROFILER_FRAME
#define TT_PROFILER_FRAME_N(name)
#define TT_PROFILER_FRAME_BEGIN(name)
#define TT_PROFILER_FRAME_END(name)
#define TT_PROFILER_SCOPE
#define TT_PROFILER_SCOPE_C(color)
#define TT_PROFILER_SCOPE_N(name)
#define TT_PROFILER_SCOPE_NC(name, color)
#define TT_PROFILER_SCOPE_NAMED(varname)
#define TT_PROFILER_SCOPE_NAMED_N(varname, display_name)
#define TT_PROFILER_SCOPE_NAMED_C(varname, color)
#define TT_PROFILER_SCOPE_NAMED_NC(varname, display_name, color)
#define TT_PROFILER_SCOPE_START(varname, display_name)
#define TT_PROFILER_SCOPE_STOP(varname)
#define TT_PROFILER_SCOPE_TEXT(varname, text, text_length)
#define TT_PROFILER_SCOPE_TEXT_AUTO(varname, text)
#define TT_PROFILER_SCOPE_TEXT_L(varname, text, text_length)
#define TT_PROFILER_SCOPE_VALUE(varname, value)
#define TT_PROFILER_PLOT(name, value)
#define TT_PROFILER_CONFIGURE_NUMERIC_PLOT(name, isStepped, isFilled, color)
#define TT_PROFILER_CONFIGURE_PERCENT_PLOT(name, isStepped, isFilled, color)
#define TT_PROFILER_CONFIGURE_MEMORY_PLOT(name, isStepped, isFilled, color)

#define TT_PROFILER_CREATE_D3D11_CONTEXT(context, device, device_context)
#define TT_PROFILER_DESTROY_D3D11_CONTEXT(context)
#define TT_PROFILER_CREATE_D3D11_CONTEXT_NAME(context, name, nameLength)
#define TT_PROFILER_D3D11_SCOPE(context, name)
#define TT_PROFILER_D3D11_SCOPE_C(context, name, color)
#define TT_PROFILER_D3D11_COLLECT_EVENTS(context)

#define TT_PROFILER_SET_THREAD_NAME(name)
#define TT_PROFILER_SET_THREAD_NAME_GROUP_HINT(name, group_hint)

#endif

#endif //__TT_SHARED_PROFILER_H_INCLUDED__
