#include "stdafx.h"
#include "GpuMemoryUsage.h"
#include <dxgi1_2.h>

#pragma comment(lib, "DXGI.lib")

CPdhGPUMemoryUsage::CPdhGPUMemoryUsage()
    : CPdhQuery(_T("\\GPU Adapter Memory(*)\\Dedicated Usage"))
{
    SelectDedicatedAdapter();
    m_isAvailable = isInitialized && !m_adapter_luid_prefix.empty()
        && m_dedicated_video_memory > 0;
}

CPdhGPUMemoryUsage::~CPdhGPUMemoryUsage()
{
}

void CPdhGPUMemoryUsage::SelectDedicatedAdapter()
{
    IDXGIFactory1* factory{};
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) || factory == nullptr)
        return;

    for (UINT index = 0; ; ++index)
    {
        IDXGIAdapter1* adapter{};
        HRESULT hr = factory->EnumAdapters1(index, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(hr) || adapter == nullptr)
            break;

        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc))
            && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
            && desc.DedicatedVideoMemory > m_dedicated_video_memory)
        {
            m_dedicated_video_memory = desc.DedicatedVideoMemory;
            wchar_t luid_prefix[64]{};
            swprintf_s(luid_prefix,
                L"luid_0x%08lX_0x%08lX_",
                static_cast<unsigned long>(desc.AdapterLuid.HighPart),
                static_cast<unsigned long>(desc.AdapterLuid.LowPart));
            m_adapter_luid_prefix = luid_prefix;
        }
        adapter->Release();
        adapter = nullptr;
    }
    factory->Release();
}

bool CPdhGPUMemoryUsage::GetGpuMemoryUsage(int& usage)
{
    usage = 0;
    if (!m_isAvailable)
        return false;

    std::vector<CounterValueItem> values;
    if (!QueryValues(values) || values.empty())
        return false;

    CString adapter_luid_prefix(m_adapter_luid_prefix.c_str());
    adapter_luid_prefix.MakeLower();
    double dedicated_usage{};
    bool instance_found{};
    for (const auto& item : values)
    {
        CString instance_name(item.name.c_str());
        instance_name.MakeLower();
        if (instance_name.Left(adapter_luid_prefix.GetLength()) == adapter_luid_prefix)
        {
            dedicated_usage += item.value;
            instance_found = true;
        }
    }

    if (!instance_found)
        return false;

    double usage_percent = dedicated_usage * 100.0
        / static_cast<double>(m_dedicated_video_memory);
    usage = static_cast<int>(usage_percent + 0.5);
    usage = min(max(usage, 0), 100);
    return true;
}
