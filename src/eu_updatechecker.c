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

#define MAYBE200MS 200
#define UPDATE_EXE L"upcheck.exe"
#define UPDATE_URL "https://api.github.com/repos/adonais/skylark/releases"

static volatile long g_upcheck_id = 0;
static volatile sptr_t g_upcheck_handle = 0;

static size_t
on_update_read_json(void *buffer, size_t size, size_t nmemb, void *stream)
{
    size_t len = size * nmemb;
    char *pdata = (char *)stream;
    const char *split = "\"tag_name\": ";
    if (pdata)
    {
        char *p = strstr((char *)buffer, "\"tag_name\": ");
        if (p)
        {
            p += strlen(split);
            if (strlen(p) > 0)
            {
                ++p;
            }
            char *terminators = strchr(p, ',');
            if (terminators && terminators - p < QW_SIZE)
            {
                snprintf(pdata, terminators - p, "%s", p);
                // 已找到, 返回0, 引发CURLE_WRITE_ERROR中断
                return 0;
            }
        }
    }
    return len;
}

static CURL*
on_update_init(struct curl_slist **pheaders)
{
    CURL *curl = NULL;
    if (pheaders && (curl = eu_curl_easy_init()) != NULL)
    {
        *pheaders = eu_curl_slist_append(*pheaders, "Accept: application/json");
        *pheaders = eu_curl_slist_append(*pheaders, "Content-Type: application/json");
        *pheaders = eu_curl_slist_append(*pheaders, "charsets: utf-8");
        eu_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *pheaders);
        eu_curl_easy_setopt(curl, CURLOPT_URL, UPDATE_URL);
        // 但使用http/2时, 检测不到最新发布的tag
        eu_curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        eu_curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:99.0) Gecko/20100101 Firefox/99.0");
        eu_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        eu_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        eu_curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_update_read_json);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6L);
        eu_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    #if APP_DEBUG
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif
    }
    return curl;
}

static int64_t
on_update_build_time(void)
{
    struct tm *p;
    char chunk[QW_SIZE] = {0};
    time_t t = on_about_build_id();
    t += 3600;
    // UTC时间, 编译时间 + 1小时, 消除云端编译与上传的间隔
    p = gmtime(&t);
    _snprintf(chunk, QW_SIZE - 1, "%d%02d%02d%02d%02d%02d", (1900+p->tm_year), (1+p->tm_mon),p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    return _atoi64(chunk);
}

static HANDLE
on_update_download(const int64_t dtag)
{
    WCHAR uri[MAX_SIZE] = {0};
    WCHAR path[MAX_BUFFER] = {0};
    WCHAR wcmd[LARGER_LEN] = {0};
    _snwprintf(path, MAX_BUFFER, L"%s\\cache", eu_config_path);
    _snwprintf(wcmd, LARGER_LEN - 1, L"\"%s\\plugins\\%s\" -uri \"%s\" -e \"%s\" -k %u -hwnd %Id -dt %I64d", eu_module_path,
               UPDATE_EXE, util_make_u16(eu_get_config()->upgrade.url, uri, MAX_SIZE - 1), path, GetCurrentProcessId(), (intptr_t)eu_module_hwnd(), dtag);
    return eu_new_process(wcmd, NULL, NULL, 0, NULL);
}

static void
on_update_msg(UPDATE_STATUS status, bool msg)
{
    switch (status)
    {
        case VERSION_LATEST:
            eu_get_config()->upgrade.flags = VERSION_LATEST;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_DEC, 0);
            }
            break;
        case VERSION_UPCHECK_ERR:
            eu_get_config()->upgrade.flags = VERSION_UPCHECK_ERR;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_ERR, 0);
            }
            break;
        case VERSION_UPDATE_REQUIRED:
            eu_get_config()->upgrade.flags = VERSION_UPDATE_REQUIRED;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_NEW, 0);
            }
            break;
        case VERSION_UPDATE_PROGRESS:
            eu_get_config()->upgrade.flags = VERSION_UPDATE_PROGRESS;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_PROGRESS, 0);
            }
            break;
        case VERSION_UPDATE_BREAK:
            eu_get_config()->upgrade.flags = VERSION_UPDATE_BREAK;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_BREAK, 0);
            }
            break;
        case VERSION_UPDATE_UNKOWN:
            eu_get_config()->upgrade.flags = VERSION_UPDATE_UNKOWN;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_UNKOWN, 0);
            }
            break;
        case VERSION_UPDATE_COMPLETED:
            eu_get_config()->upgrade.flags = VERSION_UPDATE_COMPLETED;
            if (msg)
            {
                PostMessage(on_about_hwnd(), WM_ABOUT_STC, IDS_CHECK_VER_COMPLETED, 0);
            }
            break;
        default:
            eu_get_config()->upgrade.flags = VERSION_LATEST;
            break;
    }
}

static void
on_update_loop(HANDLE handle)
{
    if (handle)
    {
        MSG msg;
        on_update_msg(VERSION_UPDATE_PROGRESS, true);
        while (true)
        {
            switch ((int)WaitForSingleObject(handle, MAYBE200MS))
            {   
                case WAIT_OBJECT_0:
                {
                    //进程运行完成并且退出了
                    uint32_t result = 256;
                    if (!GetExitCodeProcess(handle, &result))
                    {
                        eu_logmsg("%s: GetExitCodeProcess failed\n", __FUNCTION__);
                        break;
                    }
                    eu_logmsg("%s: result == %u\n", __FUNCTION__, result);
                    if (result == 0)
                    {
                        char sql[MAX_PATH] = {0};
                        on_update_msg(VERSION_UPDATE_COMPLETED, true);
                        _snprintf(sql, MAX_PATH - 1, "UPDATE skylar_ver SET szExtra = %d WHERE szName = 'skylark.exe';", eu_get_config()->upgrade.flags);
                        eu_sqlite3_send(sql, NULL, NULL);
                    }
                    else if (result == (uint32_t)-1)
                    {
                        on_update_msg(VERSION_UPDATE_BREAK, true);
                    }
                    else if (result > 0)
                    {
                        on_update_msg(VERSION_UPDATE_UNKOWN, true);
                    }
                    break;
                }
                case WAIT_TIMEOUT:
                {
                    // 有消息需要处理
                    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
                    if (msg.message == WM_QUIT)
                    {
                        TerminateProcess(handle, -1);
                        on_update_msg(IDS_CHECK_VER_UNKOWN, true);
                        eu_logmsg("process force quit...\n");
                        break;
                    }
                    continue; 
                }
                default:
                {
                    break;
                }
            }
            break;
        }
    }
}

static bool
on_update_diff_days(void)
{
    bool res = false;
    const uint64_t diff = 3600*24;
    uint64_t cur_time = (uint64_t)time(NULL);
    uint64_t last_time = eu_get_config()->upgrade.last_check;
    if (cur_time - last_time > diff)
    {
        res = true;
    }
    return res;
}

static unsigned __stdcall
on_update_send_request(void *lp)
{
    int64_t tag = 0;
    int64_t dtag = 0;
    HWND hwnd = NULL;
    char chunk[QW_SIZE] = {0};
    CURL *curl = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    struct curl_slist *headers = NULL;
    const int ident = (const int)(intptr_t)(lp);
    do
    {
        if (eu_get_config()->upgrade.flags == VERSION_UPDATE_COMPLETED)
        {
            break;
        }
        if (ident == 1 && !on_update_diff_days())
        {
            eu_logmsg("It's not time yet\n");
            break;
        }
        if ((curl = on_update_init(&headers)))
        {
            eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
            res = eu_curl_easy_perform(curl);
            eu_curl_easy_cleanup(curl);
            if (res != CURLE_OK)
            {
                eu_logmsg("curl failed, cause:%d\n", res);
            }
        }
        else
        {   // 等待dialog窗口
            Sleep(800);
        }
        if (headers)
        {
            eu_curl_slist_free_all(headers);
        }
        if (strlen(chunk) > 0)
        {
            tag = _atoi64(chunk);
        }
        if ((dtag = on_update_build_time()) > 0 && dtag < tag)
        {
            eu_logmsg("curerent_version = %I64d, tag = %I64d\n", dtag, tag);
            on_update_msg(VERSION_UPDATE_REQUIRED, true);
            if (!eu_get_config()->upgrade.enable)
            {
                break;
            }
            on_proc_counter_stop();
            on_update_loop(on_update_download(dtag));
        }
        else if (tag > 0)
        {
            on_update_msg(VERSION_LATEST, true);
            if (ident == UPCHECK_INDENT_MAIN)
            {
                PostMessage(eu_hwnd_self(), WM_UPCHECK_LAST, -1, 0);
            }
        }
        else
        {
            on_update_msg(VERSION_UPCHECK_ERR, true);
        }
    } while(0);
    _InterlockedExchange(&g_upcheck_id, 0);
    return res;
}

static void
on_update_close_handle(void)
{
    if (g_upcheck_handle)
    {
        CloseHandle((HANDLE)g_upcheck_handle);
        inter_atom_exchange(&g_upcheck_handle, 0);
    }
}

bool
on_update_do(void)
{
    int arg_c = 0;
    bool ret = false;
    WCHAR wcmd[LARGER_LEN] = {0};
    WCHAR self[MAX_PATH] = {0};
    WCHAR conf_path[MAX_PATH] = {0};
    WCHAR *param = NULL;
    WCHAR **ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (ptr_arg && GetModuleFileNameW(NULL, self, MAX_PATH - 1) > 0)
    {
        HANDLE handle = NULL;
        param = arg_c > 1 ? (WCHAR *)calloc(sizeof(WCHAR), MAX_PATH * (arg_c - 1)) : NULL;
        if (param)
        {   // 把参数追加到命令行
            for (int i = 1; i < arg_c; ++i)
            {
                if (wcslen(ptr_arg[i]) > MAX_PATH - 1)
                {   // 参数过长, 忽略
                    continue;
                }
                wcscat(param, ptr_arg[i]);
                if (i < arg_c - 1)
                {
                    wcscat(param, _T(" "));
                }
            }
            if (wcsstr(param, _T("-restart")))
            {   // 忽略此参数
                *param = 0;
            }
        }
        _snwprintf(conf_path, MAX_PATH - 1, L"%s\\cache", eu_config_path);
        _snwprintf(wcmd, LARGER_LEN - 1, L"\"%s\\plugins\\%s\" -k %lu -e \"%s\" -s \"%s\" -u 1 -param \"%s\"",
                   eu_module_path, UPDATE_EXE, GetCurrentProcessId(), conf_path, self, param ? param : _T(""));
        LocalFree(ptr_arg);
        eu_safe_free(param);
        if ((handle = eu_new_process(wcmd, NULL, NULL, 0, NULL)))
        {
            CloseHandle(handle);
            ret = true;
        }
    }
    return ret;
}

void
on_update_sql(void)
{
    if (eu_get_config())
    {
        char sql[MAX_PATH] = {0};
        eu_get_config()->upgrade.flags = VERSION_LATEST;
        eu_get_config()->upgrade.last_check = (uint64_t)time(NULL);
        _snprintf(sql, MAX_PATH - 1, "UPDATE skylar_ver SET szExtra = %d WHERE szName = 'skylark.exe';", VERSION_LATEST);
        on_sql_post(sql, NULL, NULL);
    }
}

void
on_update_thread_wait(void)
{
    if (g_upcheck_id)
    {
        PostThreadMessage(g_upcheck_id, WM_QUIT, 0, 0);
        if (g_upcheck_handle)
        {
            WaitForSingleObject((HANDLE)g_upcheck_handle, INFINITE);
        }
    }
    on_update_close_handle();
}

void
on_update_check(const int ident)
{
    HWND hwnd = eu_hwnd_self();
    if (hwnd == share_envent_get_hwnd())
    {
        if (!_InterlockedCompareExchange(&g_upcheck_id, 1, 0))
        {
            HANDLE handle = NULL;
            on_update_close_handle();
            if ((handle = ((HANDLE)_beginthreadex(NULL, 0, on_update_send_request, (void *)(intptr_t)ident, 0, (unsigned *)&g_upcheck_id))))
            {
                inter_atom_exchange(&g_upcheck_handle, (sptr_t)handle);
            }
            else
            {
                eu_logmsg("%s: on_update_send_request start failed\n", __FUNCTION__);
                _InterlockedExchange(&g_upcheck_id, 0);
            }
        }
    }
    else
    {
        SendMessage(hwnd, WM_UPCHECK_STATUS, -1, 0);
    }
}
