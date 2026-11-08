#pragma once
#include "DisplayItem.h"

class CTaskbarItemOrderHelper
{
public:
    CTaskbarItemOrderHelper(bool displayed_only = false);

    void Init();
    void Init(const std::vector<CommonDisplayItem>& available_items);

    //根据设定的顺序获取任务窗口每个显示项目
    std::vector<CommonDisplayItem> GetAllDisplayItemsWithOrder() const;

    void FromString(const std::wstring& str);
    void FromLegacyString(const std::wstring& str);
    std::wstring ToString() const;
    std::wstring ToLegacyString() const;
    void SetOrder(const vector<std::wstring>& item_order);
    const vector<std::wstring>& GetItemOrderConst() const;
    void MoveItem(const std::wstring& item_id, bool move_up,
        bool displayed_only = false);
    void RestoreDefaultOrder();

    static CString GetItemDisplayName(CommonDisplayItem item);
    static bool IsItemDisplayed(CommonDisplayItem item);
    static std::wstring GetItemStableId(CommonDisplayItem item);

private:
    // Removes invalid or duplicate IDs and appends new items without reordering.
    void NormalizeItemOrder(bool keep_unknown_items);
    int FindItemIndex(const std::wstring& item_id) const;

private:
    vector<std::wstring> m_item_order;   //保存每个项目的稳定ID顺序
    vector<CommonDisplayItem> m_all_item_in_default_order;    //当前可用的默认显示项目
    bool m_displayed_only;
};
