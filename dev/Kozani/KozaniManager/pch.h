﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include <unknwn.h>
#include <appmodel.h>

#include <thread>
#include <mutex>

// Temporarily disable C4324 because WRL generates a false (well, irrelevant) warning
//   'Microsoft::WRL::Details::StaticStorage<Microsoft::WRL::Details::OutOfProcModuleBase<ModuleT>::GenericReleaseNotifier<T>,Microsoft::WRL::Details::StorageInstance::OutOfProcCallbackBuffer1,ModuleT>': structure was padded due to alignment specifier
#pragma warning(push)
#pragma warning(disable:4324)
#include <wrl.h>
#pragma warning(pop)

#include <wil/cppwinrt.h>
#include <wil/token_helpers.h>

// Needed to get std::wstring specialization for wil::str_printf<std::wstring>
#include <wil/stl.h>

#include <wil/resource.h>
#include <wil/result_macros.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <winrt/Windows.ApplicationModel.Activation.h>

#include <wil/com.h>

#include <WindowsAppRuntimeInsights.h>

#include <Microsoft.Utf8.h>

#include "KozaniProtobufMessages.h"
#include "KozaniDvcProtocol.h"
#include "Logging.h"
