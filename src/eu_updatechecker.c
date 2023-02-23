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
        eu_curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:99.0) Gecko/20100101 Firefox/99.0");
        eu_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        eu_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        eu_curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_update_read_json);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6L);
        eu_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    #if defined(APP_DEBUG) && (APP_DEBUG > 0)
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
    p = localtime(&t);
    _snprintf(chunk, QW_SIZE - 1, "%d%02d%02d%02d%02d%02d", (1900+p->tm_year), (1+p->tm_mon),p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    /* 编译时间 + 1小时 */
    return (_atoi64(chunk) + 3600);
}

static HWND
on_update_get_hwnd(void)
{
    HWND hwnd_child = GetForegroundWindow();
    if (GetParent(hwnd_child) == eu_module_hwnd())
    {
        return hwnd_child;
    }
    return NULL;
}

static unsigned __stdcall
on_update_send_request(void *lp)
{
    int64_t tag = 0;
    HWND hwnd = NULL;
    char chunk[QW_SIZE] = {0};
    CURLcode res = CURLE_FAILED_INIT;
    struct curl_slist *headers = NULL;
    CURL *curl = on_update_init(&headers);
    if (curl)
    {
        eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
        res = eu_curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            printf("curl failed, cause:%d\n", res);
        }
        eu_curl_easy_cleanup(curl);
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
    if ((hwnd = on_update_get_hwnd()))
    {
        if (tag > 0)
        {
            if (tag > on_update_build_time())
            {
                PostMessage(hwnd, WM_ABOUT_STC, IDS_CHECK_VER_NEW, 0);
            }
            else
            {
                PostMessage(hwnd, WM_ABOUT_STC, IDS_CHECK_VER_DEC, 0);
            }
        }
        else
        {
            PostMessage(hwnd, WM_ABOUT_STC, IDS_CHECK_VER_ERR, 0);
        }
    }
    return res;
}

void
on_update_check(void)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_update_send_request, NULL, 0, NULL));
}
