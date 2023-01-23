#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>

HRESULT ExecuteCommand(HWND hwnd, IContextMenu *pContextMenu, UINT nCmd)
{
    CMINVOKECOMMANDINFO info = { sizeof(info) };
    info.lpVerb = MAKEINTRESOURCEA(nCmd);
    info.hwnd = hwnd;
    info.fMask = CMIC_MASK_UNICODE;
    info.nShow = SW_SHOWNORMAL;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        info.fMask |= CMIC_MASK_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        info.fMask |= CMIC_MASK_CONTROL_DOWN;

    return pContextMenu->InvokeCommand(&info);
}

BOOL
ShowContextMenu(HWND hwnd, LPCWSTR pszPath, POINT pt, UINT uFlags, LPRECT prc)
{
    HRESULT hr;
    IShellFolder *pFolder = NULL;
    LPCITEMIDLIST pidlChild;
    IContextMenu *pContextMenu = NULL;
    LPITEMIDLIST pidl = NULL;

    hr = SHParseDisplayName(pszPath, 0, &pidl, 0, 0);
    if (FAILED(hr))
    {
        printf("SHParseDisplayName\n");
        return FALSE;
    }
    printf("%p\n", pidl);

    hr = SHBindToParent(pidl, IID_IShellFolder, (void **)&pFolder, &pidlChild);
    if (FAILED(hr))
    {
        printf("SHBindToParent\n");
        return FALSE;
    }
    printf("%p\n", pidlChild);

    hr = pFolder->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, NULL, (void **)&pContextMenu);
    if (FAILED(hr))
    {
        printf("IShellFolder::GetUIObjectOf %08lX\n", hr);
        return FALSE;
    }

    //SetSite

    HMENU hMenu = CreatePopupMenu();

    hr = pContextMenu->QueryContextMenu(hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, uFlags);
    if (FAILED(hr))
    {
        printf("IContextMenu::QueryContextMenu\n");
        return FALSE;
    }

    SetForegroundWindow(hwnd);
    UINT nID = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
                              pt.x, pt.y, 0, hwnd, prc);
    printf("%d\n", nID);

    hr = ExecuteCommand(hwnd, pContextMenu, nID);
    if (FAILED(hr))
    {
        printf("ExecuteCommand\n");
        return FALSE;
    }

    if (pFolder)
        pFolder->Release();
    if (pContextMenu)
        pContextMenu->Release();
    return TRUE;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR szWinDir[MAX_PATH];
    GetWindowsDirectory(szWinDir, MAX_PATH);
    SetDlgItemText(hwnd, edt1, szWinDir);
    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwnd, IDOK), &rc);
    WCHAR szText[MAX_PATH];
    POINT pt = { rc.right, rc.bottom };
    UINT uFlags = 0;
    if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
        uFlags |= CMF_NODEFAULT;
    if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
        uFlags |= CMF_VERBSONLY;
    if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
        uFlags |= CMF_NOVERBS;
    if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
        uFlags |= CMF_EXPLORE;
    switch (id)
    {
    case IDOK:
        GetDlgItemTextW(hwnd, edt1, szText, MAX_PATH);
        ShowContextMenu(hwnd, szText, pt, uFlags, &rc);
        break;
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    CoInitialize(NULL);
    InitCommonControls();

    DialogBox(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);

    CoUninitialize();
    return 0;
}
