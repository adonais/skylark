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
#include <process.h>

/*****************************************************************************
 * 主窗口初始化之前, 先建立工具栏, 状态栏, 搜索框等顶层窗口
 ****************************************************************************/
static HWND
init_instance(HINSTANCE instance)
{
    HWND hwnd = eu_create_main_window(instance);
    if (hwnd)
    {
        if (!eu_create_toolbar(hwnd))
        {
            return NULL;
        }
        if (!eu_create_statusbar(hwnd))
        {
            return NULL;
        }
        if (!eu_create_search_dlg())
        {
            return NULL;
        }
        if (eu_get_config()->m_fullscreen)
        {
            eu_create_fullscreen(hwnd);
        }
        else if (strlen(eu_get_config()->m_placement) < 1)
        {
            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        }
        else
        {
            eu_restore_placement(hwnd);
            ShowWindow(hwnd, SW_SHOW);
        }
    }
    return hwnd;
}

/*****************************************************************************
 * 等待进程退出, 用于编辑器重启时等待父进程
 ****************************************************************************/
static void
eu_wait_process(DWORD pid)
{
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    if (handle)
    {
        WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);
    }
}

/*****************************************************************************
 * 应用程序入口点
 ****************************************************************************/
int
_tmain(int argc, TCHAR *argv[])
{
    bool muti = false;
    MSG msg = {0};
    HMODULE pux = NULL;   // 如果使用经典风格, 它是uxtheme的句柄
    HWND hwnd = NULL;
    HANDLE h_mapped = NULL;
    HANDLE lang_map = NULL;
    TCHAR sc_path[MAX_PATH + 1] = { 0 };
    HACCEL htable = NULL;
    HANDLE hsem = NULL;
    HINSTANCE instance = eu_module_handle();
    if (argc > 1 && _tcscmp(argv[1], _T("-restart")) == 0)
    {
        eu_wait_process(_wtoi(argv[2]));
    }
    if ((hsem = share_envent_open_file_sem()) != NULL)
    {   // 编辑器还未完全关闭
        share_close(hsem);
        MSG_BOX(IDS_USER32_UNFINISHED, IDC_MSG_TIPS, MB_OK);
        return 0;
    }
    if (!eu_process_path(eu_module_path, MAX_PATH))
    {
        return -1;
    }
    if (!share_envent_create())
    {   // 进程同步的信号量
        return -1;
    }
    SetLastError(0);   // 建立共享内存, 里面保存第一个进程的主窗口句柄
    h_mapped = share_create(NULL, PAGE_READWRITE, sizeof(HWND), SKYLARK_LOCK_NAME);
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        muti = true;
    }
    else if (!h_mapped)
    {
        return -1;
    }
    if (muti)
    {
        bool dark_mode = false;
        if (argc > 1)
        {
            file_backup bak = {0};
            if (_tcsncmp(argv[1], _T("-reg1"), 5) == 0)
            {   // 注册或卸载右键菜单1
                if (_tcschr(argv[1], _T('=')))
                {
                    dark_mode = eu_on_dark_init(true, true);
                }
                eu_undo_file_popup();
                _tcscpy(bak.rel_path, argv[1]);
                share_send_msg(&bak);
            }
            else if (_tcsncmp(argv[1], _T("-reg2"), 5) == 0)
            {   // 注册或卸载右键菜单2
                if (_tcschr(argv[1], _T('=')))
                {
                    dark_mode = eu_on_dark_init(true, true);
                }
                eu_undo_dir_popup();
                _tcscpy(bak.rel_path, argv[1]);
                share_send_msg(&bak);
            }
            else if (_tcsncmp(argv[1], _T("-reg3"), 5) == 0)
            {   // 注册文件关联 
                if (_tcschr(argv[1], _T('=')))
                {
                    dark_mode = eu_on_dark_init(true, true);
                }
                eu_create_registry_dlg();
            }
            else if (_tcslen(argv[1]) > 1)
            {   // 多个文件时, 向第一个主窗口发送WM_COPYDATA消息
                _tcscpy(bak.rel_path, argv[1]);
                share_send_msg(&bak);
            }
            share_close_lang();
        }
        else
        {   // 没有参数, 则恢复窗口
            share_send_msg(NULL);
        }
        share_close(h_mapped);
        share_envent_close();
        if (dark_mode)
        {
            eu_on_dark_release(true);
        }
        return 0;
    }
    if (argc > 1 && _tcscmp(argv[1], _T("-lua")) == 0)
    {
        const TCHAR *fname = NULL;
        const TCHAR *save = NULL;
        if (argc > 2)
        {
            fname = argv[2];
        }
        if (argc > 4 && fname && _tcscmp(fname, _T("-b")) == 0)
        {
            fname = argv[3];
            save = argv[4];
        }
        return eu_lua_script_convert(fname, save);
    }
    if (!on_hook_exception())
    {
        printf("on_hook_exception failed\n");
        msg.wParam = -1;
        goto all_clean;
    }
    if (true)
    {
        TCHAR lua_path[ENV_LEN + 1];
        TCHAR cache_path[MAX_PATH] = {0};
        _sntprintf(cache_path, MAX_PATH, _T("%s\\conf\\cache"), eu_module_path);
        if (!(cache_path[0] && eu_try_path(cache_path)))
        {
            MSG_BOX(IDC_MSG_DIR_WRITE_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        } // 设置lua脚本搜索路径
        _sntprintf(lua_path, ENV_LEN, _T("LUA_PATH=%s\\conf\\conf.d\\?.lua;%s\\conf\\scripts\\?.lua"), eu_module_path, eu_module_path);
        if (_tputenv(lua_path) != 0)
        {
            msg.wParam = -1;
            goto all_clean;
        }
        on_hook_do();
        eu_init_logs();
    }   // 加载主配置文件
    if (_sntprintf(sc_path, MAX_PATH, _T("%s\\conf\\conf.d\\eu_main.lua"), eu_module_path) > 0)
    {
        if (eu_lua_script_exec(sc_path) != 0)
        {
            msg.wParam = -1;
            goto all_clean;
        }
    }   // 加载分类配置文件
    if (eu_load_config(&pux) != 0)
    {
        msg.wParam = -1;
        goto all_clean;
    }
    if (!(lang_map = share_load_lang()))
    {
        msg.wParam = -1;
        printf("loadex_lang_config failed\n");
        goto all_clean;
    }
    if (argc > 1 && _tcscmp(argv[1], _T("-reg1")) == 0)
    {
    	eu_reg_file_popup_menu();
    	msg.wParam = 0;
    	goto all_clean;
    }
    if (argc > 1 && _tcscmp(argv[1], _T("-reg2")) == 0)
    {
    	eu_reg_dir_popup_menu();
    	msg.wParam = 0;
    	goto all_clean;
    }    
    // 注册scintilla
    if (!eu_sci_register(instance))
    {
        MSG_BOX(IDC_MSG_SCI_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        msg.wParam = -1;
        goto all_clean;
    }
    if (!(hwnd = init_instance(instance)))
    {
        msg.wParam = -1;
        goto all_clean;
    }
    if ((htable = LoadAccelerators(instance, MAKEINTRESOURCE(IDC_SKYLARK))) == NULL)
    {
        msg.wParam = -1;
        goto all_clean;
    }
    if (h_mapped)
    {
        LPVOID phandle = share_map(h_mapped, sizeof(HWND), FILE_MAP_WRITE | FILE_MAP_READ);
        if (phandle)
        {
            memcpy(phandle, &hwnd, sizeof(HWND));
            share_unmap(phandle);
            // 主窗口初始化完成, 可以发送消息了
            share_envent_set(true);
            eu_load_file();
        }
    }   
    if (strcmp(eu_get_config()->window_theme, "black") == 0)
    {
        if (eu_on_dark_init(true, true))
        {
            SendMessageTimeout(HWND_BROADCAST, WM_THEMECHANGED, 0, 0, SMTO_NORMAL, 10, 0);
        }
    }
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(eu_get_search_hwnd(), &msg))
        {
            if (eu_before_proc(&msg) > 0)
            {
                continue;
            }
            if (!TranslateAccelerator(hwnd, htable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    if (true)
    {
        eu_save_theme();
        eu_free_theme();
        eu_save_config();
    }
all_clean:
    share_close(h_mapped);
    share_close(lang_map);
    share_close_lang();
    safe_close_dll(pux);
    share_envent_release();
    eu_curl_global_release();
    eu_sci_release();
    on_hook_undo();
    eu_remote_list_release();
    eu_on_dark_release(true);
    eu_doc_config_release();
    eu_font_release();
    printf("all clean\n");
    return (int) msg.wParam;
}
