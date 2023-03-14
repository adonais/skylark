/******************************************************************************
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

#define SKYLARK_SEM_NAME _T("__skylark_show__")
#define FILE_CLOSE_EVENT (_T("__g_skylark_close_envent__"))

static HANDLE  g_skylark_sem;         // 主窗口初始化信号量
HMODULE g_skylark_lang = NULL;        // 资源dll句柄


/*****************************************************************************
 * 共享内存API的封装
 ****************************************************************************/
HANDLE
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

HANDLE
share_open(uint32_t dw_access, LPCTSTR name)
{
    return OpenFileMapping(dw_access, false, name);
}

LPVOID
share_map(HANDLE hmap, size_t bytes, uint32_t dw_access)
{
    return MapViewOfFile(hmap, dw_access, 0, 0, bytes);
}

void
share_unmap(LPVOID memory)
{
    if (memory)
    {
        UnmapViewOfFile(memory);
        memory = NULL;
    }
}

void
share_close(HANDLE handle)
{
    if ((intptr_t)handle > 0)
    {
        CloseHandle(handle);
        handle = NULL;
    }
}

/*****************************************************************************
 * 内存映射文件, 可分块映射
 ****************************************************************************/
bool
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

uint8_t*
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

/*****************************************************************************
 * 主窗口建立的信号量, 复位方式为手动恢复, 初始无信号
 ****************************************************************************/
bool
share_envent_create(void)
{
    return (g_skylark_sem = CreateEvent(NULL, TRUE, FALSE, SKYLARK_SEM_NAME)) != NULL;
}

void
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

uint32_t
share_envent_wait(uint32_t milliseconds)
{
    return WaitForSingleObject(g_skylark_sem, milliseconds);
}

void
share_envent_close(void)
{
    share_close(g_skylark_sem);
}

void
share_envent_release(void)
{
    if (g_skylark_sem)
    {
        ResetEvent(g_skylark_sem);
    }
    share_envent_close();
}

HWND
share_envent_get_hwnd(void)
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

void
share_envent_set_hwnd(HWND hwnd)
{
    HANDLE hmap = share_open(FILE_MAP_WRITE | FILE_MAP_READ, SKYLARK_LOCK_NAME);
    if (hmap)
    {
        HWND *pmemory = (HWND *) share_map(hmap, sizeof(HWND), FILE_MAP_WRITE | FILE_MAP_READ);
        if (pmemory)
        {
            memcpy(pmemory, &hwnd, sizeof(HWND));
            share_unmap(pmemory);
        }
        share_close(hmap);
    }
}

/*****************************************************************************
 * 文件自动保存时设置的信号量, 复位方式为自动恢复, 初始状态为有信号
 ****************************************************************************/
bool
share_envent_create_file_sem(HANDLE *phandle)
{
    return (*phandle = CreateEvent(NULL, FALSE, TRUE, FILE_CLOSE_EVENT)) != NULL;
}

HANDLE
share_envent_open_file_sem(void)
{
    return OpenEvent(EVENT_ALL_ACCESS, false, FILE_CLOSE_EVENT);
}

void
share_envent_wait_file_close_sem(HANDLE *phandle)
{
    WaitForSingleObject(*phandle, INFINITE);
    share_close(*phandle);
}

/*****************************************************************************
 * 多进程共享,  以消息模式放送而不是共享内存
 ****************************************************************************/
unsigned
share_send_msg(void *param)
{
    HWND hwnd = eu_module_hwnd();
    if (!hwnd)
    {
        // 等待主窗口初始化
        share_envent_wait(8000);
    }
    if ((hwnd = eu_module_hwnd()) != NULL)
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
 * 多国语言文件路径存储区, 多进程共享
 ****************************************************************************/
HANDLE
share_load_lang(void)
{
    TCHAR *u16_lang = NULL;
    HANDLE lang_map = NULL;
    TCHAR lang_path[MAX_PATH] = {0};
    if (!eu_module_path[0])
    {
        eu_process_path();
    }
    uint32_t cid = (uint32_t) GetSystemDefaultLCID();
    if (!eu_get_config() || !strcmp(eu_get_config()->m_language, "auto"))
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
        u16_lang = eu_get_config() ? eu_utf8_utf16(eu_get_config()->m_language, NULL) : NULL;
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
    else if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        printf("lang_map_lock exist\n");
        return lang_map;
    }
    LPVOID pmodule = share_map(lang_map, sizeof(lang_path), FILE_MAP_WRITE | FILE_MAP_READ);
    if (pmodule)
    {
        memcpy(pmodule, lang_path, sizeof(lang_path));
        share_unmap(pmodule);
    }
    return lang_map;
}

void
share_spinlock_wait(volatile intptr_t *plock)
{
    uint64_t spin_count = 0;
    while (inter_atom_compare_exchange(plock, 0, 0) != 0)
    {
        if (spin_count < 32)
        {
            Sleep(0);
        }
        else
        {
            Sleep(1);
        }
        ++spin_count;
    }
}

void
share_close_lang(void)
{
    eu_close_dll(g_skylark_lang);
}
