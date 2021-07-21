﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef __WINDOWSAPPSDKTELEMETRY_INCLUDED
#define __WINDOWSAPPSDKTELEMETRY_INCLUDED

#ifdef __WIL_TRACELOGGING_H_INCLUDED
#error "WIL Tracelogging.h must not be explicitly included when including this file"
#endif

char* WINDOWSAPPSDK_PACKAGE_VER();
char* WINDOWSAPPSDK_EXPERIMENTATION_LEVEL();

#define _GENERIC_PARTB_FIELDS_ENABLED \
        TraceLoggingStruct(4, "PartB_WINDOWSAPPSDK_API"), \
        TraceLoggingString(WINDOWSAPPSDK_PACKAGE_VER(), "Windows App SDK package version"), \
        TraceLoggingString(WINDOWSAPPSDK_EXPERIMENTATION_LEVEL(), "Windows App SDK experimentation level"), \
        TraceLoggingBool(wil::details::IsDebuggerPresent(), "IsDebugging"), \
        TraceLoggingBool(true, "UTCReplace_AppSessionGuid")

#include <wil/tracelogging.h>
#endif // __WINDOWSAPPSDKTELEMETRY_INCLUDED
