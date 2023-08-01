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

#define EU_SESSION_INTERVAL 400
#define SESSION_ENVENT_NAME _T("__session_backup_thread__")

static volatile long g_session_id = 0;
static volatile long g_session_total = 0;
static volatile intptr_t g_session_sem = 0;

static void
on_session_backup(const int status)
{
    switch (status)
    {
        case SESSION_THEME:
            eu_save_theme();
            break;
        case SESSION_CONFIG:
            eu_save_config();
            break;
        case SESSION_BOTH:
            eu_save_theme();
            eu_save_config();
            break;
        case SESSION_ALL:
            on_file_auto_backup();
            break;
        default:
            break;
    }
}

static inline void
on_session_set_signal(void)
{
    _InterlockedExchange(&g_session_total, 0);
    if (g_session_sem)
    {
        SetEvent((HANDLE)g_session_sem);
    }
}

static void
on_session_delete_backup(void)
{
    EU_VERIFY(g_tabpages != NULL);
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        eu_tabpage *pnode = on_tabpage_get_ptr(index);
        if (pnode && eu_exist_file(pnode->bakpath))
        {
            wchar_t buf[MAX_BUFFER] = {0};
            TCHAR *p = _tcsrchr(pnode->bakpath, _T('\\'));
            if (p++)
            {
                int len = eu_int_cast(_tcslen(p));
                if (util_isxdigit_string(p, len - 2))
                {
                    if (p[len - 1] == _T('~') && p[len - 2] == _T('~'))
                    {
                        _sntprintf(buf, MAX_BUFFER, _T("%s"), pnode->bakpath);
                        len = eu_int_cast(_tcslen(buf));
                        buf[len - 2] = 0;
                    }
                    else
                    {
                        _sntprintf(buf, MAX_BUFFER, _T("%s~~"), pnode->bakpath);
                    }
                }
            }
            if (!DeleteFile(pnode->bakpath))
            {
                eu_logmsg("%s: delete(pnode->bakpath) error, cause: %lu\n", __FUNCTION__, GetLastError());
            }
            if (eu_exist_file(buf) && !DeleteFile(buf))
            {
                eu_logmsg("%s: delete(backup~~) error, cause: %lu\n", __FUNCTION__, GetLastError());
            }
        }
    }
}

static unsigned __stdcall
on_session_thead(void *lp)
{
    MSG msg;
    while (eu_get_config()->m_session && eu_get_config()->m_up_notify > 0)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                eu_logmsg("%s: recv WM_QUIT\n", __FUNCTION__);
                break;
            }
            switch (msg.message)
            {
                case WM_BACKUP_THEME:
                    on_session_backup(SESSION_THEME);
                    on_session_set_signal();
                    break;
                case WM_BACKUP_CONFIG:
                    on_session_backup(SESSION_CONFIG);
                    on_session_set_signal();
                    break;
                case WM_BACKUP_BOTH:
                    on_session_backup(SESSION_BOTH);
                    on_session_set_signal();
                    break;
                case WM_BACKUP_ALL:
                    on_session_backup(SESSION_ALL);
                    on_session_set_signal();
                    break;
                default:
                    break;
            }
        }
        if (g_session_total)
        {  
            const long count = eu_get_config()->m_up_notify * 1000;
            if (g_session_total >= count)
            {   // 间隔时间内, 自动备份
            #if APP_DEBUG
                printf("we start auto backup\n");
            #endif
                on_session_backup(SESSION_ALL);
                _InterlockedExchange(&g_session_total, 0);
            }
            Sleep(EU_SESSION_INTERVAL);
        }
        _InterlockedExchangeAdd(&g_session_total, EU_SESSION_INTERVAL);
    }
    _InterlockedExchange(&g_session_id, 0);
    _InterlockedExchange(&g_session_total, 0);
    if (g_session_sem)
    {
        ResetEvent((HANDLE)g_session_sem);
        CloseHandle((HANDLE)g_session_sem);
        inter_atom_exchange(&g_session_sem, 0);
    }
    if (!eu_get_config()->m_session || eu_get_config()->m_up_notify <= 0)
    {
        on_session_delete_backup();
    }
    eu_logmsg("on_session_thead exit\n");
    return 0;
}

static void
on_session_thread_msg(const uint32_t msg)
{
    if (g_session_id)
    {
        PostThreadMessage(g_session_id, msg, 0, 0);
        if (g_session_sem)
        {
            WaitForSingleObject((HANDLE)g_session_sem, EU_SESSION_INTERVAL * 10);
        }
    }
}

void
on_session_thread_wait(void)
{
    if (g_session_id)
    {
        PostThreadMessage(g_session_id, WM_QUIT, 0, 0);
        Sleep(EU_SESSION_INTERVAL);
    }
}

void
on_session_do(const HWND hwnd)
{
    if (hwnd && hwnd == share_envent_get_hwnd() && eu_get_config()->m_up_notify > 0)
    {
        if (!_InterlockedCompareExchange(&g_session_id, 1, 0))
        {
            unsigned id = 0;
            HANDLE handle = ((HANDLE)_beginthreadex(NULL, 0, on_session_thead, NULL, 0, (unsigned *)&id));
            if (handle)
            {
                _InterlockedExchange(&g_session_id, (long)id);
                CloseHandle(handle);
                if (!g_session_sem)
                {
                    g_session_sem = (intptr_t)CreateEvent(NULL, FALSE, FALSE, SESSION_ENVENT_NAME);
                }
            }
        }
    }
}

void
eu_session_backup(const int status)
{
    HWND hwnd = eu_hwnd_self();
    HWND share = share_envent_get_hwnd();
    if (hwnd)
    {
        switch (status)
        {
            case SESSION_THEME:
            {
                if (g_session_id)
                {
                    on_session_thread_msg(WM_BACKUP_THEME);
                }
                else if (hwnd == share)
                {
                    on_session_backup(SESSION_THEME);
                }
                break;
            }
            case SESSION_CONFIG:
            {
                if (g_session_id)
                {
                    on_session_thread_msg(WM_BACKUP_CONFIG);
                }
                else if (hwnd == share)
                {
                    on_session_backup(SESSION_CONFIG);
                }
                break;
            }
            case SESSION_BOTH:
            {
                if (g_session_id)
                {
                    on_session_thread_msg(WM_BACKUP_BOTH);
                }
                else if (hwnd == share)
                {
                    on_session_backup(SESSION_BOTH);
                }
                break;
            }
            case SESSION_ALL:
            {
                if (g_session_id)
                {
                    on_session_thread_msg(WM_BACKUP_ALL);
                }
                else if (hwnd == share)
                {
                    on_session_backup(SESSION_ALL);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

unsigned long
on_session_thread_id(void)
{
    return (unsigned long)g_session_id;
}
