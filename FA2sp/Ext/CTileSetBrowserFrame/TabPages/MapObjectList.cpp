#include "MapObjectList.h"

#include "../../../FA2sp.h"
#include "../../../Ext/CIsoView/Body.h"
#include "../../../Ext/CMapData/Body.h"
#include "../../../ExtraWindow/Common.h"
#include "../../../Helpers/Translations.h"
#include <CMapData.h>
#include <CObjectDatas.h>
#include <CIsoView.h>

#include <algorithm>
#include <cctype>
#include <commctrl.h>
#include <cstdlib>

MapObjectList MapObjectList::Instance;

namespace
{
    constexpr UINT RefreshCommand = 1;
    constexpr UINT SearchControl = 2;
    constexpr UINT ListControl = 3;
    constexpr UINT CountControl = 4;

    int CompareText(const FString& left, const FString& right)
    {
        return _stricmp(left.c_str(), right.c_str());
    }
}

void MapObjectList::Create(HWND hParent)
{
    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = static_cast<HINSTANCE>(FA2sp::hInstance);
    wc.lpszClassName = "FA2spMapObjectList";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    RegisterClassEx(&wc);

    RECT rect{};
    GetClientRect(hParent, &rect);
    m_hWnd = CreateWindowEx(
        0, wc.lpszClassName, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        2, 20, rect.right - 6, rect.bottom - 26,
        hParent, nullptr, wc.hInstance, this
    );
    CreateControls();
}

void MapObjectList::CreateControls()
{
    if (!m_hWnd)
        return;

    m_hSearch = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        4, 4, 260, 24, m_hWnd,
        reinterpret_cast<HMENU>(SearchControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hRefresh = CreateWindow(
        "BUTTON", Translations::TranslateOrDefault("MapObjectList.Refresh", "Refresh"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        270, 4, 80, 24, m_hWnd,
        reinterpret_cast<HMENU>(RefreshCommand),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hCount = CreateWindow(
        "STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        360, 8, 120, 18, m_hWnd,
        reinterpret_cast<HMENU>(CountControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hList = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS,
        4, 34, 476, 300, m_hWnd,
        reinterpret_cast<HMENU>(ListControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    ListView_SetExtendedListViewStyle(
        m_hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    const char* headers[] = {
        "Index", "TypeID", "Coordinate", "House", "Health"
    };
    const int widths[] = { 70, 150, 100, 130, 80 };
    for (int i = 0; i < Column::Count; ++i)
    {
        LVCOLUMN column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<char*>(headers[i]);
        column.cx = widths[i];
        column.iSubItem = i;
        ListView_InsertColumn(m_hList, i, &column);
    }

    FString text;
    const char* translationKeys[] = {
        "MapObjectList.Index", "MapObjectList.TypeID",
        "MapObjectList.Coordinate", "MapObjectList.House",
        "MapObjectList.Health"
    };
    for (int i = 0; i < Column::Count; ++i)
    {
        if (!Translations::GetTranslationItem(translationKeys[i], text))
            continue;
        LVCOLUMN column{};
        column.mask = LVCF_TEXT;
        column.pszText = const_cast<char*>(text.c_str());
        ListView_SetColumn(m_hList, i, &column);
    }

    Refresh();
}

void MapObjectList::Refresh()
{
    if (!m_hWnd)
        return;

    LoadRows();
    ApplyFilterAndSort();
}

void MapObjectList::LoadRows()
{
    m_rows.clear();

    auto loadSection = [this](const char* section, ObjectType type)
    {
        auto pSection = CMapData::Instance->INI.GetSection(section);
        if (!pSection)
            return;

        int index = 0;
        for (auto& pair : pSection->GetEntities())
        {
            auto values = FString::SplitString(pair.second);
            if (values.size() < 5)
            {
                ++index;
                continue;
            }

            Row row;
            row.Index = index++;
            row.Type = type;
            row.X = atoi(values[3]);
            row.Y = atoi(values[4]);

            switch (type)
            {
            case ObjectType::Infantry:
            {
                CInfantryData data;
                CMapData::Instance->GetInfantryData(row.Index, data);
                row.TypeID = data.TypeID;
                row.House = Translations::ParseHouseName(data.House, true);
                row.Health = data.Health;
                break;
            }
            case ObjectType::Building:
            {
                CBuildingData data;
                CMapData::Instance->GetBuildingData(row.Index, data);
                row.TypeID = data.TypeID;
                row.House = Translations::ParseHouseName(data.House, true);
                row.Health = data.Health;
                break;
            }
            case ObjectType::Aircraft:
            {
                CAircraftData data;
                CMapData::Instance->GetAircraftData(row.Index, data);
                row.TypeID = data.TypeID;
                row.House = Translations::ParseHouseName(data.House, true);
                row.Health = data.Health;
                break;
            }
            case ObjectType::Unit:
            {
                CUnitData data;
                CMapData::Instance->GetUnitData(row.Index, data);
                row.TypeID = data.TypeID;
                row.House = Translations::ParseHouseName(data.House, true);
                row.Health = data.Health;
                break;
            }
            }

            m_rows.push_back(std::move(row));
        }
    };

    loadSection("Infantry", ObjectType::Infantry);
    loadSection("Units", ObjectType::Unit);
    loadSection("Structures", ObjectType::Building);
    loadSection("Aircraft", ObjectType::Aircraft);
}

bool MapObjectList::MatchesFilter(const Row& row, const FString& filter) const
{
    if (filter.empty())
        return true;

    FString all = GetCellText(row, Column::Index);
    all += " ";
    all += row.TypeID;
    all += " ";
    all += GetCellText(row, Column::Coordinate);
    all += " ";
    all += row.House;
    all += " ";
    all += row.Health;

    auto equalIgnoreCase = [](char left, char right)
    {
        return std::tolower(static_cast<unsigned char>(left)) ==
            std::tolower(static_cast<unsigned char>(right));
    };
    return std::search(
        all.begin(), all.end(), filter.begin(), filter.end(), equalIgnoreCase
    ) != all.end();
}

void MapObjectList::ApplyFilterAndSort()
{
    char buffer[512]{};
    if (m_hSearch)
        GetWindowText(m_hSearch, buffer, sizeof(buffer) - 1);
    FString filter = buffer;

    m_visibleRows.clear();
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
    {
        if (MatchesFilter(m_rows[i], filter))
            m_visibleRows.push_back(i);
    }

    std::stable_sort(m_visibleRows.begin(), m_visibleRows.end(),
        [this](int leftIndex, int rightIndex)
        {
            const auto& left = m_rows[leftIndex];
            const auto& right = m_rows[rightIndex];
            if (m_sortColumn == Column::Index)
            {
                if (left.Index != right.Index)
                    return m_sortAscending ? left.Index < right.Index : left.Index > right.Index;
                return static_cast<int>(left.Type) < static_cast<int>(right.Type);
            }
            if (m_sortColumn == Column::Health)
            {
                const int l = atoi(left.Health);
                const int r = atoi(right.Health);
                if (l != r)
                    return m_sortAscending ? l < r : l > r;
            }
            else
            {
                const int result = CompareText(
                    GetCellText(left, static_cast<Column>(m_sortColumn)),
                    GetCellText(right, static_cast<Column>(m_sortColumn))
                );
                if (result != 0)
                    return m_sortAscending ? result < 0 : result > 0;
            }
            return left.Index < right.Index;
        });

    RebuildList();
}

FString MapObjectList::GetCellText(const Row& row, Column column) const
{
    FString text;
    switch (column)
    {
    case Column::Index:
        text.Format("%d", row.Index);
        break;
    case Column::TypeID:
        text = row.TypeID;
        break;
    case Column::Coordinate:
        text.Format("%d, %d", row.X, row.Y);
        break;
    case Column::House:
        text = row.House;
        break;
    case Column::Health:
        text = row.Health;
        break;
    default:
        break;
    }
    return text;
}

void MapObjectList::RebuildList()
{
    if (!m_hList)
        return;

    ListView_DeleteAllItems(m_hList);
    for (int rowIndex : m_visibleRows)
    {
        const auto& row = m_rows[rowIndex];
        LVITEM item{};
        item.mask = LVIF_PARAM | LVIF_TEXT;
        item.iItem = ListView_GetItemCount(m_hList);
        item.lParam = rowIndex;
        FString text = GetCellText(row, Column::Index);
        item.pszText = const_cast<char*>(text.c_str());
        int displayIndex = ListView_InsertItem(m_hList, &item);
        for (int column = Column::TypeID; column < Column::Count; ++column)
        {
            text = GetCellText(row, static_cast<Column>(column));
            ListView_SetItemText(m_hList, displayIndex, column, const_cast<char*>(text.c_str()));
        }
    }
    UpdateCount();
}

void MapObjectList::UpdateCount()
{
    if (!m_hCount)
        return;

    FString text;
    FString label = Translations::TranslateOrDefault("MapObjectList.Count", "Count");
    text.Format("%s: %d", label.c_str(), static_cast<int>(m_visibleRows.size()));
    SetWindowText(m_hCount, text);
}

void MapObjectList::SortByColumn(int column)
{
    if (m_sortColumn == column)
        m_sortAscending = !m_sortAscending;
    else
    {
        m_sortColumn = column;
        m_sortAscending = true;
    }
    ApplyFilterAndSort();
}

void MapObjectList::HandleSelection(bool doubleClick)
{
    int selected = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (selected < 0)
        return;

    LVITEM item{};
    item.mask = LVIF_PARAM;
    item.iItem = selected;
    if (!ListView_GetItem(m_hList, &item) ||
        item.lParam < 0 || item.lParam >= static_cast<LPARAM>(m_rows.size()))
        return;

    const auto& row = m_rows[item.lParam];
    CIsoViewExt::MoveToMapCoord(row.X, row.Y);
    if (doubleClick)
        OpenProperties(row);
}

void MapObjectList::OpenProperties(const Row& row)
{
    CIsoView::GetInstance()->HandleProperties(row.Index, static_cast<int>(row.Type));
    Refresh();
}

LRESULT CALLBACK MapObjectList::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<MapObjectList*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
    );
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<MapObjectList*>(create->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hWnd = hWnd;
    }
    return self ? self->HandleMessage(hWnd, message, wParam, lParam)
                : DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT MapObjectList::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        OnSize();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == SearchControl && HIWORD(wParam) == EN_CHANGE)
            ApplyFilterAndSort();
        else if (LOWORD(wParam) == RefreshCommand && HIWORD(wParam) == BN_CLICKED)
            Refresh();
        return 0;
    case WM_NOTIFY:
    {
        auto* header = reinterpret_cast<LPNMHDR>(lParam);
        if (header->idFrom == ListControl && header->code == LVN_COLUMNCLICK)
        {
            auto* info = reinterpret_cast<LPNMLISTVIEW>(lParam);
            SortByColumn(info->iSubItem);
        }
        else if (header->idFrom == ListControl &&
            (header->code == LVN_ITEMCHANGED || header->code == NM_DBLCLK))
        {
            HandleSelection(header->code == NM_DBLCLK);
        }
        return 0;
    }
    case 114514:
        Refresh();
        return 0;
    case WM_DESTROY:
        m_hWnd = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void MapObjectList::OnSize() const
{
    if (!m_hWnd)
        return;

    RECT parentRect{};
    HWND parent = GetParent(m_hWnd);
    GetClientRect(parent, &parentRect);
    int tabPageheight = 20 * CFinalSunAppExt::ProgramScaleFactor;
    ::MoveWindow(
        m_hWnd, 2, tabPageheight,
        std::max(100, static_cast<int>(parentRect.right) - 6),
        std::max(40, static_cast<int>(parentRect.bottom) - tabPageheight - 6),
        FALSE
    );

    RECT rect{};
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    ::MoveWindow(m_hSearch, 4, 4, std::max(80, width - 220), 24, TRUE);
    ::MoveWindow(m_hRefresh, std::max(88, width - 210), 4, 80, 24, TRUE);
    ::MoveWindow(m_hCount, std::max(180, width - 125), 8, 120, 18, TRUE);
    ::MoveWindow(m_hList, 4, 34, std::max(100, width - 8), std::max(40, height - 38), TRUE);
}

void MapObjectList::ShowWindow(bool bShow) const
{
    ::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void MapObjectList::ShowWindow() const
{
    const_cast<MapObjectList*>(this)->Refresh();
    ShowWindow(true);
}

void MapObjectList::HideWindow() const
{
    ShowWindow(false);
}

bool MapObjectList::IsValid() const
{
    return m_hWnd != nullptr;
}

bool MapObjectList::IsVisible() const
{
    return IsValid() && ::IsWindowVisible(m_hWnd);
}

HWND MapObjectList::GetHwnd() const
{
    return m_hWnd;
}

MapObjectList::operator HWND() const
{
    return GetHwnd();
}
