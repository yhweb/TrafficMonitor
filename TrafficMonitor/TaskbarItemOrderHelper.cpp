#include "stdafx.h"
#include "TaskbarItemOrderHelper.h"
#include "Common.h"
#include "TrafficMonitor.h"

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

namespace {

bool IsStableItemId(const std::wstring& item_id)
{
    if (item_id.size() <= 2 || item_id[1] != L':')
        return false;
    return item_id[0] == L'B' || item_id[0] == L'P';
}

std::wstring EscapePluginItemId(const std::wstring& item_id)
{
    std::wstring escaped_id;
    escaped_id.reserve(item_id.size());
    for (wchar_t ch : item_id)
    {
        if (ch == L'%')
            escaped_id += L"%25";
        else if (ch == L',')
            escaped_id += L"%2C";
        else
            escaped_id.push_back(ch);
    }
    return escaped_id;
}

}  // namespace

CTaskbarItemOrderHelper::CTaskbarItemOrderHelper(bool displayed_only)
    : m_displayed_only(displayed_only)
{
}

void CTaskbarItemOrderHelper::Init()
{
    std::vector<CommonDisplayItem> items;
    for (const auto& item : theApp.m_plugins.AllDisplayItemsWithPlugins())
        items.push_back(item);
    Init(items);
}

void CTaskbarItemOrderHelper::Init(
    const std::vector<CommonDisplayItem>& available_items)
{
    m_all_item_in_default_order.clear();
    m_item_order.clear();
    for (const auto& item : available_items)
    {
        if (std::find(m_all_item_in_default_order.begin(),
            m_all_item_in_default_order.end(), item)
            == m_all_item_in_default_order.end())
        {
            m_all_item_in_default_order.push_back(item);
        }
        std::wstring item_id{ GetItemStableId(item) };
        if (!item_id.empty())
            m_item_order.push_back(item_id);
    }
    NormalizeItemOrder(true);
}

std::vector<CommonDisplayItem> CTaskbarItemOrderHelper::GetAllDisplayItemsWithOrder() const
{
    std::vector<CommonDisplayItem> items;
    for (const auto& item_id : m_item_order)
    {
        int item_index{ FindItemIndex(item_id) };
        if (item_index >= 0)
        {
            if (m_displayed_only && !IsItemDisplayed(m_all_item_in_default_order[item_index]))
            {
                continue;
            }
            items.push_back(m_all_item_in_default_order[item_index]);
        }
    }

    return items;
}

void CTaskbarItemOrderHelper::FromString(const std::wstring& str)
{
    m_item_order.clear();
    std::vector<std::wstring> item_ids;
    size_t start_pos{};
    while (start_pos <= str.size())
    {
        size_t end_pos{ str.find(L',', start_pos) };
        if (end_pos == std::wstring::npos)
            end_pos = str.size();
        std::wstring item_id{ str.substr(start_pos, end_pos - start_pos) };
        if (!item_id.empty())
            m_item_order.push_back(item_id);
        if (end_pos == str.size())
            break;
        start_pos = end_pos + 1;
    }
    NormalizeItemOrder(true);
}

void CTaskbarItemOrderHelper::FromLegacyString(const std::wstring& str)
{
    m_item_order.clear();
    size_t start_pos{};
    while (start_pos <= str.size())
    {
        size_t end_pos{ str.find(L',', start_pos) };
        if (end_pos == std::wstring::npos)
            end_pos = str.size();
        std::wstring value{ str.substr(start_pos, end_pos - start_pos) };
        int item_index{ _wtoi(value.c_str()) };
        if (item_index >= 0
            && item_index < static_cast<int>(m_all_item_in_default_order.size()))
        {
            std::wstring item_id{ GetItemStableId(m_all_item_in_default_order[item_index]) };
            if (!item_id.empty())
                m_item_order.push_back(item_id);
        }
        if (end_pos == str.size())
            break;
        start_pos = end_pos + 1;
    }
    NormalizeItemOrder(true);
}

std::wstring CTaskbarItemOrderHelper::ToString() const
{
    std::wstring result;
    for (const auto& item_id : m_item_order)
    {
        if (!item_id.empty())
        {
            result += item_id;
            result.push_back(L',');
        }
    }
    if (!result.empty())
        result.pop_back();
    return result;
}

std::wstring CTaskbarItemOrderHelper::ToLegacyString() const
{
    std::vector<int> item_order;
    for (const auto& item : GetAllDisplayItemsWithOrder())
    {
        int item_index{ FindItemIndex(GetItemStableId(item)) };
        if (item_index >= 0
            && std::find(item_order.begin(), item_order.end(), item_index) == item_order.end())
        {
            item_order.push_back(item_index);
        }
    }
    for (int i = 0; i < static_cast<int>(m_all_item_in_default_order.size()); i++)
    {
        if (std::find(item_order.begin(), item_order.end(), i) == item_order.end())
            item_order.push_back(i);
    }

    std::wstring result;
    for (int item_index : item_order)
    {
        result += std::to_wstring(item_index);
        result.push_back(L',');
    }
    if (!result.empty())
        result.pop_back();
    return result;
}

void CTaskbarItemOrderHelper::SetOrder(const vector<std::wstring>& item_order)
{
    m_item_order = item_order;
    NormalizeItemOrder(true);
}

const vector<std::wstring>& CTaskbarItemOrderHelper::GetItemOrderConst() const
{
    return m_item_order;
}

void CTaskbarItemOrderHelper::MoveItem(
    const std::wstring& item_id, bool move_up, bool displayed_only)
{
    auto current{ std::find(m_item_order.begin(), m_item_order.end(), item_id) };
    if (current == m_item_order.end())
        return;

    auto can_move_to = [this, displayed_only](const std::wstring& target_id)
    {
        int target_index{ FindItemIndex(target_id) };
        return target_index >= 0
            && (!displayed_only
                || IsItemDisplayed(m_all_item_in_default_order[target_index]));
    };

    auto target{ current };
    if (move_up)
    {
        while (target != m_item_order.begin())
        {
            --target;
            if (can_move_to(*target))
                break;
        }
        if (target == current || !can_move_to(*target))
            return;
    }
    else
    {
        while (target != m_item_order.end())
        {
            ++target;
            if (target != m_item_order.end() && can_move_to(*target))
                break;
        }
        if (target == m_item_order.end())
            return;
    }

    std::iter_swap(current, target);
}

void CTaskbarItemOrderHelper::RestoreDefaultOrder()
{
    m_item_order.clear();
    for (const auto& item : m_all_item_in_default_order)
    {
        std::wstring item_id{ GetItemStableId(item) };
        if (!item_id.empty())
            m_item_order.push_back(item_id);
    }
    NormalizeItemOrder(false);
}

CString CTaskbarItemOrderHelper::GetItemDisplayName(CommonDisplayItem item)
{
    return item.GetItemName();
}

bool CTaskbarItemOrderHelper::IsItemDisplayed(CommonDisplayItem item)
{
    bool displayed = true;
    if (!item.IsPlugin())
    {
        if ((item == TDI_CPU_TEMP) && !theApp.m_general_data.IsHardwareEnable(HI_CPU))
            displayed = false;
        if ((item == TDI_GPU_TEMP) && !theApp.m_general_data.IsHardwareEnable(HI_GPU))
            displayed = false;
        if ((item == TDI_HDD_TEMP) && !theApp.m_general_data.IsHardwareEnable(HI_HDD))
            displayed = false;
        if (item == TDI_MAIN_BOARD_TEMP && !theApp.m_general_data.IsHardwareEnable(HI_MBD))
            displayed = false;
    }

    return displayed;
}

std::wstring CTaskbarItemOrderHelper::GetItemStableId(CommonDisplayItem item)
{
    if (item.IsPlugin())
    {
        IPluginItem* plugin_item{ item.PluginItem() };
        if (plugin_item != nullptr)
        {
            return L"P:" + EscapePluginItemId(plugin_item->GetItemId());
        }
        return std::wstring();
    }

    switch (item.ItemType())
    {
    case TDI_UP:
        return L"B:UP";
    case TDI_DOWN:
        return L"B:DOWN";
    case TDI_CPU:
        return L"B:CPU";
    case TDI_MEMORY:
        return L"B:MEMORY";
    case TDI_GPU_USAGE:
        return L"B:GPU_USAGE";
    case TDI_CPU_TEMP:
        return L"B:CPU_TEMP";
    case TDI_GPU_TEMP:
        return L"B:GPU_TEMP";
    case TDI_HDD_TEMP:
        return L"B:HDD_TEMP";
    case TDI_MAIN_BOARD_TEMP:
        return L"B:MAIN_BOARD_TEMP";
    case TDI_HDD_USAGE:
        return L"B:HDD_USAGE";
    case TDI_TOTAL_SPEED:
        return L"B:TOTAL_SPEED";
    case TDI_CPU_FREQ:
        return L"B:CPU_FREQ";
    case TDI_TODAY_TRAFFIC:
        return L"B:TODAY_TRAFFIC";
    default:
        return std::wstring();
    }
}

int CTaskbarItemOrderHelper::FindItemIndex(const std::wstring& item_id) const
{
    for (int i = 0; i < static_cast<int>(m_all_item_in_default_order.size()); i++)
    {
        if (GetItemStableId(m_all_item_in_default_order[i]) == item_id)
            return i;
    }
    return -1;
}

void CTaskbarItemOrderHelper::NormalizeItemOrder(bool keep_unknown_items)
{
    std::vector<std::wstring> available_item_ids;
    for (const auto& item : m_all_item_in_default_order)
    {
        std::wstring item_id{ GetItemStableId(item) };
        if (!item_id.empty())
            available_item_ids.push_back(item_id);
    }

    std::vector<std::wstring> normalized_order;
    for (const auto& item_id : m_item_order)
    {
        bool is_available{
            std::find(available_item_ids.begin(), available_item_ids.end(),
                item_id) != available_item_ids.end() };
        if (IsStableItemId(item_id)
            && (keep_unknown_items || is_available)
            && std::find(normalized_order.begin(), normalized_order.end(), item_id) == normalized_order.end())
        {
            normalized_order.push_back(item_id);
        }
    }

    for (const auto& item : m_all_item_in_default_order)
    {
        std::wstring item_id{ GetItemStableId(item) };
        if (!item_id.empty()
            && std::find(normalized_order.begin(), normalized_order.end(), item_id) == normalized_order.end())
        {
            normalized_order.push_back(item_id);
        }
    }
    m_item_order.swap(normalized_order);
}
