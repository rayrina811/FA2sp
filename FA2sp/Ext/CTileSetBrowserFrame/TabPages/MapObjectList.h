#pragma once

#include "../Body.h"

#include <vector>

class MapObjectList
{
public:
    static MapObjectList Instance;

    MapObjectList() = default;

    void Create(HWND hParent);
    void OnSize() const;
    void ShowWindow(bool bShow) const;
    void ShowWindow() const;
    void HideWindow() const;
    bool IsValid() const;
    bool IsVisible() const;
    void Refresh();
    HWND GetHwnd() const;
    operator HWND() const;

private:
    enum class ObjectType : int
    {
        Infantry = 0,
        Building = 1,
        Aircraft = 2,
        Unit = 3
    };

    struct Row
    {
        int Index = -1;
        ObjectType Type = ObjectType::Infantry;
        int X = -1;
        int Y = -1;
        FString TypeID;
        FString House;
        FString Health;
    };

    enum Column : int
    {
        Index = 0,
        Type,
        TypeID,
        Coordinate,
        House,
        Health,
        Count
    };

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void CreateControls();
    void LoadRows();
    void ApplyFilterAndSort();
    void RebuildList();
    void HandleSelection(bool doubleClick);
    void SortByColumn(int column);
    bool MatchesFilter(const Row& row, const FString& filter) const;
    void AddRow(ObjectType type, int index, const FString& value);
    void UpdateCount();
    FString GetCellText(const Row& row, Column column) const;
    void OpenProperties(const Row& row);

    HWND m_hWnd = nullptr;
    HWND m_hSearch = nullptr;
    HWND m_hClear = nullptr;
    HWND m_hRefresh = nullptr;
    HWND m_hCount = nullptr;
    HWND m_hList = nullptr;
    std::vector<Row> m_rows;
    std::vector<int> m_visibleRows;
    int m_sortColumn = Column::Index;
    bool m_sortAscending = true;
    bool m_dataDirty = true;
    bool m_refreshPosted = false;
};
