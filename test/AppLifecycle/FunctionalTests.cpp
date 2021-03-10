﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <testdef.h>
#include "Shared.h"

namespace Test::FileSystem
{
    constexpr PCWSTR ThisTestDllFilename{ L"CppTest.dll" };
}
#include <ProjectReunion.Test.Bootstrap.h>

#include <ProjectReunion.Test.Diagnostics.h>

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace winrt::Microsoft::ApplicationModel::Activation;
using namespace winrt;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::System;

namespace TP = ::Test::Packages;
namespace TD = ::Test::Diagnostics;

// TODO: Write Register/Unregister tests that utilize the Assoc APIs to validate results.

namespace ProjectReunionCppTest
{
    class AppLifecycleTests
    {
    private:
        wil::unique_event m_failed;

        const std::wstring c_testDataFileName = L"testfile" + c_testFileExtension;
        const std::wstring c_testDataFileName_Packaged = L"testfile" + c_testFileExtension_Packaged;
        const std::wstring c_testPackageFile = g_deploymentDir + L"AppLifecycleTestPackage.msixbundle";
        const std::wstring c_testPackageCertFile = g_deploymentDir + L"AppLifecycleTestPackage.cer";
        const std::wstring c_testPackageFullName = L"AppLifecycleTestPackage_1.0.0.0_x64__ph1m9x8skttmg";

    public:
        BEGIN_TEST_CLASS(AppLifecycleTests)
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
            TEST_CLASS_PROPERTY(L"RunAs:Class", L"RestrictedUser")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassInit)
        {
            TD::DumpExecutionContext();

            ::Test::Bootstrap::SetupPackages();

            // Deploy packaged app to register handler through the manifest.
            //RunCertUtil(c_testPackageCertFile);
            //InstallPackage(c_testPackageFile);

            // Write out some test content.
            WriteContentFile(c_testDataFileName);
            WriteContentFile(c_testDataFileName_Packaged);

            return true;
        }

        TEST_CLASS_CLEANUP(ClassUninit)
        {
            TD::DumpExecutionContext();

            // Swallow errors in cleanup.
            try
            {
                DeleteContentFile(c_testDataFileName_Packaged);
                DeleteContentFile(c_testDataFileName);
                //UninstallPackage(c_testPackageFullName);
                //RunCertUtil(c_testPackageCertFile, true);
            }
            catch (const std::exception&)
            {
            }
            catch (const winrt::hresult_error&)
            {
            }

            ::Test::Bootstrap::CleanupPackages();
            return true;
        }

        TEST_METHOD_SETUP(MethodInit)
        {
            TD::DumpExecutionContext();

            VERIFY_IS_TRUE(TP::IsPackageRegistered_ProjectReunionFramework());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyDataStore());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyLifetimeManager());

            ::Test::Bootstrap::Setup();
            m_failed = CreateTestEvent(c_testFailureEventName);

            return true;
        }

        TEST_METHOD_CLEANUP(MethodUninit)
        {
            TD::DumpExecutionContext();

            VERIFY_IS_TRUE(TP::IsPackageRegistered_ProjectReunionFramework());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyDataStore());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyLifetimeManager());

            ::Test::Bootstrap::Cleanup();

            return true;
        }

        TEST_METHOD(GetActivatedEventArgsIsNotNull)
        {
            VERIFY_IS_NOT_NULL(AppLifecycle::GetActivatedEventArgs());
        }

        TEST_METHOD(GetActivatedEventArgsForLaunch)
        {
            auto args = AppLifecycle::GetActivatedEventArgs();
            VERIFY_IS_NOT_NULL(args);
            VERIFY_ARE_EQUAL(args.Kind(), ActivationKind::Launch);

            auto launchArgs = args.as<LaunchActivatedEventArgs>();
            VERIFY_IS_NOT_NULL(launchArgs);
        }

        TEST_METHOD(GetActivatedEventArgsForFile_Win32)
        {
            // Create a named event for communicating with test app.
            auto event = CreateTestEvent(c_testFilePhaseEventName);

            // Launch the test app to register for protocol launches.
            Execute(L"AppLifecycleTestApp.exe", L"/RegisterFile", g_deploymentDir);

            // Wait for the register event.
            WaitForEvent(event, m_failed);

            // Launch the file and wait for the event to fire.
            auto file = OpenDocFile(c_testDataFileName);
            auto launchResult = Launcher::LaunchFileAsync(file).get();
            VERIFY_IS_TRUE(launchResult);

            // Wait for the protocol activation.
            WaitForEvent(event, m_failed);
        }

        //TEST_METHOD(GetActivatedEventArgsForFile_PackagedWin32)
        //{
        //    // Create a named event for communicating with test app.
        //    auto event = CreateTestEvent(c_testFilePhaseEventName);

        //    // Launch the file and wait for the event to fire.
        //    auto file = OpenDocFile(c_testDataFileName_Packaged);
        //    auto launchResult = Launcher::LaunchFileAsync(file).get();
        //    VERIFY_IS_TRUE(launchResult);

        //    // Wait for the protocol activation.
        //    WaitForEvent(event, m_failed);
        //}

        TEST_METHOD(GetActivatedEventArgsForProtocol_Win32)
        {
            // Create a named event for communicating with test app.
            auto event{ CreateTestEvent(c_testProtocolPhaseEventName) };

            // Cleanup any leftover data from previous runs i.e. ensure we running with a clean slate
            try
            {
                Execute(L"AppLifecycleTestApp.exe", L"/UnregisterProtocol", g_deploymentDir);
                WaitForEvent(event, m_failed);
            }
            catch (...)
            {
                //TODO:Unregister should not fail if ERROR_FILE_NOT_FOUND | ERROR_PATH_NOT_FOUND
            }

            // Register the protocol
            Execute(L"AppLifecycleTestApp.exe", L"/RegisterProtocol", g_deploymentDir);
            WaitForEvent(event, m_failed);

            // Launch a URI with the protocol schema and wait for the app to fire the event
            Uri launchUri{ c_testProtocolScheme + L"://this_is_a_test" };
            auto launchResult{ Launcher::LaunchUriAsync(launchUri).get() };
            VERIFY_IS_TRUE(launchResult);
            WaitForEvent(event, m_failed);

            // Deregister the protocol
            Execute(L"AppLifecycleTestApp.exe", L"/UnregisterProtocol", g_deploymentDir);
            WaitForEvent(event, m_failed);
        }

        //TEST_METHOD(GetActivatedEventArgsForProtocol_PackagedWin32)
        //{
        //    // Create a named event for communicating with test app.
        //    auto event = CreateTestEvent(c_testProtocolPhaseEventName);

        //    RunCertUtil(L"AppLifecycleTestPackage.cer");

        //    // Deploy packaged app to register handler through the manifest.
        //    std::wstring packagePath{ g_deploymentDir + L"\\AppLifecycleTestPackage.msixbundle" };
        //    InstallPackage(packagePath);

        //    // Launch a protocol and wait for the event to fire.
        //    Uri launchUri{ c_testProtocolScheme_Packaged + L"://this_is_a_test" };
        //    auto launchResult = Launcher::LaunchUriAsync(launchUri).get();
        //    VERIFY_IS_TRUE(launchResult);

        //    // Wait for the protocol activation.
        //    WaitForEvent(event, m_failed);
        //}
    };

    //-----------------------------------------------------------------
    class AppLifecycleTests_UAP
    {
    private:
        wil::unique_event m_failed;

        const std::wstring c_testDataFileName = L"testfile" + c_testFileExtension;
        const std::wstring c_testDataFileName_Packaged = L"testfile" + c_testFileExtension_Packaged;
        const std::wstring c_testPackageFile = g_deploymentDir + L"AppLifecycleTestPackage.msixbundle";
        const std::wstring c_testPackageCertFile = g_deploymentDir + L"AppLifecycleTestPackage.cer";
        const std::wstring c_testPackageFullName = L"AppLifecycleTestPackage_1.0.0.0_x64__ph1m9x8skttmg";

    public:
        BEGIN_TEST_CLASS(AppLifecycleTests_UAP)
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
            TEST_CLASS_PROPERTY(L"RunFixtureAs:Class", L"RestrictedUser")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassInit)
        {
            TD::DumpExecutionContext();

            ::Test::Bootstrap::SetupPackages();

            // Write out some test content.
            WriteContentFile(c_testDataFileName);
            WriteContentFile(c_testDataFileName_Packaged);

            return true;
        }

        TEST_CLASS_CLEANUP(ClassUninit)
        {
            TD::DumpExecutionContext();

            // Swallow errors in cleanup.
            try
            {
                DeleteContentFile(c_testDataFileName_Packaged);
                DeleteContentFile(c_testDataFileName);
                //UninstallPackage(c_testPackageFullName);
            }
            catch (const std::exception&)
            {
            }
            catch (const winrt::hresult_error&)
            {
            }

            ::Test::Bootstrap::CleanupPackages();
            return true;
        }

        TEST_METHOD_SETUP(MethodInit)
        {
            TD::DumpExecutionContext();

            VERIFY_IS_TRUE(TP::IsPackageRegistered_ProjectReunionFramework());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyDataStore());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyLifetimeManager());

            m_failed = CreateTestEvent(c_testFailureEventName);

            return true;
        }

        TEST_METHOD_CLEANUP(MethodUninit)
        {
            TD::DumpExecutionContext();

            VERIFY_IS_TRUE(TP::IsPackageRegistered_ProjectReunionFramework());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyDataStore());
            VERIFY_IS_TRUE(TP::IsPackageRegistered_DynamicDependencyLifetimeManager());

            return true;
        }

        TEST_METHOD(GetActivatedEventArgsIsNull_UAP)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_METHOD_PROPERTY(L"RunAs", L"UAP")
                TEST_METHOD_PROPERTY(L"UAP:AppxManifest", L"AppLifecycle-AppxManifest.xml")
            END_TEST_METHOD_PROPERTIES();

            TD::DumpExecutionContext();

            VERIFY_IS_NULL(AppLifecycle::GetActivatedEventArgs());
        }
    };
}
