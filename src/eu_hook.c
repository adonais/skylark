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
#include <dbghelp.h>

#define __vmalloc(l) VirtualAlloc(NULL, l, MEM_COMMIT, PAGE_EXECUTE_READWRITE)
#define __vfree(p) VirtualFree(p, 0, MEM_RELEASE)
typedef void (*func_zrow)(void *, int i);
typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *SetUnhandledExceptionFilterPtr)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
typedef BOOL (WINAPI *MiniDumpWriteDumpPtr)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
                                            CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
static func_zrow put_zrow_bottom_stub;

static bool
pre_exception_filter(void)
{
    bool ret = false;
    HMODULE m_kernel = GetModuleHandle(_T("kernel32.dll"));
    if (!m_kernel)
    {
        return false;
    }
    SetUnhandledExceptionFilterPtr fnSetUnhandledExceptionFilter =
        (SetUnhandledExceptionFilterPtr) GetProcAddress(m_kernel, "SetUnhandledExceptionFilter");
    if (fnSetUnhandledExceptionFilter == NULL)
    {
    #if APP_DEBUG
        printf("GetProcAddress error\n");
    #endif
        return false;
    }
#ifdef _M_IX86
    // Code for x86:
    // 33 C0                xor         eax,eax
    // C2 04 00             ret         4
    unsigned char szExecute[] = { 0x33, 0xC0, 0xC2, 0x04, 0x00 };
#elif _M_X64
    // 33 C0                xor         eax,eax
    // C3                   ret
    unsigned char szExecute[] = { 0x33, 0xC0, 0xC3 };
#else
#error "The following code only works for x86 and x64!"
#endif
    SIZE_T bytes = 0;
    ret = WriteProcessMemory(GetCurrentProcess(), (LPVOID) fnSetUnhandledExceptionFilter, (LPCVOID) szExecute, sizeof(szExecute), &bytes);
    return ret;
}

static BOOL
hookMiniDumpWriteDump(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
                      CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam)
{
    BOOL ret = FALSE;
    HMODULE hdbg = LoadLibraryW(L"dbghelp.dll");
    if (hdbg != NULL)
    {
        MiniDumpWriteDumpPtr fnMiniDumpWriteDump = (MiniDumpWriteDumpPtr)GetProcAddress(hdbg, "MiniDumpWriteDump");
        if (fnMiniDumpWriteDump)
        {
            ret = fnMiniDumpWriteDump(hProcess, ProcessId, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam);
        }
        FreeLibrary(hdbg);
    }
    return ret;
}

static LONG WINAPI
error_handle_output(PEXCEPTION_POINTERS pExceptionInfo)
{
    /* 异常信息结构体 */
    MINIDUMP_EXCEPTION_INFORMATION ExInfo;
    /* dump生成目录 */
    TCHAR appdir[MAX_PATH + 1] = { 0 };
    HANDLE hfile = NULL;
    if (!(GetEnvironmentVariable(_T("APPDATA"), appdir, MAX_PATH) > 0))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    wcsncat(appdir, L"\\skylark.dmp", MAX_PATH);
    /* 创建文件句柄 */
    hfile = CreateFile(appdir, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hfile && ERROR_FILE_NOT_FOUND == GetLastError())
    {
        hfile = CreateFile(appdir, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (INVALID_HANDLE_VALUE == hfile)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    ExInfo.ThreadId = GetCurrentThreadId();
    ExInfo.ExceptionPointers = pExceptionInfo;
    ExInfo.ClientPointers = FALSE;
    /* MiniDumpWriteDump输出dump */
    hookMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hfile, MiniDumpNormal, &ExInfo, NULL, NULL);
    CloseHandle(hfile);
    return EXCEPTION_EXECUTE_HANDLER;
}

bool
eu_hook_exception(void)
{
#if defined(_M_IX86) || defined(_M_X64)
    if (!util_under_wine())
    {
        SetUnhandledExceptionFilter(error_handle_output);
        return pre_exception_filter();
    }
#endif
    return true;
}

/**************************************************************************************
 *1) hps            要打开的进程句柄
 *2) mark              特征码,支持通配符（??），如: 48 8B 48 ?? 48 8B 04 D1 48 8B ?? 8B
 *3) addr            起始搜索地址
 *4) endaddr        结束搜索地址
 *5) ret             记录找到的地址
 *6) n_size         多个地址时,数组的维数
 *6) deviation        特征码地址离目标地址的偏移距离，上负下正
 *7) is_call        是否为找CALL的跳转地址，true 则 ret[] 返回的是CALL跳转的地址
 *8) is_all            是否查找所有符合的地址，false找到第一个符合的地址后就结束搜索.
 *return返回值        找到的地址总数, 未找到返回0.
 ***************************************************************************************/
static int
find_matching_code(HANDLE hps, const char *mark, uintptr_t addr,uintptr_t endaddr,
                   uintptr_t *ret, int n_size, int deviation, bool is_call, bool is_all)
{
    uint8_t *pmark = NULL;
    uint8_t *pwild = NULL;
    int n_sunday = 0;
    char mark_code[MAX_PATH] = { 0 };
    size_t len = strlen(mark);
    int c = 0;
    // 偏移量
    int offset;
    // 数组下标：内存、特征码、返回地址
    int i = 0, j = 0, count = 0;
    // 内存信息
    MEMORY_BASIC_INFORMATION mbi;
    if (MAX_PATH < len)
    {   // 子串太长
        return 0;
    }
    // 删掉头部2个通配符
    if (mark[0] == '?')
    {
        c += 2;
    }
    // 如果开头还是通配符,不查找
    if (mark[c] == '?')
    {
        return 0;
    }
    // 去除所有空格
    for (int k = 0; c < (int)len; ++c)
    {
        if (mark[c] != ' ')
        {
            mark_code[k++] = mark[c];
        }
    }
    len = strlen(mark_code);
#if APP_DEBUG
    printf("addr = 0x%zx, endaddr = 0x%zx, mark_code.length = %zu, mark_code = %s\n", addr, endaddr, len, mark_code);
#endif
    // 特征码长度不能为单数
    if (len % 2 != 0)
    {
        return 0;
    }
    // 特征码长度
    len /= 2;

    // Sunday算法,数组的长度
    n_sunday = (int) len;

    // 将特征码转换成byte型
    pmark = (uint8_t *) calloc(1, len);
    pwild = (uint8_t *) calloc(1, len);
    if (!(pmark && pwild))
    {
        return 0;
    }
    for (int i = 0; i < (int)len; i++)
    {
        char temp[3] = { 0 };
        strncpy(temp, &mark_code[i * 2], 2);
        if (strcmp(temp, "??") == 0)
        {
            pwild[i] = 0xFF;
            if (n_sunday == len)
            {
                // 记录第一个通配符的索引，该索引越靠后，效率越高
                n_sunday = i;
            }
        }
        else
        {
            pwild[i] = 0x00;
        }
        pmark[i] = (uint8_t) strtoul(temp, 0, 16);
    }
    // Sunday算法,数组赋值，+1防止特征码出现FF时越界
    int a_sunday[0xFF + 1] = { 0 };
    for (int i = 0; i < n_sunday; i++)
    {
        a_sunday[pmark[i]] = i + 1;
    }
    // 起始地址
    const uintptr_t dw_begin = addr;
    // 结束地址
    const uintptr_t dw_end = endaddr;
    // 当前读取的内存块地址
    uintptr_t cur_addr = dw_begin;
    // 存放内存数据的缓冲区
    uint8_t *pmem = NULL;
    // 扫描内存
    while (cur_addr < dw_end)
    {   // 查询地址空间中内存地址的信息
        memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION));
        if (VirtualQueryEx(hps, (LPCVOID) cur_addr, &mbi, sizeof(mbi)) == 0)
        {
            goto end;
        }
        if (MEM_COMMIT == mbi.State && PAGE_EXECUTE_READ == mbi.Protect)
        {
            if (pmem)
            {
                free(pmem);
                pmem = NULL;
            } // 根据内存页大小动态分配内存
            pmem = (uint8_t *) calloc(1, mbi.RegionSize);
            if (!pmem)
            {
                break;
            } // 读取进程内存
            ReadProcessMemory(hps, (LPCVOID) cur_addr, pmem, mbi.RegionSize, 0);
            i = 0;
            j = 0;
            while (j < (int)len)
            {
                if (pmem[i] == pmark[j] || pwild[j] == 0xFF)
                {
                    i++;
                    j++;
                }
                else
                {
                    offset = i - j + n_sunday;
                    // 判断偏移量是否大于缓冲区
                    if (offset > eu_int_cast(mbi.RegionSize - len))
                    {
                        break;
                    }
                    // 判断a_sunday数组里有没有 内存偏移后的值，有则回溯，否则+1
                    if (a_sunday[pmem[offset]])
                    {
                        i = offset - a_sunday[pmem[offset]] + 1;
                        j = 0;
                    }
                    else
                    {
                        i = offset + 1;
                        j = 0;
                    }
                }
            }
            if (j == len)
            {
                // deviation 为call地址与特征码首地址的偏移
                ret[count] = cur_addr + i - len + deviation;
                if (is_call)
                {
                    DWORD temp;
                    memcpy(&temp, &pmem[i - len + deviation + 1], 4);
                    ret[count] += 5;
                    // 高位为1时, 是负数, 转为int则为减.
                    if ((temp >> 31) == 1)
                    {
                        ret[count] += (int) temp;
                    }
                    else
                    {
                        ret[count] += temp;
                    }
                #if APP_DEBUG
                    printf("ret[%d] = 0x%zx\n", count, ret[count]);
                #endif
                }
                if (++count >= n_size)
                {
                    goto end;
                }
                if (is_all)
                {
                    i = i - (int) len + 1;
                    j = 0;
                    continue;
                }
                else
                {
                    goto end;
                }
            }
            // 取下一块内存地址
            cur_addr += mbi.RegionSize;
        }
        else
        {
            // 没找到, 继续下一页
            cur_addr += mbi.RegionSize;
        }
    }
end:
    if (pmem)
    {
        free(pmem);
        pmem = NULL;
    }
    free(pmark);
    pmark = NULL;
    free(pwild);
    pwild = NULL;
    return count;
}

static uint32_t
get_module_size(HANDLE hprocess, HMODULE hmod)
{
    MEMORY_BASIC_INFORMATION mi;
    PVOID base = NULL;
    uint32_t size = 0;
    VirtualQueryEx(hprocess, hmod, &mi, sizeof(mi));
    base = mi.AllocationBase;

    while (mi.AllocationBase == base)
    {
        size += (uint32_t) mi.RegionSize;
        VirtualQueryEx(hprocess, (LPCVOID)((uintptr_t) hmod + size), &mi, sizeof(mi));
    }
    return size;
}

static bool
do_hook(void *func, void *hook_func, void **orig_func)
{
    int index = 0;
    uint32_t uold = 0;
    uint8_t *f = (uint8_t *) func;
    if (!(func && orig_func && orig_func))
    {
        return false;
    }
    // find return instruction ret(retq): c3
    while (true)
    {
        if (f[index++] == 0xc3 || index >= MAX_BUFFER)
        {
            break;
        }
    }
    if (!VirtualProtect(func, index, PAGE_EXECUTE_READWRITE, &uold))
    {
        return false;
    }
    void *old = __vmalloc(index);
    if (!old)
    {
        return false;
    }
    memcpy(old, func, index);
    *orig_func = old;
    *(uint16_t *) &f[0] = 0x25ff;
    *(uint32_t *) &f[2] = 0x00000000;
    *(uintptr_t *) &f[6] = (uintptr_t) hook_func;
    return VirtualProtect(func, index, uold, &uold);
}

static void
put_zrow_bottom_hook(void *ptc, int moving)
{
#if APP_DEBUG
    printf("put_zrow_bottom run\n");
#endif
    return;
}

static unsigned WINAPI
comctl_thread(void *lp)
{
    const char *mark = "48 8B 48 ?? 48 8B 04 D1 48 8B ?? 8B 50 28";
    HMODULE hdll = GetModuleHandle(TEXT("comctl32.dll"));
    if (hdll)
    {
        uint32_t m_size = get_module_size(GetCurrentProcess(), hdll);
        if (m_size > 0)
        {
            uintptr_t ret[1] = {0};
            find_matching_code(GetCurrentProcess(), mark, (uintptr_t)hdll, (uintptr_t)hdll + m_size, ret, 1, 0xe, true, false);
            if (ret[0] > 0)
            {
                do_hook((void*)ret[0], put_zrow_bottom_hook, (void **)&put_zrow_bottom_stub);
            }
        }
    }
    return 0;
}

void
on_hook_do(void)
{
#ifdef _M_X64
    if (!util_under_wine())
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, comctl_thread, NULL, 0, NULL));
    }
#endif
}

void
on_hook_undo(void)
{
#ifdef _M_X64    
    if (put_zrow_bottom_stub)
    {
        __vfree(put_zrow_bottom_stub);
        put_zrow_bottom_stub = NULL;
    }
#endif    
}

static uintptr_t
rva_to_va(void *base, uintptr_t rva)
{
    return (uintptr_t)base + rva;
}

static uintptr_t
get_data_directory_from_base(void *module_base, uint32_t entry_id)
{
    PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)(module_base);
    PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)rva_to_va(module_base, (uintptr_t)dosHdr->e_lfanew);
    PIMAGE_DATA_DIRECTORY dataDir = ntHdr->OptionalHeader.DataDirectory;
    return rva_to_va(module_base, (uintptr_t)dataDir[entry_id].VirtualAddress);
}

static PIMAGE_THUNK_DATA
find_address_by_name(void *module_base, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *func_name)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
        {
            continue;
        }
        PIMAGE_IMPORT_BY_NAME import = (PIMAGE_IMPORT_BY_NAME)rva_to_va(module_base, impName->u1.AddressOfData);
        if (strcmp(import->Name, func_name) != 0)
        {
            continue;
        }
        return impAddr;
    }
    return NULL;
}

static PIMAGE_THUNK_DATA
find_address_by_ordinal(void *module_base, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
    UNREFERENCED_PARAMETER(module_base);
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
        {
            return impAddr;
        }
    }
    return NULL;
}

static PIMAGE_THUNK_DATA
find_iat_thunk(void *module_base, const char *dll_name, const char *func_name)
{
    PIMAGE_IMPORT_DESCRIPTOR imports = (PIMAGE_IMPORT_DESCRIPTOR)get_data_directory_from_base(module_base, IMAGE_DIRECTORY_ENTRY_IMPORT);
    for (; imports->Name; ++imports)
    {
        if (_stricmp((LPCSTR)rva_to_va(module_base, (uintptr_t)imports->Name), dll_name) != 0)
        {
            continue;
        }
        PIMAGE_THUNK_DATA origThunk = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->FirstThunk);
        return find_address_by_name(module_base, origThunk, thunk, func_name);
    }
    return NULL;
}

static PIMAGE_THUNK_DATA
find_delayload_thunk_by_name(void *module_base, const char *dll_name, const char *func_name)
{
    PIMAGE_DELAYLOAD_DESCRIPTOR imports = (PIMAGE_DELAYLOAD_DESCRIPTOR)get_data_directory_from_base(module_base, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp((LPCSTR)rva_to_va(module_base, (uintptr_t)imports->DllNameRVA), dll_name) != 0)
        {
            continue;
        }
        PIMAGE_THUNK_DATA impName = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->ImportNameTableRVA);
        PIMAGE_THUNK_DATA impAddr = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->ImportAddressTableRVA);
        return find_address_by_name(module_base, impName, impAddr, func_name);
    }
    return NULL;
}

PIMAGE_THUNK_DATA
find_delayload_thunk_by_ordinal(void *module_base, const char *dll_name, uint16_t ordinal)
{
    PIMAGE_DELAYLOAD_DESCRIPTOR imports = (PIMAGE_DELAYLOAD_DESCRIPTOR)get_data_directory_from_base(module_base, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp((LPCSTR)rva_to_va(module_base, (uintptr_t)imports->DllNameRVA), dll_name) != 0)
        {
            continue;
        }
        PIMAGE_THUNK_DATA impName = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->ImportNameTableRVA);
        PIMAGE_THUNK_DATA impAddr = (PIMAGE_THUNK_DATA)rva_to_va(module_base, (uintptr_t)imports->ImportAddressTableRVA);
        return find_address_by_ordinal(module_base, impName, impAddr, ordinal);
    }
    return NULL;
}
