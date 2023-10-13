/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2023 Hua andy <hua.andy@gmail.com>

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
        eu_dark_theme_init(true, true);                     \
    }                                                       \
}

#define EU_REENTRANT_PARAM                                  \
((argc > 1) ? ((_tcscmp(argv[1], _T("--help")) == 0) || (_tcscmp(argv[1], _T("-lua")) == 0)) : (false))

#define SKY_SAFE_EXIT(n) {msg.wParam = n; goto all_clean;}

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
            eu_logmsg("we create fullsrceen window\n");
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

static int
init_lua_runtime(int args, TCHAR **pargv)
{
    int ret = SKYLARK_OK;
    const TCHAR *fname = NULL;
    const TCHAR *save = NULL;
    bool cinit = false;
    bool is_cui = !eu_gui_app();
    if (args > 2)
    {
        fname = pargv[2];
    }
    if (!is_cui)
    {
        cinit = (bool)AllocConsole();
        freopen("conin$","r",stdin);
        freopen("conout$","w", stdout);
        freopen("conout$","w", stderr);
    }
    if ((is_cui || cinit) && fname)
    {
        if (args > 4 && _tcscmp(fname, _T("-b")) == 0)
        {
            fname = pargv[3];
            save = pargv[4];
            _tputenv(_T("LUA_PATH="));
            ret = eu_lua_script_convert(fname, save);
            fprintf(stderr, "End-of-Conversion.\n");
        }
        else
        {
            ret = eu_lua_script_exec(fname);
        }
        system("pause");
    }
    if (cinit)
    {
        FreeConsole();
    }
    return ret;
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
    }
    if (!eu_config_init_path())
    {   // 获取配置文件所在目录
        SKY_SAFE_EXIT(SKYLARK_NOT_OPENED);
    }
    if (!eu_config_load_main())
    {   // 加载主配置文件
        SKY_SAFE_EXIT(SKYLARK_NOT_OPENED);
    }
    if (!eu_init_logs(false))
    {   // 初始化日志文件路径
        SKY_SAFE_EXIT(SKYLARK_NOT_OPENED);
    }
    eu_logmsg("Enable logging, process startup\n");
    if (!no_remote && (hsem = share_envent_open_file_sem()) != NULL)
    {   // 编辑器还未完全关闭
        share_close(hsem);
        MSG_BOX(IDS_USER32_UNFINISHED, IDC_MSG_TIPS, MB_OK);
        return SKYLARK_OK;
    }
    if (!share_envent_create())
    {   // 进程同步的信号量
        eu_logmsg("share_envent_create failed\n");
        SKY_SAFE_EXIT(SKYLARK_ENVENT_FAILED);
    }
    // 建立共享内存, 里面保存第一个进程的主窗口句柄
    if ((mapped = share_create(NULL, PAGE_READWRITE, sizeof(HWND), SKYLARK_LOCK_NAME)) == NULL)
    {
        eu_logmsg("share_create failed\n");
        SKY_SAFE_EXIT(SKYLARK_MEMAP_FAILED);
    }
    else if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        muti = true;
    }
    if (muti)
    {
        if (argc > 1)
        {
            if (!_tcsncmp(argv[1], REGFILE, _tcslen(REGFILE)))
            {   // 注册或卸载文件右键菜单
                REG_ON_DARK_MODE
                eu_reg_file_popup_menu();
            }
            else if (!_tcsncmp(argv[1], REGFOLDER, _tcslen(REGFOLDER)))
            {   // 注册或卸载目录右键菜单
                REG_ON_DARK_MODE
                eu_reg_dir_popup_menu();
            }
            else if (!_tcsncmp(argv[1], REGASSOC, _tcslen(REGASSOC)))
            {   // 注册文件关联
                REG_ON_DARK_MODE
                eu_create_registry_dlg();
            }
            else if (!no_remote)
            {
                cvector_vector_type(file_backup) vpath = NULL;
                if (eu_config_parser_path(argv, argc, &vpath))
                {
                    for (size_t i = 0; i < cvector_size(vpath); ++i)
                    {
                        if (vpath[i].rel_path[0])
                        {   // 多个文件时, 向第一个主窗口发送WM_COPYDATA消息
                            share_send_msg(&vpath[i]);
                        }
                    }
                }
                cvector_free(vpath);
            }
        }
        else
        {   // 没有参数, 则恢复窗口
            share_send_msg(NULL);
        }
        if (no_remote)
        {
            eu_get_config()->m_instance = true;
        }
        else if (!EU_REENTRANT_PARAM)
        {
            SKY_SAFE_EXIT(SKYLARK_OK);
        }
    }
    if (argc > 1 && _tcscmp(argv[1], _T("-lua")) == 0)
    {
        SKY_SAFE_EXIT(init_lua_runtime(argc, argv));
    }
#if 0
    if (!eu_hook_exception())
    {
        eu_logmsg("eu_hook_exception failed\n");
        SKY_SAFE_EXIT(SKYLARK_HOOK_FAILED);
    }
#endif
    if (!eu_config_load_accel())
    {   // 加载快捷键配置文件
        SKY_SAFE_EXIT(SKYLARK_ACCEL_FAILED);
    }
    if (!eu_config_load_docs())
    {   // 加载文档分类配置文件
        SKY_SAFE_EXIT(SKYLARK_DOCS_FAILED);
    }
    if (!(lang_map = share_load_lang()))
    {   // 加载语言资源文件
        SKY_SAFE_EXIT(SKYLARK_RESID_FAILED);
    }
    if (eu_config_check_arg(argv, argc, _T("--help")))
    {
        if (strcmp(eu_get_config()->window_theme, "black") == 0)
        {
            eu_dark_theme_init(true, true);
        }
        eu_about_command();
        SKY_SAFE_EXIT(SKYLARK_OK);
    }
    if (argc > 1 && _tcscmp(argv[1], REGFILE) == 0)
    {
        eu_reg_file_popup_menu();
        SKY_SAFE_EXIT(SKYLARK_OK);
    }
    if (argc > 1 && _tcscmp(argv[1], REGFOLDER) == 0)
    {
        eu_reg_dir_popup_menu();
        SKY_SAFE_EXIT(SKYLARK_OK);
    }
    if (!eu_config_load_toolbar())
    {   // 加载工具栏配置文件
        eu_logmsg("eu_config_load_toolbar failed\n");
        SKY_SAFE_EXIT(SKYLARK_TB_FAILED);
    }
    // 注册scintilla
    if (!eu_sci_register(instance))
    {
        eu_logmsg("eu_sci_register failed\n");
        SKY_SAFE_EXIT(SKYLARK_SCI_FAILED);
    }
    if (!(hwnd = init_instance(instance)))
    {
        eu_logmsg("init_instance failed\n");
        SKY_SAFE_EXIT(SKYLARK_INST_FAILED);
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
        }
        share_envent_set(true);  // 主窗口初始化完成, 可以发送消息了
        if (no_remote)
        {
            eu_get_config()->m_instance = true;
        }
        if (!eu_config_load_files())
        {
            eu_logmsg("eu_config_load_files failed\n");
            SKY_SAFE_EXIT(SKYLARK_CONF_FAILED);
        }
    }
    if (strcmp(eu_get_config()->window_theme, "black") == 0)
    {
        if (eu_dark_theme_init(true, true))
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
    eu_session_backup(SESSION_BOTH);
all_clean:
    share_close(mapped);
    share_close(lang_map);
    share_close_lang();
    share_envent_release();
    eu_sci_release();
    eu_remote_list_release();
    eu_dark_theme_release(true);
    eu_font_release();
    eu_dbase_release();
    eu_logmsg("All cleanup, process exit code [%d]\n", (int) msg.wParam);
    eu_api_release();
    eu_reset_config();
    return (int) msg.wParam;
}
