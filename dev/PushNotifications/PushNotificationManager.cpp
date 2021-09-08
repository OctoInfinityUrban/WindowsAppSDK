﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "PushNotificationManager.h"
#include "Microsoft.Windows.PushNotifications.PushNotificationManager.g.cpp"
#include "PushNotificationTelemetry.h"
#include "PushNotificationCreateChannelResult.h"
#include "PushNotifications-Constants.h"
#include <winrt/Windows.ApplicationModel.background.h>
#include <winrt/Windows.Networking.PushNotifications.h>
#include "PushNotificationBackgroundTask.h"
#include <winrt/Windows.Foundation.Metadata.h>
#include <winerror.h>
#include <algorithm>
#include "PushNotificationChannel.h"
#include "externs.h"
#include <string_view>
#include <frameworkudk/pushnotifications.h>
#include "NotificationsLongRunningProcess_h.h"
#include "PushNotificationTelemetry.h"
#include <iostream>

using namespace std::literals;

constexpr std::wstring_view backgroundTaskName = L"PushBackgroundTaskName"sv;

static wil::unique_event g_waitHandleForArgs;

wil::unique_event& GetWaitHandleForArgs()
{
    return g_waitHandleForArgs;
}

namespace winrt
{
    using namespace Windows::ApplicationModel::Background;
    using namespace Windows::Networking::PushNotifications;
    using namespace Windows::Foundation;
}

namespace winrt::Microsoft::Windows::PushNotifications::implementation
{
    static winrt::Windows::ApplicationModel::Background::IBackgroundTaskRegistration s_pushTriggerRegistration{ nullptr };
    static wil::unique_com_class_object_cookie s_comActivatorRegistration;
    static wil::srwlock s_activatorInfoLock;

    inline constexpr auto c_maxBackoff{ 5min };
    inline constexpr auto c_initialBackoff{ 60s };
    inline constexpr auto c_backoffIncrement{ 60s };

    const HRESULT WNP_E_NOT_CONNECTED = static_cast<HRESULT>(0x880403E8L);
    const HRESULT WNP_E_RECONNECTING = static_cast<HRESULT>(0x880403E9L);
    const HRESULT WNP_E_BIND_USER_BUSY = static_cast<HRESULT>(0x880403FEL);

    bool IsChannelRequestRetryable(const hresult& hr)
    {
        switch (hr)
        {
        case HRESULT_FROM_WIN32(ERROR_TIMEOUT):
        case WNP_E_NOT_CONNECTED:
        case WPN_E_OUTSTANDING_CHANNEL_REQUEST:
        case WNP_E_RECONNECTING:
        case WNP_E_BIND_USER_BUSY:
        case HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE):
            return true;
        default:
            return false;
        }
    }

    void RegisterUnpackagedApplicationHelper(
        const winrt::guid& remoteId,
        _Out_ wil::unique_cotaskmem_string &unpackagedAppUserModelId)
    {
        auto coInitialize = wil::CoInitializeEx();

        auto notificationPlatform{ wil::CoCreateInstance<NotificationsLongRunningPlatform, INotificationsLongRunningPlatform>(CLSCTX_LOCAL_SERVER) };

        wil::unique_cotaskmem_string processName;
        THROW_IF_FAILED(GetCurrentProcessPath(processName));

        THROW_IF_FAILED(notificationPlatform->RegisterFullTrustApplication(processName.get(), remoteId, &unpackagedAppUserModelId));
    }

    winrt::hresult CreateChannelWithRemoteIdHelper(const winrt::guid& remoteId, ChannelDetails& channelInfo) noexcept try
    {
        wchar_t appUserModelId[APPLICATION_USER_MODEL_ID_MAX_LENGTH] = {};
        UINT32 appUserModelIdSize{ ARRAYSIZE(appUserModelId) };

        THROW_IF_FAILED(GetCurrentApplicationUserModelId(&appUserModelIdSize, appUserModelId));

        THROW_HR_IF(E_INVALIDARG, (appUserModelIdSize > APPLICATION_USER_MODEL_ID_MAX_LENGTH) || (appUserModelIdSize == 0));

        HRESULT operationalCode{};
        ABI::Windows::Foundation::DateTime channelExpiryTime{};
        wil::unique_cotaskmem_string channelId;
        wil::unique_cotaskmem_string channelUri;

        THROW_IF_FAILED(PushNotifications_CreateChannelWithRemoteIdentifier(
            appUserModelId,
            remoteId,
            &operationalCode,
            &channelId,
            &channelUri,
            &channelExpiryTime));

        THROW_IF_FAILED(operationalCode);

        winrt::copy_from_abi(channelInfo.channelExpiryTime, &channelExpiryTime);
        channelInfo.appUserModelId = winrt::hstring{ appUserModelId };
        channelInfo.channelId = winrt::hstring{ channelId.get() };
        channelInfo.channelUri = winrt::hstring{ channelUri.get() };

        return S_OK;
    }
    CATCH_RETURN()

    winrt::IAsyncOperationWithProgress<winrt::Microsoft::Windows::PushNotifications::PushNotificationCreateChannelResult, winrt::Microsoft::Windows::PushNotifications::PushNotificationCreateChannelStatus> PushNotificationManager::CreateChannelAsync(const winrt::guid &remoteId)
    {
        std::cout << "ELx - CreateChannelAsync -1" << std::endl;

        try
        {
            std::cout << "ELx - CreateChannelAsync -2" << std::endl;
            THROW_HR_IF(E_INVALIDARG, (remoteId == winrt::guid()));

            std::cout << "ELx - CreateChannelAsync -3" << std::endl;
            auto cancellation{ co_await winrt::get_cancellation_token() };

            std::cout << "ELx - CreateChannelAsync -4" << std::endl;
            cancellation.enable_propagation(true);

            std::cout << "ELx - CreateChannelAsync -5" << std::endl;
            // Allow to register the progress and complete handler
            co_await resume_background();

            std::cout << "ELx - CreateChannelAsync -6" << std::endl;
            auto progress{ co_await winrt::get_progress_token() };

            std::cout << "ELx - CreateChannelAsync -7" << std::endl;
            uint8_t retryCount = 0;
            winrt::hresult channelRequestResult = E_PENDING;
            PushNotificationChannelStatus status = PushNotificationChannelStatus::InProgress;

            std::cout << "ELx - CreateChannelAsync -8" << std::endl;
            PushNotificationCreateChannelStatus
                channelStatus = { status, channelRequestResult, retryCount };

            std::cout << "ELx - CreateChannelAsync -9" << std::endl;
            progress(channelStatus);

            std::cout << "ELx - CreateChannelAsync -10" << std::endl;
            for (auto backOffTime = c_initialBackoff; ; backOffTime += c_backoffIncrement)
            {
                std::cout << "ELx - CreateChannelAsync -11" << std::endl;
                try
                {
                    std::cout << "ELx - CreateChannelAsync -12" << std::endl;
                    if (IsActivatorSupported(PushNotificationRegistrationActivators::PushTrigger))
                    {
                        std::cout << "ELx - CreateChannelAsync -13" << std::endl;
                        ChannelDetails channelInfo{};
                        winrt::hresult hr = CreateChannelWithRemoteIdHelper(remoteId, channelInfo);

                        // RemoteId APIs are not applicable for downlevel OS versions.
                        // So we get error E_NOTIMPL and we fallback to calling into
                        // public WinRT API CreatePushNotificationChannelForApplicationAsync
                        // to request a channel.
                        std::cout << "ELx - CreateChannelAsync -14" << std::endl;
                        if (SUCCEEDED(hr))
                        {
                            std::cout << "ELx - CreateChannelAsync -15" << std::endl;
                            PushNotificationTelemetry::ChannelRequestedByApi(S_OK, remoteId);

                            std::cout << "ELx - CreateChannelAsync -16" << std::endl;
                            co_return winrt::make<PushNotificationCreateChannelResult>(
                                winrt::make<PushNotificationChannel>(channelInfo),
                                S_OK,
                                PushNotificationChannelStatus::CompletedSuccess);
                        }
                        else if (hr == E_NOTIMPL)
                        {
                            std::cout << "ELx - CreateChannelAsync -17" << std::endl;
                            PushNotificationChannelManager channelManager{};
                            winrt::PushNotificationChannel pushChannelReceived{ nullptr };

                            std::cout << "ELx - CreateChannelAsync -18" << std::endl;
                            pushChannelReceived = co_await channelManager.CreatePushNotificationChannelForApplicationAsync();

                            std::cout << "ELx - CreateChannelAsync -19" << std::endl;
                            PushNotificationTelemetry::ChannelRequestedByApi(S_OK, remoteId);

                            std::cout << "ELx - CreateChannelAsync -20" << std::endl;
                            co_return winrt::make<PushNotificationCreateChannelResult>(
                                winrt::make<PushNotificationChannel>(pushChannelReceived),
                                S_OK,
                                PushNotificationChannelStatus::CompletedSuccess);
                        }
                        else
                        {
                            std::cout << "ELx - CreateChannelAsync -21" << std::endl;
                            winrt::check_hresult(hr);
                        }
                    }
                    else
                    {
                        std::cout << "ELx - CreateChannelAsync -22" << std::endl;
                        wil::unique_cotaskmem_string unpackagedAppUserModelId;
                        RegisterUnpackagedApplicationHelper(remoteId, unpackagedAppUserModelId);

                        std::cout << "ELx - CreateChannelAsync -23" << std::endl;
                        PushNotificationChannelManager channelManager{};
                        winrt::PushNotificationChannel pushChannelReceived{ co_await channelManager.CreatePushNotificationChannelForApplicationAsync(unpackagedAppUserModelId.get()) };

                        std::cout << "ELx - CreateChannelAsync -24" << std::endl;
                        PushNotificationTelemetry::ChannelRequestedByApi(S_OK, remoteId);

                        std::cout << "ELx - CreateChannelAsync -25" << std::endl;
                        co_return winrt::make<PushNotificationCreateChannelResult>(
                            winrt::make<PushNotificationChannel>(pushChannelReceived),
                            S_OK,
                            PushNotificationChannelStatus::CompletedSuccess);
                    }
                }
                catch (...)
                {
                    std::cout << "ELx - CreateChannelAsync -26" << std::endl;
                    auto channelRequestException = hresult_error(to_hresult(), take_ownership_from_abi);

                    std::cout << "ELx - CreateChannelAsync -27" << std::endl;
                    if ((backOffTime <= c_maxBackoff) && IsChannelRequestRetryable(channelRequestException.code()))
                    {
                        std::cout << "ELx - CreateChannelAsync -28" << std::endl;
                        channelStatus.extendedError = channelRequestException.code();
                        channelStatus.status = PushNotificationChannelStatus::InProgressRetry;
                        channelStatus.retryCount = ++retryCount;

                        std::cout << "ELx - CreateChannelAsync -29" << std::endl;
                        progress(channelStatus);
                    }
                    else
                    {
                        std::cout << "ELx - CreateChannelAsync -30" << std::endl;
                        PushNotificationTelemetry::ChannelRequestedByApi(channelRequestException.code(), remoteId);

                        std::cout << "ELx - CreateChannelAsync -31" << std::endl;
                        co_return winrt::make<PushNotificationCreateChannelResult>(
                            nullptr,
                            channelRequestException.code(),
                            PushNotificationChannelStatus::CompletedFailure);
                    }
                }

                std::cout << "ELx - CreateChannelAsync -32" << std::endl;
                co_await winrt::resume_after(backOffTime);
            }
        }
        catch (...)
        {
            std::cout << "ELx - CreateChannelAsync -33" << std::endl;
            HRESULT hrError = wil::ResultFromCaughtException();
            PushNotificationTelemetry::ChannelRequestedByApi(hrError, remoteId);

            std::cout << "ELx - CreateChannelAsync -34" << std::endl;
            THROW_HR(hrError);
        }
    }

    void PushNotificationManager::RegisterActivator(PushNotificationActivationInfo const& details)
    {
        try
        {
            THROW_HR_IF_NULL(E_INVALIDARG, details);

            GUID taskClsid = details.TaskClsid();
            THROW_HR_IF(E_INVALIDARG, taskClsid == GUID_NULL);

            auto registrationActivators{ details.Activators() };

            auto isBackgroundTaskFlagSet{ WI_IsAnyFlagSet(registrationActivators, PushNotificationRegistrationActivators::PushTrigger | PushNotificationRegistrationActivators::ComActivator) };
            THROW_HR_IF(E_INVALIDARG, isBackgroundTaskFlagSet && !IsActivatorSupported(registrationActivators));

            auto isProtocolActivatorSet{ WI_IsFlagSet(registrationActivators, PushNotificationRegistrationActivators::ProtocolActivator) };
            THROW_HR_IF(E_INVALIDARG, isProtocolActivatorSet && !IsActivatorSupported(registrationActivators));

            if (isProtocolActivatorSet)
            {
                auto coInitialize = wil::CoInitializeEx();

                wil::com_ptr<INotificationsLongRunningPlatform> notificationPlatform{
                    wil::CoCreateInstance<NotificationsLongRunningPlatform, INotificationsLongRunningPlatform>(CLSCTX_LOCAL_SERVER) };

                wil::unique_cotaskmem_string processName;
                THROW_IF_FAILED(GetCurrentProcessPath(processName));
                THROW_IF_FAILED(notificationPlatform->RegisterLongRunningActivator(processName.get()));
            }

            BackgroundTaskBuilder builder{ nullptr };

            if (WI_IsFlagSet(registrationActivators, PushNotificationRegistrationActivators::PushTrigger))
            {
                {
                    auto lock = s_activatorInfoLock.lock_exclusive();
                    THROW_HR_IF(E_INVALIDARG, s_pushTriggerRegistration);
                }

                winrt::hstring taskClsidStr = winrt::to_hstring(taskClsid);
                winrt::hstring backgroundTaskFullName = backgroundTaskName + taskClsidStr;

                auto tasks = BackgroundTaskRegistration::AllTasks();
                bool isTaskRegistered = std::any_of(std::begin(tasks), std::end(tasks),
                    [&](auto&& task)
                    {
                        auto name = task.Value().Name();

                        if (std::wstring_view(name).substr(0, backgroundTaskName.size()) != backgroundTaskName)
                        {
                            return false;
                        }

                        if (name == backgroundTaskFullName)
                        {
                            s_pushTriggerRegistration = task.Value();
                            return true;
                        }

                        throw winrt::hresult_invalid_argument(L"RegisterActivator has different clsid registered.");
                    });

                if (!isTaskRegistered)
                {
                    builder = BackgroundTaskBuilder();
                    builder.Name(backgroundTaskFullName);

                    PushNotificationTrigger trigger{};
                    builder.SetTrigger(trigger);

                    THROW_HR_IF(E_NOTIMPL, !AppModel::Identity::IsPackagedProcess());

                    // In case the interface is not supported, let it throw.
                    auto builder5 = builder.as<winrt::IBackgroundTaskBuilder5>();
                    builder5.SetTaskEntryPointClsid(taskClsid);
                    winrt::com_array<winrt::IBackgroundCondition> conditions = details.GetConditions();
                    for (auto condition : conditions)
                    {
                        builder.AddCondition(condition);
                    }
                }
            }

            BackgroundTaskRegistration registeredTaskFromBuilder{ nullptr };

            auto scopeExitToCleanRegistrations = wil::scope_exit(
                [&]()
                {
                    s_comActivatorRegistration.reset();

                    // Clean the task registration only if it was created during this call
                    if (registeredTaskFromBuilder)
                    {
                        registeredTaskFromBuilder.Unregister(true);
                    }
                }
            );

            if (WI_IsFlagSet(registrationActivators, PushNotificationRegistrationActivators::ComActivator))
            {
                {
                    auto lock = s_activatorInfoLock.lock_exclusive();
                    THROW_HR_IF_MSG(E_INVALIDARG, s_comActivatorRegistration, "ComActivator already registered.");
                }

                GetWaitHandleForArgs().create();

                THROW_IF_FAILED(::CoRegisterClassObject(
                    taskClsid,
                    winrt::make<PushNotificationBackgroundTaskFactory>().get(),
                    CLSCTX_LOCAL_SERVER,
                    REGCLS_MULTIPLEUSE,
                    &s_comActivatorRegistration));
            }

            if (builder)
            {
                registeredTaskFromBuilder = builder.Register();
            }

            scopeExitToCleanRegistrations.release();

            auto lock = s_activatorInfoLock.lock_exclusive();
            s_pushTriggerRegistration = registeredTaskFromBuilder;

            PushNotificationTelemetry::ActivatorRegisteredByApi(S_OK, details.Activators());
        }

        catch(...)
        {
            HRESULT hrError = wil::ResultFromCaughtException();
            PushNotificationTelemetry::ActivatorRegisteredByApi(hrError, 
                details == nullptr ? PushNotificationRegistrationActivators::Undefined : details.Activators());

            THROW_HR(hrError);
        }
    }

    void PushNotificationManager::UnregisterActivator(PushNotificationRegistrationActivators const& activators)
    {
        try
        {
            auto lock = s_activatorInfoLock.lock_exclusive();
            if (WI_IsFlagSet(activators, PushNotificationRegistrationActivators::PushTrigger))
            {
                THROW_HR_IF_NULL_MSG(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), s_pushTriggerRegistration, "PushTrigger not registered.");
                s_pushTriggerRegistration.Unregister(true);
                s_pushTriggerRegistration = nullptr;
            }

            // Check for COM flag, a valid cookie
            if (WI_IsFlagSet(activators, PushNotificationRegistrationActivators::ComActivator))
            {
                THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !s_comActivatorRegistration, "ComActivator not registered.");
                s_comActivatorRegistration.reset();
            }

            if (WI_IsFlagSet(activators, PushNotificationRegistrationActivators::ProtocolActivator))
            {
                auto coInitialize = wil::CoInitializeEx();

                wil::com_ptr<INotificationsLongRunningPlatform> notificationPlatform{
                    wil::CoCreateInstance<NotificationsLongRunningPlatform, INotificationsLongRunningPlatform>(CLSCTX_LOCAL_SERVER) };

                wil::unique_cotaskmem_string processName;
                THROW_IF_FAILED(GetCurrentProcessPath(processName));
                THROW_IF_FAILED(notificationPlatform->UnregisterLongRunningActivator(processName.get()));
            }
        }
        catch (...)
        {
            HRESULT hrError = wil::ResultFromCaughtException();
            PushNotificationTelemetry::ActivatorUnregisteredByApi(hrError, activators);
            THROW_HR(hrError);
        }

        PushNotificationTelemetry::ActivatorUnregisteredByApi(S_OK, activators);
    }

    void PushNotificationManager::UnregisterAllActivators()
    {
        try
        {
            auto lock = s_activatorInfoLock.lock_exclusive();
            if (s_pushTriggerRegistration)
            {
                s_pushTriggerRegistration.Unregister(true);
                s_pushTriggerRegistration = nullptr;
            }

            s_comActivatorRegistration.reset();
        }
        catch(...)
        {
            HRESULT hrError = wil::ResultFromCaughtException();
            PushNotificationTelemetry::ActivatorUnregisteredByApi(hrError, PushNotificationRegistrationActivators::PushTrigger | PushNotificationRegistrationActivators::ComActivator);
            THROW_HR(hrError);
        }
        PushNotificationTelemetry::ActivatorUnregisteredByApi(S_OK, PushNotificationRegistrationActivators::PushTrigger | PushNotificationRegistrationActivators::ComActivator);
    }

    static bool HasBackgroundTaskEntryPointClsid() {
        return Metadata::ApiInformation::IsMethodPresent(L"Windows.ApplicationModel.Background.BackgroundTaskBuilder", L"SetTaskEntryPointClsid");
    }

    bool IsBackgroundTaskBuilderAvailable()
    {
        static bool hasSetTaskEntrypoint = HasBackgroundTaskEntryPointClsid();
        return hasSetTaskEntrypoint;
    }

    bool PushNotificationManager::IsActivatorSupported(PushNotificationRegistrationActivators const& activators)
    {
        THROW_HR_IF(E_INVALIDARG, activators == PushNotificationRegistrationActivators::Undefined);

        auto isBackgroundTaskFlagSet{ WI_IsAnyFlagSet(activators, PushNotificationRegistrationActivators::PushTrigger | PushNotificationRegistrationActivators::ComActivator) };
        auto isProtocolActivatorSet{ WI_IsFlagSet(activators, PushNotificationRegistrationActivators::ProtocolActivator) };

        THROW_HR_IF(E_INVALIDARG, isBackgroundTaskFlagSet && isProtocolActivatorSet); // Invalid flag combination
        if (AppModel::Identity::IsPackagedProcess() && IsBackgroundTaskBuilderAvailable())
        {
            if (isProtocolActivatorSet) // ProtocolActivator unsupported if COM activation is available
            {
                return false;
            }
            return isBackgroundTaskFlagSet;
        }
        else
        {
            return isProtocolActivatorSet;
        }
    }
}
