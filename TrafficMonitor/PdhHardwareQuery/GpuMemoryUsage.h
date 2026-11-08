#pragma once
#include "PdhQuery.h"

class CPdhGPUMemoryUsage : public CPdhQuery
{
public:
    CPdhGPUMemoryUsage();
    ~CPdhGPUMemoryUsage();

    // Gets dedicated GPU memory usage (0-100).
    bool GetGpuMemoryUsage(/*out*/ int& usage);

private:
    void SelectDedicatedAdapter();

private:
    bool m_isAvailable{};
    std::wstring m_adapter_luid_prefix;
    SIZE_T m_dedicated_video_memory{};
};
