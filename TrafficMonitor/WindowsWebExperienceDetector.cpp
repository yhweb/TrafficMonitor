#include "stdafx.h"
#include "WindowsWebExperienceDetector.h"
#ifndef DISABLE_WINDOWS_WEB_EXPERIENCE_DETECTOR
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Storage.h>
#pragma comment(lib, "RuntimeObject.lib")
#endif
#include "TrafficMonitor.h"

bool WindowsWebExperienceDetector::IsDetected() noexcept
{
#ifndef DISABLE_WINDOWS_WEB_EXPERIENCE_DETECTOR
    if (theApp.m_win_version.IsWindows11OrLater())
    {
        try
        {
            auto manager = winrt::Windows::Management::Deployment::PackageManager{};
            auto packages = manager.FindPackagesForUser(
                {},
                winrt::hstring{ L"MicrosoftWindows.Client.WebExperience" },
                winrt::hstring{
                    L"CN=Microsoft Windows, O=Microsoft Corporation, L=Redmond, S=Washington, C=US" });
            auto result = packages.First().HasCurrent();
            return result;
        }
        catch (...)
        {
            // FindPackagesForUser 需要系统 packageQuery 能力，非管理员/沙箱进程会抛
            // E_ACCESSDENIED。本函数为 noexcept，异常一旦逃逸即 std::terminate -> abort
        }
    }
#endif
    return false;
}
