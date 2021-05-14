﻿#include "pch.h"
#include <testdef.h>
#include <iostream>
#include <sstream>
#include <wil/win32_helpers.h>
#include <winrt/Windows.ApplicationModel.Background.h> // we need this for BackgroundTask APIs
using namespace winrt::Microsoft::Windows::AppLifecycle;
using namespace winrt::Microsoft::Windows::PushNotifications;
using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Background; // BackgroundTask APIs

enum UnitTest {
    channelRequestUsingNullRemoteId, channelRequestUsingRemoteId, multipleChannelRequestUsingSameRemoteId,
    multipleChannelRequestUsingMultipleRemoteId, threeChannelRequestUsingSameRemoteId, activatorTest,
};

static std::map<std::string, UnitTest> switchMapping;
winrt::guid remoteId1(L"a2e4a323-b518-4799-9e80-0b37aeb0d225");
winrt::guid remoteId2(L"CA1A4AB2-AC1D-4EFC-A132-E5A191CA285A");
winrt::guid remoteId3(L"40FCE789-C6BF-4F47-A6CF-6B9C1DCE31BA");

void signalPhase(const std::wstring& phaseEventName)
{
    wil::unique_event phaseEvent;
    if (phaseEvent.try_open(phaseEventName.c_str(), EVENT_MODIFY_STATE, false))
    {
        phaseEvent.SetEvent();
    }
}

void initUnitTestMapping()
{
    switchMapping["ChannelRequestUsingNullRemoteId"] = UnitTest::channelRequestUsingNullRemoteId;
    switchMapping["ChannelRequestUsingRemoteId"] = UnitTest::channelRequestUsingRemoteId;
    switchMapping["MultipleChannelRequestUsingSameRemoteId"] = UnitTest::multipleChannelRequestUsingSameRemoteId;
    switchMapping["MultipleChannelRequestUsingMultipleRemoteId"] = UnitTest::multipleChannelRequestUsingMultipleRemoteId;
    switchMapping["ThreeChannelRequestUsingSameRemoteId"] = UnitTest::threeChannelRequestUsingSameRemoteId;
    switchMapping["ActivatorTest"] = UnitTest::activatorTest;
}

bool ChannelRequestUsingNullRemoteId()
{
    winrt::hresult hr = S_OK;

    try
    {
        auto channelOperation = PushNotificationManager::CreateChannelAsync(GUID_NULL).get();
    }
    catch (...)
    {
        auto channelRequestException = hresult_error(to_hresult());
        hr = channelRequestException.code();
    }

    return (hr == E_INVALIDARG);
}

bool ChannelRequestUsingRemoteId()
{
    wil::unique_handle channelEvent = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    auto channelOperationResult = S_OK;
    auto channelOperation = PushNotificationManager::CreateChannelAsync(remoteId1);

    // Setup the completed event handler
    channelOperation.Completed(
        [&channelEvent, &channelOperationResult](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close(); // Avoids using cached channel everytime the test runs
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
                channelOperationResult = result.ExtendedError();
            }

            SetEvent(channelEvent.get());
        });

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent.get(), 960000 /* milliseconds */))
    {
        channelOperation.Cancel();
        channelOperationResult = ERROR_TIMEOUT;
    }
    else
    {
        channelOperation.Close(); // Do not call getresults after this
    }

    return (channelOperationResult == S_OK);
}

bool MultipleChannelRequestUsingSameRemoteId()
{
    wil::unique_handle channelEvent1 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    wil::unique_handle channelEvent2 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));

    auto channelOperationResult2 = S_OK;

    auto channelOperation1 = PushNotificationManager::CreateChannelAsync(remoteId1);

    channelOperation1.Completed(
        [&channelEvent1](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close(); // Avoids using cached channel everytime the test runs
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
            }

            SetEvent(channelEvent1.get());
        });

    auto channelOperation2 = PushNotificationManager::CreateChannelAsync(remoteId1);

    channelOperation2.Completed(
        [&channelEvent2, &channelOperationResult2](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close(); // Avoids using cached channel everytime the test runs
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
                channelOperationResult2 = result.ExtendedError();
            }

            SetEvent(channelEvent2.get());
        });

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent2.get(), 960000 /* milliseconds */))
    {
        channelOperation2.Cancel();
        channelOperationResult2 = ERROR_TIMEOUT;
    }

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent1.get(), 960000 /* milliseconds */))
    {
        channelOperation1.Cancel();
    }

    return (channelOperationResult2 == WPN_E_OUTSTANDING_CHANNEL_REQUEST);
}

bool MultipleChannelRequestUsingMultipleRemoteId()
{
    wil::unique_handle channelEvent1 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    wil::unique_handle channelEvent2 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));

    auto channelOperationResult2 = S_OK;

    auto channelOperation1 = PushNotificationManager::CreateChannelAsync(remoteId1);

    channelOperation1.Completed(
        [&channelEvent1](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close();
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
            }

            SetEvent(channelEvent1.get());
        });

    auto channelOperation2 = PushNotificationManager::CreateChannelAsync(remoteId2);

    channelOperation2.Completed(
        [&channelEvent2, &channelOperationResult2](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close();
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
                channelOperationResult2 = result.ExtendedError();
            }

            SetEvent(channelEvent2.get());
        });

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent2.get(), 960000 /* milliseconds */))
    {
        channelOperation2.Cancel();
        channelOperationResult2 = ERROR_TIMEOUT;
    }

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent1.get(), 960000 /* milliseconds */))
    {
        channelOperation1.Cancel();
    }

    return (channelOperationResult2 == WPN_E_OUTSTANDING_CHANNEL_REQUEST);
}

bool ThreeChannelRequestUsingSameRemoteId()
{
    wil::unique_handle channelEvent1 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));

    wil::unique_handle channelEvent2 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    auto channelOperationResult2 = S_OK;

    wil::unique_handle channelEvent3 = wil::unique_handle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    auto channelOperationResult3 = S_OK;

    auto channelOperation1 = PushNotificationManager::CreateChannelAsync(remoteId1);

    channelOperation1.Completed(
        [&channelEvent1](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close();
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
            }

            SetEvent(channelEvent1.get());
        });

    auto channelOperation2 = PushNotificationManager::CreateChannelAsync(remoteId2);

    channelOperation2.Completed(
        [&channelEvent2, &channelOperationResult2](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close();
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
                channelOperationResult2 = result.ExtendedError();
            }

            SetEvent(channelEvent2.get());
        });


    auto channelOperation3 = PushNotificationManager::CreateChannelAsync(remoteId3);

    channelOperation3.Completed(
        [&channelEvent3, &channelOperationResult3](
            IAsyncOperationWithProgress<PushNotificationCreateChannelResult, PushNotificationCreateChannelStatus> const& sender,
            AsyncStatus const /* asyncStatus */)
        {
            auto result = sender.GetResults();
            if (result.Status() == PushNotificationChannelStatus::CompletedSuccess)
            {
                result.Channel().Close();
            }
            else if (result.Status() == PushNotificationChannelStatus::CompletedFailure)
            {
                channelOperationResult3 = result.ExtendedError();
            }

            SetEvent(channelEvent3.get());
        });

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent2.get(), 960000 /* milliseconds */))
    {
        channelOperation2.Cancel();
        channelOperationResult2 = ERROR_TIMEOUT;
    }

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent3.get(), 960000 /* milliseconds */))
    {
        channelOperation3.Cancel();
        channelOperationResult3 = ERROR_TIMEOUT;
    }

    // The maximum amount of time it takes for channel request to be obtained - 16mins
    if (WAIT_OBJECT_0 != WaitForSingleObject(channelEvent1.get(), 960000 /* milliseconds */))
    {
        channelOperation1.Cancel();
    }

    return ((channelOperationResult2 == WPN_E_OUTSTANDING_CHANNEL_REQUEST) && (channelOperationResult3 == WPN_E_OUTSTANDING_CHANNEL_REQUEST));
}

bool ActivatorTest()
{
    try
    {
        PushNotificationActivationInfo info(
            PushNotificationRegistrationKind::PushTrigger | PushNotificationRegistrationKind::ComActivator,
            c_fakeComServerId);

        auto token = PushNotificationManager::RegisterActivator(info);
        if (!token.TaskRegistration())
        {
            return false;
        }

        PushNotificationManager::UnregisterActivator(token, PushNotificationRegistrationKind::PushTrigger | PushNotificationRegistrationKind::ComActivator);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool runUnitTest(std::string unitTest)
{
    switch (switchMapping[unitTest])
    {
    case UnitTest::channelRequestUsingNullRemoteId:
        return ChannelRequestUsingNullRemoteId();

    case UnitTest::channelRequestUsingRemoteId:
        return ChannelRequestUsingRemoteId();

    case UnitTest::multipleChannelRequestUsingSameRemoteId:
        return MultipleChannelRequestUsingSameRemoteId();

    case UnitTest::multipleChannelRequestUsingMultipleRemoteId:
        return MultipleChannelRequestUsingMultipleRemoteId();

    case UnitTest::threeChannelRequestUsingSameRemoteId:
        return ThreeChannelRequestUsingSameRemoteId();

    case UnitTest::activatorTest:
        return ActivatorTest();

    default:
        return false;
    }
}

int main()
{
    initUnitTestMapping();

    PushNotificationActivationInfo info(
        PushNotificationRegistrationKind::PushTrigger | PushNotificationRegistrationKind::ComActivator,
        winrt::guid("ccd2ae3f-764f-4ae3-be45-9804761b28b2")); // same clsid as app manifest

    auto token = PushNotificationManager::RegisterActivator(info);
    
    auto args = AppInstance::GetCurrent().GetActivatedEventArgs();
    auto kind = args.Kind();

    if (kind == ExtendedActivationKind::Protocol)
    {
        auto protocolArgs = args.Data().as<IProtocolActivatedEventArgs>();
        Uri actualUri = protocolArgs.Uri();
        std::string unitTest = winrt::to_string(actualUri.Host());

        std::cout << unitTest << std::endl;

        PushNotificationManager::UnregisterActivator(token, PushNotificationRegistrationKind::PushTrigger | PushNotificationRegistrationKind::ComActivator);

        // Switch on this variable to run specific components (uri://ComponentToTest)
        auto output = runUnitTest(unitTest);

        if (output)
        {
            // Signal TAEF that protocol was activated and valid.
            signalPhase(c_testProtocolScheme_Packaged);
        }
        else
        {
            // Signal TAEF that the test failed
            signalPhase(c_testFailureEventName);
        }
    }
    else if (kind == ExtendedActivationKind::Push)
    {
        PushNotificationReceivedEventArgs pushArgs = args.Data().as<PushNotificationReceivedEventArgs>();
        auto payload = pushArgs.Payload();
        std::wstring payloadString(payload.begin(), payload.end());
        PushNotificationManager::UnregisterActivator(token, PushNotificationRegistrationKind::PushTrigger | PushNotificationRegistrationKind::ComActivator);
        if (!payloadString.compare(c_rawNotificationPayload))
        {
            signalPhase(c_testProtocolScheme_Packaged);
        }
        else
        {
            // Signal TAEF that the test failed
            signalPhase(c_testFailureEventName);
        }
    }

    return 0;
};
