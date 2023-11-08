// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "Microsoft.Windows.Management.Deployment.PackageDeploymentResult.g.h"

namespace winrt::Microsoft::Windows::Management::Deployment::implementation
{
    struct PackageDeploymentResult : PackageDeploymentResultT<PackageDeploymentResult>
    {
        PackageDeploymentResult() = default;
        PackageDeploymentResult(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus status, winrt::hresult const& extendedError, bool isRegistered, winrt::guid const& activityId);

        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus Status();
        winrt::hresult ExtendedError();
        winrt::guid ActivityId();
        bool IsRegistered();

    private:
        PackageDeploymentStatus m_status{};
        winrt::hresult m_extendedError;
        bool m_isRegistered{};
        winrt::guid m_activityId{};
    };
}
