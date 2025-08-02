/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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

#define UPDATE_EXE L"upcheck.exe"
#define UPDATE_URL "https://api.github.com/repos/adonais/skylark/releases"

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
                eu_logmsg("Upcheck: package[%s]\n", pdata);
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
        // 默认使用http/2时, 检测不到最新发布的tag ?
        eu_curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        eu_curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:140.0) Gecko/20100101 Firefox/140.0");
        eu_curl_ssl_setting(curl);
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
    WCHAR uri[MAX_PATH] = {0};
    WCHAR path[MAX_BUFFER] = {0};
    WCHAR wcmd[LARGER_LEN] = {0};
    _snwprintf(path, MAX_BUFFER, L"%s\\cache", eu_config_path);
    _snwprintf(wcmd, LARGER_LEN - 1, L"\"%s\\plugins\\%s\" -uri \"%s\" -e \"%s\" -k %lu -hwnd %Id -dt %I64d", eu_module_path,
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
on_update_kill(HANDLE handle)
{
    TerminateProcess(handle, -1);
    on_update_msg(IDS_CHECK_VER_UNKOWN, true);
    eu_logmsg("Upcheck: process force quit...\n");
}

static void
on_update_loop(TASK_T hv)
{
    if (hv)
    {
        HANDLE handle = (HANDLE)hv->pdata;
        on_update_msg(VERSION_UPDATE_PROGRESS, true);
        _InterlockedExchange(&hv->xcode, 1);
        while (true)
        {
            if (WaitForSingleObject(eu_threadpool_handle(), 0) != WAIT_TIMEOUT)
            {
                on_update_kill(handle);
                break;
            }
            if (_InterlockedCompareExchange(&hv->cancel, 0, 1))
            {
                on_update_kill(handle);
                eu_logmsg("Upcheck: recv cancel message, thread %lu exit ...\n", GetCurrentThreadId());
                break;
            }
            if (WaitForSingleObject(handle, MAYBE200MS) != WAIT_TIMEOUT)
            {
                //进程运行完成并且退出了
                int unsigned long result = 255;
                if (!GetExitCodeProcess(handle, &result))
                {
                    eu_logmsg("Upcheck: GetExitCodeProcess failed\n");
                    break;
                }
                eu_logmsg("Upcheck: result == %lu\n", result);
                switch((int)result)
                {
                    case 0:
                    {
                        char sql[MAX_PATH] = {0};
                        on_update_msg(VERSION_UPDATE_COMPLETED, true);
                        _snprintf(sql, MAX_PATH - 1, "UPDATE skylar_ver SET szExtra = %d WHERE szName = 'skylark.exe';", eu_get_config()->upgrade.flags);
                        eu_sqlite3_send(sql, NULL, NULL);
                        break;
                    }
                    case 1:
                    {
                        on_update_msg(VERSION_LATEST, true);
                        break;
                    }
                    default:
                    {
                        on_update_msg(VERSION_UPDATE_UNKOWN, true);
                        break;
                    }
                }
                break;
            }
        }
        if (handle)
        {
            CloseHandle(handle);
        }
        _InterlockedExchange(&hv->xcode, 0);
    }
}

static bool
on_update_diff_days(void)
{
    const uint64_t diff = 3600*24;
    uint64_t cur_time = (uint64_t)time(NULL);
    uint64_t last_time = eu_get_config()->upgrade.last_check;
    if (cur_time - last_time > diff)
    {
        return true;
    }
    return false;
}

static void __stdcall
on_update_send_request(PTP_CALLBACK_INSTANCE inst, PVOID lp, PTP_WAIT wait, TP_WAIT_RESULT result)
{
    int64_t dst_tag = 0;
    int64_t src_tag = 0;
    HWND hwnd = NULL;
    char chunk[QW_SIZE] = {0};
    CURL *curl = NULL;
    struct curl_slist *headers = NULL;
    TASK_T hv = (TASK_T)lp;
    do
    {
        if (!hv)
        {
            break;
        }
        if (eu_get_config()->upgrade.flags == VERSION_UPDATE_COMPLETED)
        {
            break;
        }
        if (hv->attach == UPCHECK_INDENT_MAIN && !on_update_diff_days())
        {
            eu_logmsg("Upcheck: it's not time yet\n");
            break;
        }
        if ((curl = on_update_init(&headers)))
        {
            eu_logmsg("Upcheck: thread start ...\n");
            eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
            CURLcode res = eu_curl_easy_perform(curl);
            eu_curl_easy_cleanup(curl);
            if (res != CURLE_OK && res != CURLE_WRITE_ERROR)
            {
                eu_logmsg("Upcheck: curl failed, code[%d]\n", res);
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
            dst_tag = _atoi64(chunk);
        }
        if ((src_tag = on_update_build_time()) > 0 && src_tag < dst_tag)
        {
            eu_logmsg("Upcheck: curerent_version = %I64d, dst_tag = %I64d\n", src_tag, dst_tag);
            on_update_msg(VERSION_UPDATE_REQUIRED, true);
            if (eu_get_config()->upgrade.enable)
            {
                hv->pdata = (intptr_t)on_update_download(src_tag);
                on_update_loop(hv);
            }
        }
        else if (dst_tag > 0)
        {
            if (hv->attach == UPCHECK_INDENT_ABOUT)
            {
                on_update_msg(VERSION_LATEST, true);
            }
            else if (hv->attach == UPCHECK_INDENT_MAIN)
            {
                PostMessage(eu_hwnd_self(), WM_UPCHECK_LAST, -1, 0);
            }
        }
        else
        {
            on_update_msg(VERSION_UPCHECK_ERR, true);
        }
    } while(0);
}

bool
on_update_excute(void)
{
    int arg_c = 0;
    bool ret = false;
    WCHAR self[MAX_PATH] = {0};
    WCHAR **ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (GetModuleFileNameW(NULL, self, MAX_PATH - 1) > 0 && ptr_arg != NULL)
    {
        HANDLE handle = NULL;
        WCHAR wcmd[LARGER_LEN] = {0};
        WCHAR conf_path[MAX_PATH] = {0};
        WCHAR *param = arg_c > 1 ? (WCHAR *)calloc(sizeof(WCHAR), MAX_PATH * (arg_c - 1)) : NULL;
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
        char *pver = eu_utf16_utf8(__EU_INFO_RELEASE_VERSION, NULL);
        eu_get_config()->upgrade.flags = VERSION_LATEST;
        eu_get_config()->upgrade.last_check = (uint64_t)time(NULL);
        _snprintf(sql, MAX_PATH - 1, "UPDATE skylar_ver SET szVersion = '%s', szBUildId = %I64u, szExtra = %d WHERE szName = 'skylark.exe';", 
                  pver, on_about_build_id(), VERSION_LATEST);
        on_sql_post(sql, NULL, NULL);
        free(pver);
    }
}

void
on_update_run(const int indent)
{
    HWND hwnd = eu_hwnd_self();
    if (hwnd == share_envent_get_hwnd())
    {
        if (!_tcsnicmp(eu_config_path, eu_module_path, _tcslen(eu_module_path)) && util_try_path(eu_module_path))
        {
            if ((eu_get_config()->upgrade.enable || indent == UPCHECK_INDENT_ABOUT) && !on_update_check())
            {
                TASK_ARG hv = {indent};
                eu_threadpool_add(on_update_send_request, &hv);
            }
        }
        else
        {
            eu_get_config()->upgrade.enable = false;
        }
    }
    else
    {
        SendMessage(hwnd, WM_UPCHECK_STATUS, -1, 0);
    }
}

void
on_update_cancel(void)
{
    eu_threadpool_cancel(&on_update_send_request, 0);
}

bool
on_update_check(void)
{
    return eu_threadpool_check(&on_update_send_request, 0);
}
