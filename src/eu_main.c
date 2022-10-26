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

#define REG_ON_DARK_MODE                                    \
{                                                           \
    if (_tcschr(argv[1], _T('=')))                          \
    {                                                       \
        dark_mode = eu_on_dark_init(true, true);            \
    }                                                       \
}

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
        if (eu_get_config()->m_fullscreen)
        {
            printf("we create fullsrceen window\n");
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
    bool no_remote = false;
    MSG msg = {0};
    HWND hwnd = NULL;
    HANDLE mapped = NULL;
    HANDLE lang_map = NULL;
    TCHAR cache_path[MAX_PATH + 1] = {0};
    HANDLE hsem = NULL;
    HINSTANCE instance = eu_module_handle();
    if (argc > 1)
    {
        if (_tcscmp(argv[1], _T("-restart")) == 0)
        {
            eu_wait_process(_wtoi(argv[2]));
        }
        else if (_tcscmp(argv[1], _T("-noremote")) == 0)
        {
            no_remote = true;
        }
    }  // 获取主进程所在目录
    if (!eu_process_path(eu_module_path, MAX_PATH))
    {
        return -1;
    }  // 便携目录是否可写入
    if (_sntprintf(cache_path, MAX_PATH, _T("%s\\conf\\cache"), eu_module_path) > 0)
    {
        if (!(cache_path[0] && eu_try_path(cache_path)))
        {
            MSG_BOX(IDC_MSG_DIR_WRITE_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
    }  // 设置lua脚本搜索路径
    if (!eu_lua_path_setting())
    {
        return -1;
    }  // 加载主配置文件
    if (!eu_load_main_config())
    {
        return -1;
    }
    if (!no_remote && (hsem = share_envent_open_file_sem()) != NULL)
    {   // 编辑器还未完全关闭
        share_close(hsem);
        MSG_BOX(IDS_USER32_UNFINISHED, IDC_MSG_TIPS, MB_OK);
        return 0;
    }
    if (!share_envent_create())
    {   // 进程同步的信号量
        return -1;
    }
    // 建立共享内存, 里面保存第一个进程的主窗口句柄
    if ((mapped = share_create(NULL, PAGE_READWRITE, sizeof(HWND), SKYLARK_LOCK_NAME)) == NULL)
    {
        return -1;
    }
    else if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        muti = true;
    }
    if (muti)
    {
        bool dark_mode = false;
        if (argc > 1)
        {
            file_backup bak = {0};
            if (!_tcsncmp(argv[1], REGFILE, _tcslen(REGFILE)))
            {   // 注册或卸载文件右键菜单
                REG_ON_DARK_MODE
                eu_undo_file_popup();
                _tcsncpy(bak.rel_path, argv[1], MAX_PATH - 1);
                share_send_msg(&bak);
            }
            else if (!_tcsncmp(argv[1], REGFOLDER, _tcslen(REGFOLDER)))
            {   // 注册或卸载目录右键菜单
                REG_ON_DARK_MODE
                eu_undo_dir_popup();
                _tcsncpy(bak.rel_path, argv[1], MAX_PATH - 1);
                share_send_msg(&bak);
            }
            else if (!_tcsncmp(argv[1], REGASSOC, _tcslen(REGASSOC)))
            {   // 注册文件关联
                REG_ON_DARK_MODE
                eu_create_registry_dlg();
            }
            else if (!no_remote && eu_config_parser_path(argv, argc, bak.rel_path))
            {   // 多个文件时, 向第一个主窗口发送WM_COPYDATA消息
                eu_postion_setup(argv, argc, &bak);
                share_send_msg(&bak);
            }
            share_close_lang();
        }
        else
        {   // 没有参数, 则恢复窗口
            share_send_msg(NULL);
        }
        if (!no_remote)
        {
            share_close(mapped);
            share_envent_close();
            if (dark_mode)
            {
                eu_on_dark_release(true);
            }
            return 0;
        }
    }
    if (argc > 1 && _tcscmp(argv[1], _T("-lua")) == 0)
    {
        bool cinit = false;
        const TCHAR *fname = NULL;
        const TCHAR *save = NULL;
        if (argc > 2)
        {
            fname = argv[2];
        }
        if (AllocConsole())
        {
            freopen("conin$","r",stdin);
            freopen("conout$","w", stdout);
            freopen("conout$","w", stderr);
            cinit = true;
        }
        if (cinit && argc > 4 && fname && _tcscmp(fname, _T("-b")) == 0)
        {
            fname = argv[3];
            save = argv[4];
            _tputenv(_T("LUA_PATH="));
            fprintf(stderr, "End-of-Conversion: \n");
        }
        if (cinit)
        {
            msg.wParam = eu_lua_script_convert(fname, save);
            system("pause");
            FreeConsole();
        }
        goto all_clean;
    }
    if (!on_hook_exception())
    {
        printf("on_hook_exception failed\n");
        msg.wParam = -1;
        goto all_clean;
    }
    else
    {
        on_hook_do();
        eu_init_logs();
    }
    if (!eu_load_config())
    {   // 加载分类配置文件
        msg.wParam = -1;
        goto all_clean;
    }
    if (!(lang_map = share_load_lang()))
    {   // 加载语言资源文件
        msg.wParam = -1;
        goto all_clean;
    }
    if (eu_check_arg(argv, argc, _T("--help")))
    {
        if (strcmp(eu_get_config()->window_theme, "black") == 0)
        {
            eu_on_dark_init(true, true);
        }
        eu_about_command();
        msg.wParam = 0;
        goto all_clean;
    }
    if (argc > 1 && _tcscmp(argv[1], REGFILE) == 0)
    {
        eu_reg_file_popup_menu();
        msg.wParam = 0;
        goto all_clean;
    }
    if (argc > 1 && _tcscmp(argv[1], REGFOLDER) == 0)
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
    if (mapped)
    {
        if (!muti)
        {
            LPVOID phandle = share_map(mapped, sizeof(HWND), FILE_MAP_WRITE | FILE_MAP_READ);
            if (phandle)
            {
                memcpy(phandle, &hwnd, sizeof(HWND));
                share_unmap(phandle);
            }
        }  // 主窗口初始化完成, 可以发送消息了
        share_envent_set(true);
        eu_get_config()->m_instance = no_remote ? true : eu_get_config()->m_instance;
        eu_load_file();
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
        if ((!eu_get_search_hwnd() || !IsDialogMessage(eu_get_search_hwnd(), &msg)) &&
            (!eu_result_hwnd() || !IsDialogMessage(eu_result_hwnd(), &msg)) &&
            (!eu_snippet_hwnd() || !IsDialogMessage(eu_snippet_hwnd(), &msg)))
        {
            if (eu_before_proc(&msg) > 0)
            {
                continue;
            }
            if (!TranslateAccelerator(hwnd, eu_get_accel()->haccel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    eu_save_theme();
    eu_save_config();
all_clean:
    eu_free_theme();
    eu_free_accel();
    share_close(mapped);
    share_close(lang_map);
    share_close_lang();
    share_envent_release();
    eu_curl_global_release();
    eu_sci_release();
    on_hook_undo();
    eu_remote_list_release();
    eu_on_dark_release(true);
    eu_doc_ptr_free();
    eu_font_release();
    eu_close_db_handle();
    printf("all clean\n");
    return (int) msg.wParam;
}
