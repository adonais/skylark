/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2022 Hua andy <hua.andy@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/
#include "framework.h"

LIST_HEAD(list_server);

#define REMOTEFS_HANDLE(hdlg, resi, sw)     \
do {                                        \
    HWND hc = GetDlgItem(hdlg, resi);       \
    if (hc)                                 \
    {                                       \
        if (sw == SW_HIDE)                  \
        {                                   \
            ShowWindow(hc, sw);             \
        }                                   \
        else                                \
        {                                   \
            ShowWindow(hc, SW_SHOW);        \
        }                                   \
    }                                       \
}                                           \
while (0)

#define GET_DLG_DATA(hdlg, psrv)                                                                  \
do                                                                                                \
{                                                                                                 \
    TCHAR servername[100+1];                                                                      \
    TCHAR protocol[20+1];                                                                         \
    TCHAR networkaddr[MAX_PATH+1];                                                                \
    TCHAR numbuf[20+1];                                                                           \
    TCHAR user[20+1];                                                                             \
    TCHAR pwd[32+1];                                                                              \
    TCHAR passphrase[32+1];                                                                       \
    TCHAR key_path[MAX_PATH+1];                                                                   \
    GetWindowText(GetDlgItem(hdlg, IDC_FILESERVER_NAME_EDIT), servername,100);                    \
    GetWindowText(GetDlgItem(hdlg, IDC_COMMPROTOCOL_COMBOBOX),protocol,20);                       \
    GetWindowText(GetDlgItem(hdlg, IDC_NETWORK_PORT_EDIT), numbuf, 20);                           \
    GetWindowText(GetDlgItem(hdlg, IDC_NETWORK_ADDRESS_EDIT),networkaddr,MAX_PATH);               \
    GetWindowText(GetDlgItem(hdlg, IDC_LOGIN_USER_EDIT), user,20);                                \
    GetWindowText(GetDlgItem(hdlg, IDC_LOGIN_PASS_EDIT),pwd,32);                                  \
    GetWindowText(GetDlgItem(hdlg, IDC_LOGIN_PASSPHRASE_EDIT),passphrase,32);                     \
    GetWindowText(GetDlgItem(hdlg, IDC_LOGIN_PRIVATE_EDIT),key_path,MAX_PATH);                    \
    psrv->port = _tstoi(numbuf);                                                                  \
    WideCharToMultiByte(CP_UTF8, 0, servername, -1, psrv->servername, 100, NULL, NULL);           \
    WideCharToMultiByte(CP_UTF8, 0, protocol, -1, psrv->protocol, 20, NULL, NULL);                \
    WideCharToMultiByte(CP_UTF8, 0, networkaddr, -1, psrv->networkaddr, MAX_PATH, NULL, NULL);    \
    WideCharToMultiByte(CP_UTF8, 0, user, -1, psrv->user, 20, NULL, NULL);                        \
    WideCharToMultiByte(CP_UTF8, 0, pwd, -1, psrv->pwd, 32, NULL, NULL);                          \
    WideCharToMultiByte(CP_UTF8, 0, passphrase, -1, psrv->passphrase, 32, NULL, NULL);            \
    WideCharToMultiByte(CP_UTF8, 0, key_path, -1, psrv->key_path, MAX_PATH, NULL, NULL);          \
}                                                                                                 \
while(0)

#define SET_DLG_DATA(hdlg, psrv)                                                                  \
do                                                                                                \
{                                                                                                 \
    TCHAR servername[100+1];                                                                      \
    TCHAR protocol[20+1];                                                                         \
    TCHAR networkaddr[MAX_PATH+1];                                                                \
    TCHAR numbuf[20+1];                                                                           \
    TCHAR user[20+1];                                                                             \
    TCHAR pwd[32+1];                                                                              \
    TCHAR passphrase[32+1];                                                                       \
    TCHAR key_path[MAX_PATH+1];                                                                   \
    MultiByteToWideChar(CP_UTF8, 0, psrv->servername, -1, servername, 100);                       \
    MultiByteToWideChar(CP_UTF8, 0, psrv->protocol, -1, protocol, 20);                            \
    MultiByteToWideChar(CP_UTF8, 0, psrv->networkaddr, -1, networkaddr, MAX_PATH);                \
    MultiByteToWideChar(CP_UTF8, 0, psrv->user, -1, user, 20);                                    \
    MultiByteToWideChar(CP_UTF8, 0, psrv->pwd, -1, pwd, 32);                                      \
    MultiByteToWideChar(CP_UTF8, 0, psrv->passphrase, -1, passphrase, 32);                        \
    MultiByteToWideChar(CP_UTF8, 0, psrv->key_path, -1, key_path, MAX_PATH);                      \
    Edit_SetText(GetDlgItem(hdlg, IDC_FILESERVER_NAME_EDIT), servername);                         \
    Edit_SetText(GetDlgItem(hdlg, IDC_COMMPROTOCOL_COMBOBOX), protocol);                          \
    Edit_SetText(GetDlgItem(hdlg, IDC_NETWORK_ADDRESS_EDIT), networkaddr);                        \
    Edit_SetText(GetDlgItem(hdlg, IDC_LOGIN_USER_EDIT), user);                                    \
    Edit_SetText(GetDlgItem(hdlg, IDC_LOGIN_PASS_EDIT), pwd);                                     \
    Edit_SetText(GetDlgItem(hdlg, IDC_LOGIN_PASSPHRASE_EDIT), passphrase);                        \
    Edit_SetText(GetDlgItem(hdlg, IDC_LOGIN_PRIVATE_EDIT), key_path);                             \
    _sntprintf(numbuf, _countof(numbuf) - 1, _T("%d"), psrv->port);                               \
    Edit_SetText(GetDlgItem(hdlg, IDC_NETWORK_PORT_EDIT), numbuf);                                \
    ComboBox_SetCurSel(box_access, psrv->accesss);                                                \
}                                                                                                 \
while(0)

CURL* WINAPI
on_remote_init_socket(const char *url, remotefs *pserver)
{
    bool oneway_certification = false;
    CURL *curl = NULL;
    if (!url)
    {
        return NULL;
    }
    if (!pserver)
    {
        return NULL;
    }
    if (!util_availed_char(pserver->user[0]))
    {
        return NULL;
    }
    if (!(curl = eu_curl_easy_init()))
    {
        printf("eu_curl_easy_init failed!\n");
        return NULL;
    }
    eu_curl_easy_setopt(curl, CURLOPT_URL, url);
    oneway_certification = pserver->key_path[0] && eu_exist_path(pserver->key_path);
    if(!oneway_certification)
    {   // 使用账号, 密码登录
        char userpwd[MAX_PATH + 1] = {0};
        snprintf(userpwd, MAX_PATH, "%s:%s", pserver->user, pserver->pwd);
        eu_curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
        eu_curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    }
    else
    {   // 客户端证书私钥，用于双向认证
        printf("we user pkey login\n");
        eu_curl_easy_setopt(curl, CURLOPT_USERNAME, pserver->user);
        eu_curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, pserver->key_path);
        if (strlen(pserver->passphrase) > 0)
        {
            eu_curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pserver->passphrase);
        }
    }
    return curl;
}

static int
on_remote_save_config(remotefs* pserver)
{
    char sql[MAX_BUFFER] = {0};
    unsigned char enc_pass[33] = {0};
    unsigned char base64_pass[66] = {0};
    if (util_availed_char(pserver->pwd[0]))
    {
        util_aes_enc((unsigned char *) (pserver->pwd), enc_pass, 32);
        on_edit_ssl_enc_base64(base64_pass, enc_pass, 32);
    }
    const char *exec = "insert into file_remote(szName, szProtocol, szAddress, szPort, szArea, szUser, szPass, szPrivate, szPassphrase) "
                       "values('%s','%s', '%s', %d, %d, '%s', '%s', '%s', '%s') ON CONFLICT(szName) DO UPDATE SET szProtocol= "
                       "excluded.szProtocol,szAddress=excluded.szAddress,szPort=excluded.szPort,szArea=excluded.szArea,"
                       "szUser=excluded.szUser,szPass=excluded.szPass,szPrivate=excluded.szPrivate,szPassphrase=excluded.szPassphrase;";
    _snprintf(sql, MAX_BUFFER-1, exec, pserver->servername, pserver->protocol, pserver->networkaddr,
              pserver->port, pserver->accesss, pserver->user, base64_pass, pserver->key_path, pserver->passphrase);
    return eu_sqlite3_send(sql, NULL, NULL);
}

static int
on_remote_remove_config(remotefs* pserver)
{
    char sql[MAX_PATH] = {0};
    if (pserver->servername[0] == 0)
    {
        return 1;
    }
    // 从数据库中删除该行
    _snprintf(sql, MAX_PATH-1, "DELETE FROM file_remote WHERE szName='%s';", pserver->servername);
    if (!eu_sqlite3_send(sql, NULL, NULL))
    {
        // 从链表中删除该节点
        struct list_head *pos, *n;
        list_for_each_safe(pos, n, &list_server)
        {
            remotefs *tmp = list_entry(pos, remotefs, node_server);
            if (tmp == pserver)
            {
                list_del_init(pos);
                eu_safe_free(tmp);
            }
        }
    }
    return 0;
}

static unsigned WINAPI
on_remote_server_testing(void * lp)
{
    HWND hdlg = (HWND)lp;
    remotefs *pserver = (remotefs *)calloc(1, sizeof(remotefs));
    if (pserver == NULL)
    {
        return 1;
    }
    GET_DLG_DATA(hdlg, pserver);
    if (pserver->networkaddr[0] && pserver->user[0] && pserver->port > 0)
    {
        CURL *curl = NULL;
        char url[MAX_BUFFER] = {0};
        snprintf(url, MAX_BUFFER - 1, "sftp://%s:%d/", pserver->networkaddr, pserver->port);
        if ((curl = on_remote_init_socket(url, pserver)) == NULL)
        {
            MSG_BOX(IDC_MSG_ATTACH_FAIL3, IDC_MSG_ERROR, MB_OK);
            free(pserver);
            return 1;
        }
    #if defined(APP_DEBUG) && (APP_DEBUG > 0)
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        CURLcode res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        if (res != CURLE_OK)
        {
            if (res == 67)
            {
                if (util_availed_char(pserver->key_path[0]))
                {
                    MSG_BOX(IDC_MSG_ATTACH_FAIL1, IDC_MSG_ERROR, MB_OK);
                }
                else
                {
                    MSG_BOX(IDC_MSG_ATTACH_FAIL2, IDC_MSG_ERROR, MB_OK);
                }
            }
            else
            {
                MSG_BOX(IDC_MSG_ATTACH_FAIL, IDC_MSG_ERROR, MB_OK);
            }
        }
        else
        {
            MSG_BOX(IDC_MSG_ATTACH_SUCCESS, IDC_MSG_TIPS, MB_OK);
        }
    }
    free(pserver);
    return 0;
}

static int
on_remote_server_browser(HWND hdlg)
{
    TCHAR *path = NULL;
    HWND hwnd_edit = GetDlgItem(hdlg, IDC_LOGIN_PRIVATE_EDIT);
    if (!hwnd_edit)
    {
        return 1;
    }
    if (!(path = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH+1)))
    {
        return 1;
    }
    if (on_file_open_filename_dlg(hdlg, path, MAX_PATH))
    {
        free(path);
        return 1;
    }
    if (*path)
    {
        Edit_SetText(hwnd_edit, path);
    }
    free(path);
    return 0;
}

static LRESULT CALLBACK
remotefs_combox_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sub_id, DWORD_PTR dwRefData)
{
    switch(msg)
    {
        case WM_PAINT:
        {
            uintptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if(!(style & CBS_DROPDOWNLIST))
            {
                break;
            }
            RECT rc;
            GetClientRect(hwnd, &rc);
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH brush = CreateSolidBrush(on_dark_enable() ? rgb_dark_btn_color : GetSysColor(COLOR_BTNFACE));
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            HGDIOBJ oldbrush = SelectObject(hdc, brush);
            HGDIOBJ oldpen = SelectObject(hdc, pen);
            SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
            set_bk_color(hdc, on_dark_enable());
            set_text_color(hdc, on_dark_enable());
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            if(GetFocus() == hwnd)
            {
                RECT temp = rc;
                InflateRect(&temp, -2, -2);
                DrawFocusRect(hdc, &temp);
            }
            int index = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
            if(index >= 0)
            {
                int buflen = (int)SendMessage(hwnd, CB_GETLBTEXTLEN, index, 0);
                TCHAR *buf = (TCHAR *)calloc(sizeof(TCHAR), buflen + 1);
                SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
                rc.left += 5;
                DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL|DT_LEFT|DT_VCENTER|DT_SINGLELINE);
                free(buf);
            }
            SelectObject(hdc, oldpen);
            SelectObject(hdc, oldbrush);
            DeleteObject(brush);
            DeleteObject(pen);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwnd, remotefs_combox_proc, sub_id);
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static INT_PTR CALLBACK
remotefs_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int serverindex;
    int servercount;
    remotefs *pserver = NULL;
    static HWND server_box = NULL;
    static HWND box_access = NULL;
    int nret = 0;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND box_protocol = NULL;
            LOAD_I18N_RESSTR(IDC_MSG_DIR_HOME, m_home);
            LOAD_I18N_RESSTR(IDC_MSG_DIR_ROOT, m_root);
            server_box = GetDlgItem(hdlg, IDC_REMOTE_FILESERVERS_LISTBOX);
            list_for_each_entry(pserver, &list_server, remotefs, node_server)
            {
                TCHAR *serv = *pserver->servername?eu_utf8_utf16(pserver->servername, NULL):NULL;
                if (serv)
                {
                    serverindex = ListBox_AddString(server_box, serv);
                    ListBox_SetItemData(server_box, serverindex, pserver);
                    free(serv);
                }
            }
            box_protocol = GetDlgItem(hdlg, IDC_COMMPROTOCOL_COMBOBOX);
            ComboBox_AddString(box_protocol, _T("SFTP"));
            ComboBox_SetCurSel(box_protocol, 0);
            box_access = GetDlgItem(hdlg, IDC_ACCESS_AREA_COMBOBOX);
            ComboBox_AddString(box_access, m_home);
            ComboBox_AddString(box_access, m_root);
            ComboBox_SelectString(box_access, -1, m_home);
            SetWindowText(GetDlgItem(hdlg, IDC_NETWORK_PORT_EDIT), _T("22"));
            SetWindowSubclass(box_protocol, remotefs_combox_proc, REMOTEFS_PROTOCOL_SUBID, 0);
            SetWindowSubclass(box_access, remotefs_combox_proc, REMOTEFS_ACCESS_SUBID, 0);
            REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_FILE, SW_HIDE);
            REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PASSPHRASE_EDIT, SW_HIDE);
            REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PRIVATE_EDIT, SW_HIDE);
            REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY, SW_HIDE);
            REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY_BUTTON, SW_HIDE);
            if (on_dark_enable())
            {
                const int buttons[] = {IDC_REMOVE_SERVER_BUTTON,
                                       IDC_TEST_SERVER_BUTTON,
                                       IDC_ADD_SERVER_BUTTON,
                                       IDOK,
                                       IDM_APPLY_NOW,
                                       IDC_PRIVATE_KEY_BUTTON};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(GetDlgItem(hdlg, IDC_USE_PRIVATE), L"", L"");
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return util_creater_window(hdlg, eu_module_hwnd());
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDC_REMOVE_SERVER_BUTTON,
                                       IDC_TEST_SERVER_BUTTON,
                                       IDC_ADD_SERVER_BUTTON,
                                       IDOK,
                                       IDM_APPLY_NOW,
                                       IDC_USE_PRIVATE,
                                       IDC_PRIVATE_KEY_BUTTON};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hdlg);
            }
            break;
        }
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_COMMAND:
        {
            uint16_t lid = LOWORD(wParam);
            switch (lid)
            {
                case IDC_REMOTE_FILESERVERS_LISTBOX:
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        serverindex = ListBox_GetCurSel(server_box);
                        if (serverindex >= 0)
                        {
                            pserver = (remotefs *) ListBox_GetItemData(server_box,serverindex);
                            if(!pserver)
                            {
                                break;
                            }
                            SET_DLG_DATA(hdlg, pserver);
                        }
                    }
                    break;
                case IDC_USE_PRIVATE:
                {
                    HWND hc = NULL;
                    bool use_key = IsDlgButtonChecked(hdlg, IDC_USE_PRIVATE);
                    if (use_key)
                    {  // 清除密码以及显示必要的控件
                        hc = GetDlgItem(hdlg, IDC_LOGIN_PASS_EDIT);
                        SetWindowText(hc, NULL);
                        REMOTEFS_HANDLE(hdlg, IDC_REMOTE_PASS, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PASS_EDIT, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_FILE, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PASSPHRASE_EDIT, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PRIVATE_EDIT, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY_BUTTON, SW_SHOW);
                    }
                    else
                    {  // 清除密钥文件
                        hc = GetDlgItem(hdlg, IDC_LOGIN_PRIVATE_EDIT);
                        SetWindowText(hc, NULL);
                        REMOTEFS_HANDLE(hdlg, IDC_REMOTE_PASS, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PASS_EDIT, SW_SHOW);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_FILE, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PASSPHRASE_EDIT, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_LOGIN_PRIVATE_EDIT, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY, SW_HIDE);
                        REMOTEFS_HANDLE(hdlg, IDC_PRIVATE_KEY_BUTTON, SW_HIDE);
                    }
                    break;
                }
                case IDC_TEST_SERVER_BUTTON:
                {
                    CloseHandle((HANDLE)_beginthreadex(NULL,0,&on_remote_server_testing, (void *)hdlg, 0, NULL));
                    return 1;
                }
                case IDC_ADD_SERVER_BUTTON:
                    SetWindowText(GetDlgItem(hdlg, IDC_FILESERVER_NAME_EDIT), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_COMMPROTOCOL_COMBOBOX), _T("SFTP"));
                    SetWindowText(GetDlgItem(hdlg, IDC_NETWORK_ADDRESS_EDIT), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_NETWORK_PORT_EDIT), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_LOGIN_USER_EDIT), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_LOGIN_PASS_EDIT), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_PRIVATE_KEY), _T(""));
                    SetWindowText(GetDlgItem(hdlg, IDC_LOGIN_PRIVATE_EDIT), _T(""));
                    serverindex = ListBox_GetCurSel(server_box);
                    ListBox_SetCurSel(server_box, -1);
                    SetFocus(GetDlgItem(hdlg, IDC_FILESERVER_NAME_EDIT));
                    break;
                case IDC_PRIVATE_KEY_BUTTON:
                {
                    on_remote_server_browser(hdlg);
                    return 1;
                }
                case IDC_REMOVE_SERVER_BUTTON:
                    serverindex = ListBox_GetCurSel(server_box);
                    if (serverindex >= 0)
                    {
                        HTREEITEM htiRoot;
                        tree_data *tvd = NULL;
                        pserver = (remotefs *) ListBox_GetItemData(server_box,serverindex);
                        if (!pserver)
                        {
                            break;
                        }
                        htiRoot = TreeView_GetRoot(g_filetree);
                        while (htiRoot)
                        {
                            tvd = on_treebar_get_treeview(htiRoot);
                            if (tvd->server && pserver == tvd->server)
                            {
                                eu_safe_free(tvd);
                                TreeView_DeleteItem(g_filetree, htiRoot);
                                break;
                            }
                            htiRoot = TreeView_GetNextSibling(g_filetree, htiRoot);
                        }
                        on_remote_remove_config(pserver);
                        ListBox_DeleteString(server_box, serverindex);
                        servercount = ListBox_GetCount(server_box);
                        if (serverindex == servercount)
                        {
                            serverindex--;
                        }
                        ListBox_SetCurSel(server_box, serverindex);
                        SendMessage(hdlg, WM_COMMAND, MAKELONG(0, LBN_SELCHANGE), 0);
                    }
                    break;
                case IDM_APPLY_NOW:
                    {
                        bool new_server = false;
                        serverindex = ListBox_GetCurSel(server_box);
                        if (serverindex == LB_ERR)
                        {
                            new_server = true;
                            pserver = (remotefs *) calloc(1, sizeof(remotefs));
                        }
                        else
                        {
                            pserver = (remotefs *) ListBox_GetItemData(server_box,serverindex);
                        }
                        if (!pserver)
                        {
                            break;
                        }
                        GET_DLG_DATA(hdlg, pserver);
                        pserver->accesss = ComboBox_GetCurSel(box_access);
                        if (pserver->pwd[0])
                        {
                            pserver->cfg = true;
                        }
                        else
                        {
                            pserver->cfg = false;
                        }
                        // 检查服务器名, ip地址, 用户名
                        if (!(util_availed_char(pserver->servername[0]) && util_availed_char(pserver->networkaddr[0]) && util_availed_char(pserver->user[0])))
                        {
                            if (new_server)
                            {
                                free(pserver);
                            }
                            break;
                        }
                        // 检测密码或私钥
                        if (!(util_availed_char(pserver->key_path[0]) || util_availed_char(pserver->pwd[0])))
                        {
                            if (new_server)
                            {
                                free(pserver);
                            }
                            break;
                        }
                        if (on_remote_save_config(pserver))
                        {
                            break;
                        }
                        // 更新文件管理器上的sftp配置
                        on_treebar_update_addr(pserver);
                        if (new_server)
                        {
                            printf("we add pserver to list\n");
                            list_add_tail(&(pserver->node_server), &list_server);
                            TCHAR *serv = *pserver->servername?eu_utf8_utf16(pserver->servername, NULL):NULL;
                            if (serv)
                            {
                                serverindex = ListBox_AddString(server_box, serv);
                                on_treebar_load_remote(g_filetree , pserver);
                                free(serv);
                            }
                        }
                        SendMessage(hdlg, WM_COMMAND, MAKELONG(0, LBN_SELCHANGE), 0);
                        break;
                    }
                case IDOK:
                case IDCANCEL:
                    EndDialog(hdlg, LOWORD(wParam));
                    return 1;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

static int
on_remote_parser_callback(void *data, int count, char **column, char **names)
{
    if ((int *)data)
    {
        *(int *)data = 0;
    }
    remotefs *pserver = (remotefs *) calloc(1, sizeof(remotefs));
    if (!pserver)
    {
        return 1;
    }
    for (int i = 1; i < count; ++i)
    {
        if (!strcmp(names[i], "szName") && strlen(column[i]) > 0)
        {
            strncpy(pserver->servername, column[i], 100);
        }
        else if (!strcmp(names[i], "szProtocol") && strlen(column[i]) > 1)
        {
            strncpy(pserver->protocol, column[i], 20);
        }
        else if (!strcmp(names[i], "szAddress") && strlen(column[i]) > 1)
        {
            strncpy(pserver->networkaddr, column[i], MAX_PATH);
        }
        else if (!strcmp(names[i], "szPort") && strlen(column[i]) > 0)
        {
            pserver->port = atoi(column[i]);
        }
        else if (!strcmp(names[i], "szArea") && strlen(column[i]) > 0)
        {
            pserver->accesss = atoi(column[i]);
        }
        else if (!strcmp(names[i], "szUser") && strlen(column[i]) > 0)
        {
            strncpy(pserver->user, column[i], 20);
        }
        else if (!strcmp(names[i], "szPass") && strlen(column[i]) > 1)
        {
            unsigned char pwd_base64[33] = {0};
            on_edit_ssl_dec_base64(pwd_base64, (unsigned char *)column[i], (int) strlen(column[i]));
            util_aes_dec(pwd_base64, (unsigned char *) (pserver->pwd), 32);
        }
        else if (!strcmp(names[i], "szPrivate") && strlen(column[i]) > 1)
        {
            strncpy(pserver->key_path, column[i], MAX_PATH);
        }
        else if (!strcmp(names[i], "szPassphrase") && strlen(column[i]) > 0)
        {
            strncpy(pserver->passphrase, column[i], 32);
        }
    }
    list_add_tail(&(pserver->node_server), &list_server);
    return 0;
}

remotefs *
on_remote_list_find(const TCHAR *url)
{
    char *purl = NULL;
    remotefs *tmp = NULL;
    char addr[MAX_PATH+1] = {0};
    char port[6+1] = {0};
    struct list_head *pos, *n;
    if (!(purl = eu_utf16_utf8(url, NULL)))
    {
        return NULL;
    }
    if (sscanf(purl,"%*[^0-9] %[^:] %*[^0-9]%[^/]", addr, port) != 2)
    {
        if (sscanf(purl,"%*[^:]://%[^:]:%[1-9]", addr, port) != 2)
        {
            free(purl);
            return NULL;
        }
    }
	printf("addr = %s, port = %s\n", addr, port);
    if (list_empty(&list_server))
    {
        on_sql_post("SELECT * FROM file_remote;", on_remote_parser_callback, NULL);
    }
    list_for_each_safe(pos, n, &list_server)
    {
        tmp = list_entry(pos, remotefs, node_server);
        if (tmp)
        {
            if (STRCMP(addr, ==, tmp->networkaddr) && tmp->port == atoi(port))
            {
                free(purl);
                return tmp;
            }
        }
    }
    free(purl);
    return NULL;
}

void
eu_remote_list_release(void)
{
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &list_server)
    {
        remotefs *tmp = list_entry(pos, remotefs, node_server);
        if (tmp)
        {
            list_del_init(pos);
            free(tmp);
        }
    }
}

unsigned __stdcall
on_remote_load_config(void *lp)
{
    int load_code = 1;
    const char *sql = "SELECT * FROM file_remote;";
    int err = eu_sqlite3_send(sql, on_remote_parser_callback, &load_code);
    if (err == SQLITE_OK && load_code == 0)
    {
        if (on_treebar_variable_initialized(&g_filetree))
        {
            SendMessage(g_filetree, TVI_LOADREMOTE, 0, 0);
        }
    }
    return err;
}

void
on_remote_manager(void)
{
    i18n_dlgbox(eu_module_hwnd(), IDD_REMOTE_FILESERVERS_DIALOG, remotefs_proc, 0);
}
