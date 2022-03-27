/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

#define SKYLARK_SEM_NAME _T("__skylark_show__")
#define FILE_CLOSE_EVENT (_T("__g_skylark_close_envent__"))

static HANDLE  g_skylark_sem;         // 主窗口初始化信号量
HMODULE g_skylark_lang = NULL;        // 资源dll句柄

bool WINAPI
share_open_file(LPCTSTR path, bool read_only, uint32_t dw_creation, HANDLE *phandle)
{
    *phandle = CreateFile(path, read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE), 
                          (FILE_SHARE_READ|FILE_SHARE_WRITE), NULL, dw_creation, FILE_ATTRIBUTE_NORMAL, NULL);
     if (*phandle == INVALID_HANDLE_VALUE)
     {
        return false;
     }
     return true;
}

HANDLE WINAPI
share_create(HANDLE handle, uint32_t dw_protect, size_t size, LPCTSTR name)
{
    DWORD hi = 0, lo = 0;
    if (size > 0)
    {
        lo = (uint32_t) (size & 0xffffffff);
        hi = (uint32_t) (((uint64_t) size >> 32) & 0xffffffff);  
    }
    if (!handle)
    {
        return CreateFileMapping(INVALID_HANDLE_VALUE, NULL, dw_protect, hi, lo, name);
    }
    return CreateFileMapping(handle, NULL, dw_protect, hi, lo, name);
}

HANDLE WINAPI
share_open(uint32_t dw_access, LPCTSTR name)
{
    return OpenFileMapping(dw_access, false, name);
}

LPVOID WINAPI
share_map(HANDLE hmap, size_t bytes, uint32_t dw_access)
{
    return MapViewOfFile(hmap, dw_access, 0, 0, bytes);
}

bool WINAPI
share_unmap(LPVOID memory) 
{
    if (memory == NULL)
    {
        return false;
    }
    return UnmapViewOfFile(memory);
}

void WINAPI
share_close(HANDLE handle)
{
    if ((uintptr_t)handle > 0)
    {
        CloseHandle(handle);
        handle = NULL;
    }
}

uint8_t *WINAPI
share_map_section(HANDLE mapping, uint64_t offset, size_t length, bool read_only)
{
    int      adjust;
    uint64_t adj_offset;
    size_t   adj_length;
    SYSTEM_INFO si = {0};
    if (!mapping)
    {
        return NULL;
    }
    GetSystemInfo( &si );
    adjust = offset % si.dwAllocationGranularity;
    adj_offset = offset - adjust;
    adj_length = length + adjust;
    return (uint8_t *)MapViewOfFile(mapping, 
                                    read_only?FILE_MAP_READ:FILE_MAP_WRITE,
                                    (DWORD)(adj_offset >> 32),
                                    (DWORD)(adj_offset & 0xffffffff),
                                    adj_length);
}

bool WINAPI
share_envent_create(void)
{
    return (g_skylark_sem = CreateEvent(NULL, TRUE, FALSE, SKYLARK_SEM_NAME)) != NULL;
}

void WINAPI
share_envent_set(bool signaled)
{
    if (signaled)
    {
        SetEvent(g_skylark_sem);
    }
    else
    {
        ResetEvent(g_skylark_sem);
    }
}

uint32_t WINAPI
share_envent_wait(uint32_t milliseconds)
{
    return WaitForSingleObject(g_skylark_sem, milliseconds);
}

void WINAPI
share_envent_close(void)
{
    share_close(g_skylark_sem);
}

void WINAPI
share_envent_release(void)
{
    if (g_skylark_sem)
    {
        ResetEvent(g_skylark_sem);
    }
    share_envent_close();
}

bool WINAPI
share_envent_create_close_sem(HANDLE *phandle)
{
    return (*phandle = CreateEvent(NULL, FALSE, TRUE, FILE_CLOSE_EVENT)) != NULL;
}

HANDLE WINAPI
share_envent_open_file_sem(void)
{
    return OpenEvent(EVENT_ALL_ACCESS, false, FILE_CLOSE_EVENT);
}

HWND WINAPI
share_get_hwnd(void)
{
    HWND hwnd = NULL;
    HANDLE hmap = share_open(FILE_MAP_READ, SKYLARK_LOCK_NAME);
    if (hmap)
    {
        HWND *pmemory = (HWND *) share_map(hmap, sizeof(HWND), FILE_MAP_READ);
        if (pmemory)
        {
            hwnd = *pmemory;
            share_unmap(pmemory);
        }
        share_close(hmap);
    }
    return hwnd;
}

/*****************************************************************************
 * 多进程共享,  以消息模式放送而不是共享内存
 ****************************************************************************/
unsigned WINAPI
share_send_msg(void *param)
{
    HWND hwnd = NULL;
    // 等待主窗口初始化
    share_envent_wait(INFINITE);
    if ((hwnd = share_get_hwnd()) != NULL)
    {
        file_backup *pbak = (file_backup *)param;
        if (pbak)
        {
            COPYDATASTRUCT cpd = { 0 };
            cpd.lpData = (PVOID) pbak;
            cpd.cbData = (DWORD) sizeof(file_backup);
            SendMessageW(hwnd, WM_COPYDATA, 0, (LPARAM) &cpd);
        }
        SwitchToThisWindow(hwnd, true);
    }
    return 0;
}

/*****************************************************************************
 * 根据当前区域加载多国语言文件
 ****************************************************************************/
HANDLE WINAPI
share_load_lang(void)
{
    TCHAR *u16_lang = NULL;
    HANDLE lang_map = NULL;
    TCHAR lang_path[MAX_PATH] = {0};
    
    uint32_t cid = (uint32_t) GetSystemDefaultLCID();
    if (!strcmp(eu_get_config()->m_language, "auto"))
    {
        switch (cid)
        {
            case 0x0804: // 简中
            {
                _sntprintf(lang_path, MAX_PATH-1, _T("%s\\locales\\zh-cn.dll"), eu_module_path);
                g_skylark_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE);
                break;
            }
            case 0x0404: // 繁体
            case 0x0411: // 日文
            case 0x0412: // 韩文
            {
                break;
            }
            default:
                _sntprintf(lang_path, MAX_PATH-1, _T("%s\\locales\\en-us.dll"), eu_module_path);
                g_skylark_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE);            
                break;
        }
    }
    else
    {
        u16_lang = eu_utf8_utf16(eu_get_config()->m_language, NULL);
        if (u16_lang)
        {
            _sntprintf(lang_path, MAX_PATH-1, _T("%s\\locales\\%s"), eu_module_path, u16_lang);
            g_skylark_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE);
            free(u16_lang);
        }
    }
    if (!g_skylark_lang)
    {
        _sntprintf(lang_path, MAX_PATH-1, _T("%s\\locales\\en-us.dll"), eu_module_path);
        g_skylark_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!g_skylark_lang)
        {
            printf("can not load lang library\n");
            return NULL;
        }
    }
    if (!(lang_map = share_create(NULL, PAGE_READWRITE, sizeof(lang_path), SKYLARK_LOCK_LANG)))
    {
        printf("share_create error in %s\n", __FUNCTION__);
        return NULL;
    }
    LPVOID pmodule = share_map(lang_map, sizeof(lang_path), FILE_MAP_WRITE | FILE_MAP_READ);
    if (pmodule)
    {
        memcpy(pmodule, lang_path, sizeof(lang_path));
        share_unmap(pmodule);
    }
    return lang_map;
}

void WINAPI
share_close_lang(void)
{
    safe_close_dll(g_skylark_lang);
}