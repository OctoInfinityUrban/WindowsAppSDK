// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "Microsoft.Windows.ApplicationModel.WindowsAppRuntime.DeploymentInitializeOptions.g.h"

namespace winrt::Microsoft::Windows::ApplicationModel::WindowsAppRuntime::implementation
{
    struct DeploymentInitializeOptions : DeploymentInitializeOptionsT<DeploymentInitializeOptions>
    {
        DeploymentInitializeOptions() = default;

        bool ForceDeployment();
        void ForceDeployment(bool value);
        bool OnError_DebugBreak();
        void OnError_DebugBreak(bool value);
        bool OnError_DebugBreak_IfDebuggerAttached();
        void OnError_DebugBreak_IfDebuggerAttached(bool value);
        bool OnError_FailFast();
        void OnError_FailFast(bool value);
        bool OnError_ShowUI();
        void OnError_ShowUI(bool value);
        bool OnNoPackageIdentity_NOOP();
        void OnNoPackageIdentity_NOOP(bool value);

    private:
        bool m_ForceDeployment{};
        bool m_OnError_DebugBreak{};
        bool m_OnError_DebugBreak_IfDebuggerAttached{};
        bool m_OnError_FailFast{};
        bool m_OnError_ShowUI{};
        bool m_OnNoPackageIdentity_NOOP{};
    };
}
namespace winrt::Microsoft::Windows::ApplicationModel::WindowsAppRuntime::factory_implementation
{
    struct DeploymentInitializeOptions : DeploymentInitializeOptionsT<DeploymentInitializeOptions, implementation::DeploymentInitializeOptions>
    {
    };
}
