﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <EnvironmentManager.h>
#include <EnvironmentManager.g.cpp>
#include <EnvironmentVariableChangeTracker.h>

namespace winrt::Microsoft::ProjectReunion::implementation
{

    EnvironmentManager::EnvironmentManager(Scope const& scope)
        : m_Scope(scope) { }

    ProjectReunion::EnvironmentManager EnvironmentManager::GetForProcess()
    {
        ProjectReunion::EnvironmentManager environmentManager{ nullptr };
        environmentManager = winrt::make<implementation::EnvironmentManager>(Scope::Process);
        return environmentManager;
    }

    Microsoft::ProjectReunion::EnvironmentManager EnvironmentManager::GetForUser()
    {
        ProjectReunion::EnvironmentManager environmentManager{ nullptr };
        environmentManager = winrt::make<implementation::EnvironmentManager>(Scope::User);
        return environmentManager;
    }

    Microsoft::ProjectReunion::EnvironmentManager EnvironmentManager::GetForMachine()
    {
        ProjectReunion::EnvironmentManager environmentManager{ nullptr };
        environmentManager = winrt::make<implementation::EnvironmentManager>(Scope::Machine);
        return environmentManager;
    }

    Windows::Foundation::Collections::IMapView<hstring, hstring> EnvironmentManager::GetEnvironmentVariables()
    {
        Windows::Foundation::Collections::StringMap environmentVariables;

        if (m_Scope == Scope::Process)
        {
            environmentVariables = GetProcessEnvironmentVariables();
        }
        else
        {
            environmentVariables = GetUserOrMachineEnvironmentVariables();
        }

        return environmentVariables.GetView();
    }

    hstring EnvironmentManager::GetEnvironmentVariable(hstring const& variableName)
    {
        if (variableName.empty())
        {
            THROW_HR(E_INVALIDARG);
        }

        if (m_Scope == Scope::Process)
        {
            return hstring(GetProcessEnvironmentVariable(std::wstring(variableName)));
        }
        else
        {
            return hstring(GetUserOrMachineEnvironmentVariable(std::wstring(variableName)));
        }
    }

    void EnvironmentManager::SetEnvironmentVariable(hstring const& name, hstring const& value)
    {
        // If we are running in process scope it is okay to
       // set EV's because they will not be tracked.

       // Disallow sets if current OS is 20H0 or lower because
       // tracking EV changes relies on a feature in builds above 20H0
        if (m_Scope != Scope::Process && IsCurrentOS20H2OrLower())
        {
            THROW_HR(E_NOTIMPL);
        }

        auto setEV = [&, name, value, this]()
        {
            if (m_Scope == Scope::Process)
            {
                BOOL result = FALSE;
                if (!value.empty())
                {
                    result = ::SetEnvironmentVariable(name.c_str(), value.c_str());
                }
                else
                {
                    result = ::SetEnvironmentVariable(name.c_str(), nullptr);
                }

                if (result == TRUE)
                {
                    return S_OK;
                }
                else
                {
                    DWORD lastError = GetLastError();
                    RETURN_HR(HRESULT_FROM_WIN32(lastError));
                }
            }

            // m_Scope should be user or machine here.
            wil::unique_hkey environmentVariableKey = GetRegHKeyForEVUserAndMachineScope(true);

            if (!value.empty())
            {
                THROW_IF_FAILED(HRESULT_FROM_WIN32(RegSetValueEx(
                    environmentVariableKey.get()
                    , name.c_str()
                    , 0
                    , REG_SZ
                    , reinterpret_cast<const BYTE*>(value.c_str())
                    , static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)))));
            }
            else
            {
                THROW_IF_FAILED(HRESULT_FROM_WIN32(RegDeleteValue(environmentVariableKey.get()
                    , name.c_str())));
            }

            LRESULT broadcastResult = SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE,
                reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(L"Environment"),
                SMTO_NOTIMEOUTIFNOTHUNG | SMTO_BLOCK, 1000, nullptr);

            if (broadcastResult == 0)
            {
                THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
            }

            return S_OK;
        };

        EnvironmentVariableChangeTracker changeTracker(std::wstring(name), std::wstring(value), m_Scope);

        THROW_IF_FAILED(changeTracker.TrackChange(setEV));
    }

    void EnvironmentManager::AppendToPath(hstring const& path)
    {
        throw hresult_not_implemented();
    }

    void EnvironmentManager::RemoveFromPath(hstring const& path)
    {
        throw hresult_not_implemented();
    }

    void EnvironmentManager::AddExecutableFileExtension(hstring const& pathExt)
    {
        throw hresult_not_implemented();
    }

    void EnvironmentManager::RemoveExecutableFileExtension(hstring const& pathExt)
    {
        throw hresult_not_implemented();
    }

    StringMap EnvironmentManager::GetProcessEnvironmentVariables() const
    {
        //Get the pointer to the process block
        PWSTR environmentVariablesString{ GetEnvironmentStrings() };
        THROW_HR_IF_NULL(E_POINTER, environmentVariablesString);

        StringMap environmentVariables;
        for (auto environmentVariableOffset = environmentVariablesString; *environmentVariableOffset; environmentVariableOffset += wcslen(environmentVariableOffset) + 1)
        {
            auto delimiter{ wcschr(environmentVariableOffset, L'=') };
            FAIL_FAST_HR_IF_NULL(E_UNEXPECTED, delimiter);
            std::wstring variableName(environmentVariableOffset, 0, delimiter - environmentVariableOffset);
            auto variableValue{ delimiter + 1 };
            environmentVariables.Insert(variableName, variableValue);
        }

        THROW_IF_WIN32_BOOL_FALSE(FreeEnvironmentStrings(environmentVariablesString));

        return environmentVariables;
    }

    std::wstring EnvironmentManager::GetProcessEnvironmentVariable(const std::wstring variableName) const
    {
        // Get the size of the buffer.
        DWORD sizeNeededInCharacters = ::GetEnvironmentVariable(variableName.c_str(), nullptr, 0);

        // If we got an error
        if (sizeNeededInCharacters == 0)
        {
            DWORD lastError = GetLastError();

            if (lastError == ERROR_ENVVAR_NOT_FOUND)
            {
                return L"";
            }
            else
            {
                THROW_HR(HRESULT_FROM_WIN32(lastError));
            }
        }

        std::wstring environmentVariableValue{};

        environmentVariableValue.resize(sizeNeededInCharacters - 1);
        DWORD getResult = ::GetEnvironmentVariable(variableName.c_str(), &environmentVariableValue[0], sizeNeededInCharacters);

        if (getResult == 0)
        {
            THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
        }

        return environmentVariableValue;
    }

    StringMap EnvironmentManager::GetUserOrMachineEnvironmentVariables() const
    {
        StringMap environmentVariables;
        wil::unique_hkey environmentVariablesHKey = GetRegHKeyForEVUserAndMachineScope();

        // While this way of calculating the max size of the names,
        // values, and total number of entries includes two calls
        // to the registry, I believe this is superior to
        // using a do/while or a while with a prime
        // because there is no chance of the loop iterating more than
        // is needed AND the size of the name and value arrays are
        // only as big as the biggest name or value.

        DWORD sizeOfLongestNameInCharacters;
        DWORD sizeOfLongestValueInCharacters;
        DWORD numberOfValues;

        THROW_IF_WIN32_ERROR(RegQueryInfoKeyW(environmentVariablesHKey.get(),
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &numberOfValues, &sizeOfLongestNameInCharacters,
            &sizeOfLongestValueInCharacters, nullptr, nullptr));

        // +1 for null character
        const DWORD c_nameLength = sizeOfLongestNameInCharacters + 1;
        const DWORD c_valueSizeInBytes{ static_cast<DWORD>(sizeOfLongestValueInCharacters * sizeof(WCHAR)) };

        std::unique_ptr<wchar_t[]> environmentVariableName(new wchar_t[c_nameLength]);
        std::unique_ptr<BYTE[]> environmentVariableValue(new BYTE[c_valueSizeInBytes]);

        for (DWORD valueIndex = 0; valueIndex < numberOfValues; valueIndex++)
        {
            DWORD nameLength = c_nameLength;
            DWORD valueSize = c_valueSizeInBytes;
            LSTATUS enumerationStatus = RegEnumValueW(environmentVariablesHKey.get()
                , valueIndex
                , environmentVariableName.get()
                , &nameLength
                , nullptr
                , nullptr
                , environmentVariableValue.get()
                , &valueSize);

            // An empty name indicates the default value.
            if (nameLength == 0)
            {
                continue;
            }

            // If there was an error getting the value
            if (enumerationStatus != ERROR_SUCCESS && enumerationStatus != ERROR_NO_MORE_ITEMS)
            {
                THROW_HR(HRESULT_FROM_WIN32(enumerationStatus));
            }
            environmentVariableValue.get()[valueSize] = L'\0';
            environmentVariables.Insert(environmentVariableName.get(), reinterpret_cast<PWSTR>(environmentVariableValue.get()));

            environmentVariableName.get()[0] = L'\0';
            environmentVariableValue.reset(new BYTE[c_valueSizeInBytes]);
        }

        return environmentVariables;
    }

    std::wstring EnvironmentManager::GetUserOrMachineEnvironmentVariable(const std::wstring variableName) const
    {
        wil::unique_hkey environmentVariableHKey = GetRegHKeyForEVUserAndMachineScope();

        DWORD sizeOfEnvironmentValue;

        // See how big we need the buffer to be
        LSTATUS queryResult = RegQueryValueEx(environmentVariableHKey.get(), variableName.c_str(), 0, nullptr, nullptr, &sizeOfEnvironmentValue);

        if (queryResult != ERROR_SUCCESS)
        {
            if (queryResult == ERROR_FILE_NOT_FOUND)
            {
                return L"";
            }

            THROW_HR(HRESULT_FROM_WIN32((queryResult)));
        }

        std::unique_ptr<wchar_t[]> environmentValue(new wchar_t[sizeOfEnvironmentValue]);
        THROW_IF_FAILED(HRESULT_FROM_WIN32((RegQueryValueEx(environmentVariableHKey.get(), variableName.c_str(), 0, nullptr, (LPBYTE)environmentValue.get(), &sizeOfEnvironmentValue))));

        return std::wstring(environmentValue.get());
    }

    wil::unique_hkey EnvironmentManager::GetRegHKeyForEVUserAndMachineScope(bool needsWriteAccess) const
    {
        FAIL_FAST_HR_IF(E_INVALIDARG, m_Scope == Scope::Process);

        REGSAM registrySecurity = KEY_READ;

        if (needsWriteAccess)
        {
            registrySecurity |= KEY_WRITE;
        }

        wil::unique_hkey environmentVariablesHKey;
        if (m_Scope == Scope::User)
        {
            THROW_IF_FAILED(HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_CURRENT_USER, USER_EV_REG_LOCATION, 0, registrySecurity, environmentVariablesHKey.addressof())));
        }
        else //Scope is Machine
        {
            THROW_IF_FAILED(HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_LOCAL_MACHINE, MACHINE_EV_REG_LOCATION, 0, registrySecurity, environmentVariablesHKey.addressof())));
        }

        return environmentVariablesHKey;
    }
}
