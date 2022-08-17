﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <PackageInfo.h>
#include <PackageDefinitions.h>
#include <winrt/Windows.Foundation.h>
#include "Microsoft.Windows.ApplicationModel.WindowsAppRuntime.DeploymentManager.g.h"

#include <DeploymentActivityContext.h>

namespace winrt::Microsoft::Windows::ApplicationModel::WindowsAppRuntime::implementation
{
    static PCWSTR c_deploymentAgentFilename{ L"DeploymentAgent.exe" };

    struct DeploymentManager
    {
        DeploymentManager() = default;
        static WindowsAppRuntime::DeploymentResult GetStatus();
        static WindowsAppRuntime::DeploymentResult Initialize();
        static WindowsAppRuntime::DeploymentResult Initialize(WindowsAppRuntime::DeploymentInitializeOptions const& deploymentInitializeOptions);

    private:
        static WindowsAppRuntime::DeploymentResult GetStatus(hstring const& packageFullName);
        static WindowsAppRuntime::DeploymentResult Initialize(hstring const& packageFullName);
        static WindowsAppRuntime::DeploymentResult Initialize(hstring const& packageFullName, WindowsAppRuntime::DeploymentInitializeOptions const& deploymentInitializeOptions);

    private:
        static WindowsAppRuntime::DeploymentResult _Initialize(
            ::WindowsAppRuntime::Deployment::Activity::Context& initializeActivityContext,
            hstring const& packageFullName,
            WindowsAppRuntime::DeploymentInitializeOptions const& deploymentInitializeOptions);

    private:
        static MddCore::PackageInfo GetPackageInfoForPackage(std::wstring const& packageFullName);
        static std::vector<std::wstring> FindPackagesByFamily(std::wstring const& packageFamilyName);
        static HRESULT VerifyPackage(const std::wstring& packageFamilyName, const PACKAGE_VERSION targetVersion);
        static std::wstring GetPackagePath(std::wstring const& packageFullName);
        static HRESULT AddPackageInBreakAwayProcess(const std::filesystem::path& packagePath, const bool forceDeployment);
        static std::wstring GenerateDeploymentAgentPath();
        static HRESULT AddPackage(const std::filesystem::path& packagePath, const bool forceDeployment);
        static HRESULT DeployPackages(const std::wstring& frameworkPackageFullName, const bool forceDeployment);
        static HRESULT Deploy(const std::wstring& frameworkPackageFullName, const bool forceDeployment = false);
        static HRESULT InstallLicenses(const std::wstring& frameworkPackageFullName);
        static hstring GetCurrentFrameworkPackageFullName();

    };
}
namespace winrt::Microsoft::Windows::ApplicationModel::WindowsAppRuntime::factory_implementation
{
    struct DeploymentManager : DeploymentManagerT<DeploymentManager, implementation::DeploymentManager>
    {
    };
}
