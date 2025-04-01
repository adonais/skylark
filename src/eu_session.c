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
                eu_logmsg("Session: %s delete(bakpath) error, cause: %lu\n", __FUNCTION__, GetLastError());
            }
            if (eu_exist_file(buf) && !DeleteFile(buf))
            {
                eu_logmsg("Session: %s delete(backup~~) error, cause: %lu\n", __FUNCTION__, GetLastError());
            }
        }
    }
}

static void __stdcall
on_session_thead(PTP_CALLBACK_INSTANCE inst, PVOID pv, PTP_TIMER tm)
{
    TASK_T hv = (TASK_T)pv;
    const long count = eu_get_config()->m_up_notify * 1000;
    if (hv)
    {
        volatile long session_total = 0;
        hv->pthid = GetCurrentThreadId();
        _InterlockedExchange(&hv->xcode, 1);
        while (!_InterlockedCompareExchange(&hv->cancel, 0, 1))
        {
            if (session_total)
            {
                if (session_total >= count)
                {
                    on_session_backup(SESSION_ALL);
                    _InterlockedExchange(&session_total, 0);
                }
                Sleep(EU_SESSION_INTERVAL);
            }
            _InterlockedExchangeAdd(&session_total, EU_SESSION_INTERVAL);
        }
        eu_logmsg("Session: %s exit\n", __FUNCTION__);
        _InterlockedExchange(&hv->xcode, 0);
    }
}

bool
on_session_check(void)
{
    return eu_timer_check(&on_session_thead, 0);
}

void
on_session_cancel(void)
{
    if (eu_timer_cancel(&on_session_thead, 0))
    {
        on_session_delete_backup();
    }
}

void
on_session_run(const int indent)
{
    const int mt_max = INT_MAX/1000;
    int mt = eu_get_config() ? eu_get_config()->m_up_notify : 0;
    // 在主窗口运行, 所以它们要么都为0, 要么相等
    if (eu_hwnd_self() == share_envent_get_hwnd() && mt > 4 && mt < mt_max)
    {
        TASK_ARG hv = {eu_get_config()->m_up_notify * 1000};
        if (indent == UPCHECK_INDENT_MAIN)
        {
            if (!eu_timer_create(&on_session_thead, &hv))
            {
                eu_logmsg("Session: %s, eu_timer_create failed\n", __FUNCTION__);
            }
        }
        else if (indent == UPCHECK_INDENT_ABOUT)
        {
            eu_timer_cancel(&on_session_thead, 0);
            if (!eu_timer_create(&on_session_thead, &hv))
            {
                eu_logmsg("Session: %s, eu_timer_create failed\n", __FUNCTION__);
            }
        }
    }
}

void
eu_session_backup(const int status)
{
    if (eu_hwnd_self() == share_envent_get_hwnd())
    {
        switch (status)
        {
            case SESSION_THEME:
            {
                on_session_backup(SESSION_THEME);
                break;
            }
            case SESSION_CONFIG:
            {
                on_session_backup(SESSION_CONFIG);
                break;
            }
            case SESSION_BOTH:
            {
                on_session_backup(SESSION_BOTH);
                break;
            }
            case SESSION_ALL:
            {
                on_session_backup(SESSION_ALL);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

uint32_t
on_session_thread_id(void)
{
    return eu_timer_id(&on_session_thead, 0);
}
