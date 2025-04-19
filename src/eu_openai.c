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

typedef struct
{
    jobject_string stream;
    jobject_string reply;
    CURL **curl;
    uint32_t output;
    uint32_t think;
    HWND hwnd_rc;
    HWND hwnd_sc;
} ai_data;

static void
on_openai_data_free(eu_tabpage *p)
{
    if (p && p->presult)
    {
        for (size_t i = 0; i < cvector_size(p->presult->ai_msg.ai); ++i)
        {
            eu_safe_free(p->presult->ai_msg.ai[i].content);
        }
        cvector_freep(&p->presult->ai_msg.ai);
        _InterlockedExchange(&p->presult->json_id, 0);
    }
}

static CURL*
on_openai_init_socket(eu_tabpage *p, struct curl_slist **pheaders, jobject_string *pds, char *str)
{
    CURL *curl = eu_curl_easy_init();
    if (p && p->presult &&  pheaders && curl != NULL && init_dynamic_string(pds) == 0)
    {
        char auth[FILESIZE] = {0};
        size_t vec_size = 0;
        ai_info info = {str, "user"};
        p->presult->ai_msg.curl = curl;
        cvector_push_back(p->presult->ai_msg.ai, info);
        _snprintf(auth, FILESIZE, "Authorization: Bearer %s", eu_get_config()->openai.key);
        *pheaders = eu_curl_slist_append(*pheaders, "Accept: application/json");
        *pheaders = eu_curl_slist_append(*pheaders, "charsets: utf-8");
        *pheaders = eu_curl_slist_append(*pheaders, "Content-Type: application/json");
        *pheaders = eu_curl_slist_append(*pheaders, auth);
        append_string(pds, "{\"model\": \"");
        append_string(pds, eu_get_config()->openai.model);
        append_string(pds, "\",\"messages\": [");
        for (size_t i = 0; i < (vec_size = cvector_size(p->presult->ai_msg.ai)); ++i)
        {
            if (vec_size == 1)
            {
                append_string(pds, "{\"role\": \"system\", \"content\": \"");
                if (STR_NOT_NUL(eu_get_config()->openai.setting))
                {
                    eu_str_replace(eu_get_config()->openai.setting, 1024, "\n", "\\n");
                    append_string(pds, eu_str_replace(eu_get_config()->openai.setting, 1024, "\"", "\\\""));
                }
                else
                {
                    append_string(pds, "You are a helpful assistant.");
                }
                append_string(pds, "\"},");
            }
            append_string(pds, "{\"role\": \"");
            append_string(pds, p->presult->ai_msg.ai[i].role);
            append_string(pds, "\", \"content\": \"");
            append_string(pds, p->presult->ai_msg.ai[i].content);
            append_string(pds, "\"}");
            if (i < vec_size - 1)
            {
                append_string(pds, ",");
            }
        }
        append_string(pds, "],");
        if (eu_get_config()->openai.max_tokens > 0)
        {
            char tokens[OVEC_LEN] = {0};
            _snprintf(tokens, OVEC_LEN, "%d", eu_get_config()->openai.max_tokens);
            append_string(pds, "\"max_tokens\": ");
            append_string(pds, tokens);
            append_string(pds, ",");
        }
        append_string(pds, "\"stream\": ");
        append_string(pds, eu_get_config()->openai.stream ? "true" : "false");
        append_string(pds, "}");
        eu_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *pheaders);
        eu_curl_easy_setopt(curl, CURLOPT_URL, eu_get_config()->openai.base);
        eu_curl_ssl_setting(curl);
        eu_curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 16L);
        eu_curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pds->data);
    #if APP_DEBUG
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif
    }
    return curl;
}

static void
on_openai_output_error(const HWND hwnd, const int http_code)
{
    switch (http_code)
    {   // https://api-docs.deepseek.com/zh-cn/quick_start/error_codes
        case 400:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_FORMAT_ERR, 0), 0);
            break;
        }
        case 401:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_AUTH_FAILD, 0), 0);
            break;
        }
        case 402:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_INSUFFICIENT_BAL, 0), 0);
            break;
        }
        case 404:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_404, 0), 0);
            break;
        }
        case 422:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_PARAMETE_ERR, 0), 0);
            break;
        }
        case 429:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_TMP_ERR, 0), 0);
            break;
        }
        case 500:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_FAILURE, 0), 0);
            break;
        }
        case 503:
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_BUSY, 0), 0);
            break;
        }
        default:
        {
            break;
        }
    }
}

static bool
on_openai_init_chunk(ai_data **pdata, eu_tabpage *p)
{
    if ((*pdata = (ai_data *)malloc(sizeof(ai_data))) != NULL &&
        (init_dynamic_string(&(*pdata)->stream)) == 0 &&
        (init_dynamic_string(&(*pdata)->reply)) == 0)
    {
        (*pdata)->curl = &(p->presult->ai_msg.curl);
        (*pdata)->hwnd_sc = p->hwnd_sc;
        (*pdata)->hwnd_rc = p->presult->hwnd_sc;
        (*pdata)->output = 0x1;
        (*pdata)->think = eu_get_config()->openai.think ? 0x7 : 0x1;
        return true;
    }
    return false;
}

static void
on_openai_destory_chunk(ai_data **pdata)
{
    if (*pdata)
    {
        destory_dynamic_string(&(*pdata)->stream);
        destory_dynamic_string(&(*pdata)->reply);
        *((*pdata)->curl) = NULL;
        (*pdata)->curl = NULL;
        (*pdata)->hwnd_sc = NULL;
        (*pdata)->hwnd_rc = NULL;
        free(*pdata);
        *pdata = NULL;
    }
}

static void
on_openai_parser_content(const char *str, const HWND hwnd, uint32_t *output)
{
    if (str && hwnd && output)
    {
        if (*output & THINK_INIT)
        {
            *output &= ~THINK_INIT;
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(WM_OPENAI_DATA, 0), (intptr_t)(_strdup(str)));
        }
        else
        {
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(WM_OPENAI_DATA, AI_EOT), (intptr_t)(_strdup(str)));
        }
    }
}

static bool
on_openai_parser_next(const json_value *obj, ai_data *mem)
{
    for (unsigned int z = 0; z < obj->u.object.length; ++z)
    {   // content and reasoning_content
        char *name = obj->u.object.values[z].name;
        json_value *v = obj->u.object.values[z].value;
        if (strcmp(name, "content") == 0)
        {
            if (v->type == json_string && v->u.string.ptr[0] != 0)
            {
                append_string(&mem->reply, v->u.string.ptr);
                on_openai_parser_content(v->u.string.ptr, mem->hwnd_sc, &mem->output);
            }
        }
        else if (strcmp(name, "reasoning_content") == 0)
        {
            if (v->type == json_null)
            {
                if (mem->think & THINK_ENDDO)
                {
                    mem->think &= ~THINK_ENDDO;
                    PostMessage(mem->hwnd_rc, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_THINK, 0), AI_EOT);
                }
            }
            else if (v->type == json_string)
            {
                if (v->u.string.ptr[0] == 0)
                {
                    if (mem->think & THINK_INIT)
                    {
                        mem->think &= ~THINK_INIT;
                        PostMessage(mem->hwnd_rc, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_THINK, 0), 0);
                    }
                }
                else if (mem->think & THINK_OUTPUT)
                {
                    PostMessage(mem->hwnd_rc, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_SERV_THINK, 0), (intptr_t)(_strdup(v->u.string.ptr)));
                }
            }
        }
        return true;
    }
    return false;
}

static bool
on_openai_parser_json(json_value *obj, ai_data *mem)
{
    if (obj && obj->type == json_object && mem)
    {
        unsigned int i, j, k = 0;
        for (i = 0; i < obj->u.object.length; ++i)
        {
            json_value *value = obj->u.object.values[i].value;
            if (value->type == json_array)
            {   // choices array
                for (j = 0; j < value->u.array.length; ++j)
                {
                    json_value *v = value->u.array.values[j];
                    if (v->type == json_object)
                    {   // {} object
                        for (k = 0; k < v->u.object.length; ++k)
                        {
                            json_value *v1 = v->u.object.values[k].value;
                            if (v1->type == json_object)
                            {   // delta or message object
                                return on_openai_parser_next(v1, mem);
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

static size_t
on_openai_reply(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    ai_data *mem = (ai_data *)userp;
    char *start = (char *)contents;
    if (real_size > 0 && mem && mem->hwnd_sc && mem->hwnd_rc)
    {
        char *p1, *p2 = NULL;
        json_value *root = NULL;
        json_settings sets = {0};
        while ((p1 = strchr(start, '\n')) || (p1 = strrchr(start, '}')))
        {
            if (*p1 == '\n')
            {
                *p1 = 0;
            }
            else
            {
                *(p1 + 1) = 0;
            }
            if ((p2 = strchr(start, '{')) != NULL)
            {
                if (strnicmp(p2, "{\"error\":", 9) == 0)
                {   
                    int code = 0;
                    eu_curl_easy_getinfo(*(mem->curl), CURLINFO_RESPONSE_CODE, &code);
                    if (code == 200)
                    {   // 其他错误?
                        PostMessage(mem->hwnd_rc, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_DATA_ERR, 0), 0);
                    }
                    break;
                }
                if (!(root = json_parse_ex(&sets, p2, strlen(p2), NULL)))
                {
                    if (!mem->reply.length)
                    {   // 不标准的json格式?
                        eu_logmsg("Openai: %s, json_parse_ex failed\n", __FUNCTION__);
                    }
                    break;
                }
                if (!on_openai_parser_json(root, mem))
                {
                    PostMessage(mem->hwnd_rc, WM_COMMAND, MAKEWPARAM(IDS_OPENAI_DATA_ERR, 0), 0);
                    eu_logmsg("Openai: %s, on_openai_parser_json failed\n", __FUNCTION__);
                    real_size = 0;
                    break;
                }
            }
            start = p1 + 1;
        }
    }
    return real_size;
}

static char *
on_openai_replace_data(const char *str)
{
    char *v = NULL;
    char *v1 = str ? util_str_replace(str, "\r", "") : NULL;
    char *v2 = v1 ? util_str_replace(v1, "\n", "") : NULL;
    char *v3 = v2 ? util_str_replace(v2, "\\", "\\\\") : NULL;
    char *v4 = v3 ? util_str_replace(v3,  "\"", "\\\"") : NULL;
    if (v4)
    {
        v = util_str_replace(v4, "'", "\\\"");
        free(v4);
    }
    eu_safe_free(v1);
    eu_safe_free(v2);
    eu_safe_free(v3);
    return v;
}

int
on_openai_session(char *str, const HWND hwnd_rc)
{
    CURL *curl = NULL;
    ai_data *chunk = NULL;
    CURLcode res = SKYLARK_ERROR;
    jobject_string ds = {0};
    struct curl_slist *headers = NULL;
    eu_tabpage *p = on_tabpage_from_result(hwnd_rc);
    if (p && p->presult)
    {
        const HWND hwnd_sc = p->hwnd_sc;
        on_sci_call(p->presult, SCI_SETREADONLY, 1, 0);
        if (hwnd_sc && (curl = on_openai_init_socket(p, &headers, &ds, str)) != NULL && on_openai_init_chunk(&chunk, p))
        {
            eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_openai_reply);
            eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
            res = eu_curl_easy_perform(curl);
        }
        if (res == SKYLARK_OK)
        {
            int http_code = 0;
            eu_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            on_openai_output_error(hwnd_rc, http_code);
            if (200 == http_code)
            {
                ai_info info = {on_openai_replace_data(chunk->reply.data), "assistant"};
                cvector_push_back(p->presult->ai_msg.ai, info);
            }
        }
        if (headers)
        {
            eu_curl_slist_free_all(headers);
        }
        if (curl)
        {
            eu_curl_easy_cleanup(curl);
        }
        else
        {
            res = EUE_CURL_INIT_FAIL;
            PostMessage(p->presult->hwnd_sc, WM_COMMAND, MAKEWPARAM(IDC_MSG_ATTACH_FAIL3, 0), 0);
        }
        if (chunk)
        {
            on_openai_destory_chunk(&chunk);
        }
        destory_dynamic_string(&ds);
        Sleep(200);  // 让 PostMessage 先到达
        on_sci_call(p->presult, SCI_SETREADONLY, 0, 0);
    }
    return res;
}

static unsigned WINAPI
on_openai_request(void *lp)
{
    MSG msg;
    const HWND hwnd = (const HWND)lp;
    eu_tabpage *p = on_tabpage_from_result(hwnd);
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            else if (msg.message == WM_OPENAI_TALK && msg.wParam > 0)
            {
                if (on_openai_session((char *)msg.wParam, hwnd) == EUE_CURL_INIT_FAIL)
                {
                    break;
                }
            }
        }
        Sleep(MAYBE200MS);
    }
    on_openai_data_free(p);
    eu_logmsg("Openai: recv wm_quit\n");
    return 0;
}

void
on_openai_run(const char *str, const HWND hwnd)
{
    eu_tabpage *p = on_tabpage_from_result(hwnd);
    if (STR_NOT_NUL(str) && p && p->presult)
    {
        p = p->presult;
        if (p->json_id)
        {
            PostThreadMessage((DWORD)(p->json_id), WM_OPENAI_TALK, (WPARAM)_strdup(str), 0);
        }
        else
        {
            CloseHandle((HANDLE) _beginthreadex(NULL, 0, &on_openai_request, (void *)hwnd, 0, (uint32_t *)(&p->json_id)));
            if (p->json_id)
            {
                Sleep(100);
                PostThreadMessage((DWORD)p->json_id, WM_OPENAI_TALK, (WPARAM)_strdup(str), 0);
            }
        }
    }
}

void
on_openai_cancel(eu_tabpage *presult)
{
    if (presult && presult->json_id)
    {
        if (presult->ai_msg.curl)
        {   // 终止curl_easy_perform
            eu_logmsg("Openai: we set curl timeout 1m\n");
            eu_curl_easy_setopt(presult->ai_msg.curl, CURLOPT_TIMEOUT_MS, 1L);
            Sleep(1000);
        }
        PostThreadMessage((DWORD)presult->json_id, WM_QUIT, 0, 0);
        while (_InterlockedCompareExchange(&presult->json_id, 0, 0) != 0)
        {
            Sleep(100);
        }
    }
}

bool
on_openai_check(eu_tabpage *presult)
{
    return (presult && presult->json_id != 0);
}
