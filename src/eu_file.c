/*******************************************************************************
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

#include <fcntl.h>
#include "framework.h"

#define MINIMUM_MEM 0x12c00000

static volatile long push_thread_id;
static volatile long file_close_id;
static HANDLE file_event_final = NULL;

static bool
set_ext_filter(const TCHAR *ext, TCHAR **pfilter)
{
    TCHAR p_all[] = {'A','l','l',' ','F','i','e','s','(','*','.','*',')','\0','*','.','*','\0','\0'};
    if (!(*pfilter = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH+1)))
    {
        return false;
    }    
    if (ext)
    {
        TCHAR *pext = NULL;
        TCHAR *pdesc = NULL;
        TCHAR exts[ACNAME_LEN+1] = {0};
        char u8_exts[ACNAME_LEN+1] = {0};
        _sntprintf(exts, ACNAME_LEN, _T("*%s;"), ext);
        WideCharToMultiByte(CP_UTF8, 0, exts, -1, u8_exts, ACNAME_LEN, NULL, NULL);
        for (doctype_t *mapper = eu_doc_get_ptr(); mapper && mapper->extname; ++mapper)
        {
            if (strstr(mapper->extname, u8_exts) != NULL)
            {
                pext = eu_utf8_utf16(&mapper->extname[1], NULL);
                pdesc = eu_utf8_utf16(mapper->filedesc, NULL);
                if (_tcslen(pext) > 0 && pext[_tcslen(pext) - 1] == ';')
                {
                    pext[_tcslen(pext) - 1] = 0;
                }
                break;
            }
        }
        if (pext && pdesc)
        {
            int l = _sntprintf(*pfilter, MAX_PATH, _T("%s(%s)"), pdesc, pext);
            int ext_len = _tcslen(exts) > 0 ? (int)_tcslen(exts) - 1 : 0;
            exts[ext_len] = 0;   // strip ';'
            memcpy(*pfilter + l + 1, exts, ext_len * sizeof(TCHAR));
            memcpy(*pfilter + l + ext_len + 2, p_all, sizeof(p_all));
            free(pext);
            free(pdesc);
            return true;
        }
    }
    memcpy(*pfilter, p_all, sizeof(p_all));
    return true;
}

int
on_file_opendlg_filename(HWND hwnd, TCHAR *file_name, int name_len)
{
    OPENFILENAME ofn = {sizeof(OPENFILENAME),};
    if (!file_name)
    {
        return EUE_POINT_NULL;
    }
    LOAD_I18N_RESSTR(IDS_TOOLBAR_1, lptext);
    *file_name = 0;
    ofn.hwndOwner = hwnd ? hwnd : eu_module_hwnd();
    ofn.hInstance = eu_module_handle();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = name_len;
    ofn.lpstrFilter = _T("All Files(*.*)\0*.*\0" \
                         "C/C++ Files(*.c;*.cpp;*.cxx;*.h;*.hpp;*.rc)\0*.c;*.cpp;*.cxx;*.h;*.hpp;*.rc\0" \
                         "Script Files(*.bat;*.cmd;*.sh;*.lua;*.pl;*.py)\0*.bat;*.cmd;*.sh;*.lua;*.pl;*.py\0" \
                         "HTML Files(*.htm;*.html;*.xml;*.css;*.js)\0*.htm;*.html;*.xml;*.css;*.js\0" \
                         "PE Files(*.exe;*.com;*.dll;*.sys)\0*.exe;*.com;*.dll;*.sys\0\0");
    ofn.lpstrTitle = lptext;
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY;
    return !GetOpenFileName(&ofn);
}

static int
get_savedlg_filename(TCHAR *file_name, int name_len)
{
    int err = SKYLARK_OK;
    TCHAR *filter = NULL;
    OPENFILENAME ofn = {sizeof(ofn),};
    if (!file_name)
    {
        return EUE_POINT_NULL;
    }
    LOAD_I18N_RESSTR(IDS_TOOLBAR_2, lptext);
    TCHAR *p = _tcsrchr(file_name, _T('.'));
    if (!set_ext_filter(p, &filter))
    {
        return EUE_EXT_FILTER_ERR;
    }
    ofn.hwndOwner = eu_module_hwnd();
    ofn.hInstance = eu_module_handle();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = name_len;
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = lptext;
    ofn.Flags = OFN_ENABLESIZING | OFN_HIDEREADONLY;
    err = !GetSaveFileName(&ofn);
    free(filter);
    return err;
}

static int
sqlite_callback(void *data, int count, char **column, char **names)
{
    HMENU hre = (HMENU)data;
    int index = GetMenuItemCount(hre);
    for (int i = 0; i < count && column[i]; ++i)
    {
        if (column[i][0] != 0)
        {
            TCHAR *ptr_row = eu_utf8_utf16(column[i], NULL);
            if (ptr_row != NULL)
            {
                AppendMenu(hre, MF_POPUP | MF_STRING, IDM_HISTORY_BASE + index, ptr_row);
                free(ptr_row); 
            }
        }
    }
    return 0;
}

static unsigned __stdcall
push_file_thread(void *lp)
{  
    struct _stat buf = {0};
    const TCHAR *pathfile = (const TCHAR *)lp;
    int result = _tstat (pathfile, &buf); 
    if (!result)
    {
        char *pfile = NULL;
        char sql[MAX_BUFFER] = {0};
        if ((pfile = eu_utf16_utf8(pathfile, NULL)) != NULL)
        {
            _snprintf(sql, MAX_BUFFER-1, "insert or ignore into file_recent(szName,szDate) values('%s', %I64u);", pfile, buf.st_mtime);
        }
        if (*sql && eu_sqlite3_send(sql, NULL, NULL) != 0)
        {
            printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
        }
        if (pfile)
        {
            free(pfile);
        }
    }
    return 0;
}

static unsigned __stdcall
update_menu_thread(void *lp)
{
    HMENU hroot = GetMenu(eu_module_hwnd());
    if (hroot)
    {
        HMENU hre = NULL;
        HMENU hfile = GetSubMenu(hroot, 0);
        hre = hfile ? GetSubMenu(hfile, 2) : NULL;
        if (hre)
        {
            int count = GetMenuItemCount(hre);
            for (int index = 0; index < count; ++index)
            {
                DeleteMenu(hre, 0, MF_BYPOSITION);
            }
            if (eu_sqlite3_send("SELECT szName FROM file_recent;", sqlite_callback, (void *)hre) != 0)
            {
                printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
            }
            if ((count = GetMenuItemCount(hre)) == 0)
            {
                LOAD_I18N_RESSTR(IDC_MSG_HISTORY_ZERO, m_str);
                if (AppendMenu(hre, MF_POPUP | MF_STRING, IDM_HISTORY_BASE, m_str))
                {
                    util_enable_menu_item(eu_module_hwnd(), IDM_HISTORY_BASE, false);
                }
            }
        }
    }
    _InterlockedExchange(&push_thread_id, 0);
    return 0;
}

static void
on_file_push_recent(const TCHAR *path)
{
    HANDLE hthread = (HANDLE) _beginthreadex(NULL, 0, push_file_thread, (void *)path, CREATE_SUSPENDED, NULL);
    if (hthread)
    {
        SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);
        ResumeThread(hthread);
        CloseHandle(hthread);
    }
}

void
on_file_update_recent(void)
{   // 防止多个线程同时刷新menu菜单
    if (!push_thread_id)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, update_menu_thread, NULL, 0, (unsigned *)&push_thread_id));
    }
}

void
on_file_clear_recent(void)
{
    HMENU hroot = GetMenu(eu_module_hwnd());
    if (hroot)
    {
        HMENU hre = NULL;
        HMENU hfile = GetSubMenu(hroot, 0);
        hre = hfile ? GetSubMenu(hfile, 2) : NULL;
        if (hre && eu_sqlite3_send("delete from file_recent;", NULL, NULL) == 0)
        {
            bool ret = false;
            int count = GetMenuItemCount(hre);
            for (int index = 0; index < count; ++index)
            {
                if (!(ret = DeleteMenu(hre, 0, MF_BYPOSITION)))
                {
                    break;
                }
            }
            if (ret)
            {
                LOAD_I18N_RESSTR(IDC_MSG_HISTORY_ZERO, m_str);
                if (AppendMenu(hre, MF_POPUP | MF_STRING, IDM_HISTORY_BASE, m_str))
                {
                    util_enable_menu_item(eu_module_hwnd(), IDM_HISTORY_BASE, false);
                }
            }
        }
    }
}

void
on_file_splite_path(const TCHAR *full_path, TCHAR *dri_name, TCHAR *pathname, TCHAR *filename, TCHAR *mainname, TCHAR *extname)
{
    const TCHAR *p1 = NULL;
    const TCHAR *p2 = NULL;
    const TCHAR *path_base = NULL;
    const TCHAR *name_base = NULL;
    TCHAR dri[8] = { 0 };
    p1 = _tcschr(full_path, _T(':'));
    if (p1)
    {
        path_base = p1 + 1;
        _stprintf(dri, _T("%.*s"), (int) (path_base - full_path), full_path);
    }
    else
    {
        path_base = full_path;
    }
    if (dri_name && dri[0]) 
    {
        _tcscpy(dri_name, dri);
    }
    p1 = _tcsrchr(path_base, _T('/'));
    p2 = _tcsrchr(path_base, _T('\\'));
    if (p1 == NULL && p2 == NULL)
    {
        p1 = path_base;
    }
    else if (p1 && p2)
    {
        if (p1 < p2)
        {
            p1 = p2 + 1;
        }
        else
        {
            p1++;
        }
    }
    else
    {
        if (p2)
        {
            p1 = p2 + 1;
        }
        else
        {
            p1++;
        }
    }
    name_base = p1;
    if (pathname) 
    {
        _stprintf(pathname, _T("%s%.*s"), dri, (int) (name_base - path_base), path_base);
    }
    if (filename) 
    {
        _tcscpy(filename, name_base);
    }
    p1 = _tcsrchr(name_base, _T('.'));
    if (p1)
    {
        if (mainname) 
        {
            _stprintf(mainname, _T("%.*s"), (int) (p1 - name_base), name_base);
        }
        if (extname) 
        {
            _tcscpy(extname, p1);
        }
    }
    else
    {
        if (mainname)
        {
            _tcscpy(mainname, name_base);
        }
        if (extname) 
        {
            extname[0] = _T('\0');
        }
    }
}

int
on_file_new(void)
{
    eu_tabpage *pnode = NULL;
    TCHAR filename[100] = {0};
    const uint8_t *bom_str = NULL;
    if ((pnode = (eu_tabpage *) calloc(1, sizeof(eu_tabpage))) == NULL)
    {
        return EUE_POINT_NULL;
    }
    else
    {
        pnode->is_blank = true;
        pnode->begin_pos = -1;
    }
    if (on_tabpage_newdoc_name(filename, 100)[0])
    {
        _tcscpy(pnode->pathfile, filename);
        _tcscpy(pnode->filename, filename);
        
    }
    if (on_tabpage_add(pnode))
    {
        eu_safe_free(pnode);
        return EUE_INSERT_TAB_FAIL;
    }
    else
    {
        on_sci_before_file(pnode);
        eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
        pnode->eol = eu_get_config()->new_file_eol;
        pnode->codepage = eu_get_config()->new_file_enc;
    }
    switch (pnode->codepage)
    {
        case IDM_UNI_UTF8B:
            bom_str = (const uint8_t *) "\xEF\xBB\xBF";
            break;
        case IDM_UNI_UTF16LEB:
            bom_str = (const uint8_t *) "\xFF\xFE";
            break;
        case IDM_UNI_UTF16BEB:
            bom_str = (const uint8_t *) "\xFE\xFF";
            break;
        default:
            break;
    }
    if (bom_str)
    {
        on_encoding_set_bom(bom_str, pnode);
    }
    if (true)
    {
        util_set_title(pnode->pathfile);
        on_sci_after_file(pnode);
        on_tabpage_selection(pnode, -1);
        eu_window_resize(eu_module_hwnd());
    }
    return SKYLARK_OK;
}

static bool
get_file_size(HANDLE hfile, uint64_t *psize)
{
    if (!GetFileSizeEx(hfile, (LARGE_INTEGER *) psize))
    {
        *psize = 0;
        return false;
    }
    return true;
}

uint64_t WINAPI
on_file_get_avail_phys(void)
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return (statex.ullAvailPhys);
}

static bool
map_hex_file(eu_tabpage *pnode, HANDLE hfile, size_t nbyte)
{
    if (!pnode || pnode->phex)
    {
        return false;
    }
    // 如果是二进制文件, 使用内存映射
    pnode->phex = (PHEXVIEW) calloc(1, sizeof(HEXVIEW));
    if (!pnode->phex)
    {
        return false;
    }
    // PAGE_READWRITE, write immediately
    pnode->phex->hmap = share_create(hfile, PAGE_WRITECOPY, nbyte, NULL);
    if (pnode->phex->hmap == NULL)
    {
        printf("share_create failed, cause %lu\n", GetLastError());
        eu_safe_free(pnode->phex);
        return false;
    }
    // 映射全部文件
    pnode->phex->pbase = (uint8_t *)share_map(pnode->phex->hmap, nbyte, FILE_MAP_COPY);
    if (pnode->phex->pbase == NULL)
    {
        printf("share_map failed, cause %lu\n", GetLastError());
        safe_close_handle(pnode->phex->hmap);
        eu_safe_free(pnode->phex);
        return false;
    }
    pnode->phex->ex_style |= (pnode->file_attr & FILE_ATTRIBUTE_READONLY) ? HVS_READONLY : 0;
    return true;
}

static int
load_file_pre(eu_tabpage *pnode, file_backup *pbak)
{
    int err = SKYLARK_OK;
    int bytesread = 0;
    uint8_t *pdata = NULL;
    HANDLE hfile = NULL;
    HANDLE hmap = NULL;
    FILE *fp = NULL;
    uint64_t block = 0;
    char buf[CHECK_LEN] = { 0 };
    TCHAR *pfull = pbak->rel_path;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pbak->status)
    {
        pfull = pbak->bak_path;
    }
    if (*pfull == 0)
    {
        return EUE_PATH_NULL;
    }
    if ((pnode->file_attr = GetFileAttributes(pfull)) == INVALID_FILE_ATTRIBUTES)
    {
        return EUE_FILE_ATTR_ERR;
    }
    if (!share_open_file(pfull, true, OPEN_EXISTING, &hfile))
    {
        return EUE_API_OPEN_FILE_ERR;
    }
    if (!get_file_size(hfile, &pnode->raw_size))
    {
        safe_close_handle(hfile);
        return EUE_FILE_SIZE_ERR;
    }
    if (on_file_get_avail_phys() - pnode->raw_size < MINIMUM_MEM)
    {
        // phymem < 300MB, Skylark exit
        safe_close_handle(hfile);
        MSG_BOX(IDC_MSG_MEM_NOT_AVAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return EUE_NOT_ENOUGH_MEMORY;
    }
    if (pbak->cp)
    {   // 存在备份,不再测试编码
        pnode->eol = pbak->eol;
        pnode->codepage = pbak->cp;
        pnode->hex_mode = !!pbak->hex;
        if (pnode->codepage == IDM_OTHER_BIN)
        {
            if (!map_hex_file(pnode, hfile, 0))
            {
                err = EUE_MAP_HEX_ERR;
            }
        }
        else if (pnode->hex_mode)
        {
            printf("on hex_mode is true\n");
            if ((err = !map_hex_file(pnode, hfile, 0)) == 0)
            {
                if (pbak->bakcp == pbak->cp || !pbak->status)
                {
                    pnode->phex->hex_ascii = true;
                }
                else
                {
                    on_encoding_set_bom_from_cp(pnode);
                }
            }
        }
        else
        {
            on_encoding_set_bom_from_cp(pnode);
        }        
        safe_close_handle(hfile);
        return err;
    }
    if (!ReadFile(hfile, buf, sizeof(buf), &bytesread, NULL))
    {
        safe_close_handle(hfile);
        MSG_BOX_ERR(IDC_MSG_OPEN_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return EUE_API_READ_FILE_ERR;
    }
    if (bytesread < 1)
    {
        // 为空的新文件, 设为默认模式
        pnode->eol = eu_get_config()->new_file_eol;
        pnode->codepage = eu_get_config()->new_file_enc;
    }
    else // 如果是utf8编码, 获取换行符, 否则, 等编码转换后再获取换行符
    {
        pnode->codepage = eu_try_encoding(buf, bytesread, false, pfull);
        if (pnode->codepage < IDM_UNI_UTF16LEB)
        {
            pnode->eol = on_encoding_line_mode(buf, bytesread);
        }
        on_encoding_set_bom((const uint8_t *) buf, pnode);
    }
    if (pnode->codepage == IDM_OTHER_BIN)
    {
        if (!map_hex_file(pnode, hfile, 0))
        {
            err = EUE_MAP_HEX_ERR;
        }
    }
    safe_close_handle(hfile);
    return err;
}

int
on_file_to_tab(eu_tabpage *pnode, file_backup *pbak)
{
    size_t len = 0;
    size_t buf_len = 0;
    size_t pre_offset = 0;
    size_t err = SKYLARK_OK;
    FILE *fp = NULL;
    uint8_t *data = NULL;
    bool bin_file = false;
    bool is_utf8 = false;
    TCHAR *pfull = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pnode->codepage == IDM_OTHER_BIN || pnode->hex_mode)
    {
        return SKYLARK_OK;
    }
    pfull = pnode->pathfile;
    if (pbak && pbak->status)
    {
        pfull = pbak->bak_path;
        is_utf8 = true;
    }
    if (*pfull == 0)
    {
        return EUE_PATH_NULL;
    }
    if (!is_utf8)
    {
        pre_offset = pnode->pre_len;
    }
    if (true)
    {   // io读取
        buf_len = BUFF_SIZE;
        if (buf_len > pnode->raw_size)
        {
            buf_len = pnode->raw_size;
        }
        if ((fp = _tfopen(pfull, _T("rb"))) == NULL)
        {
            MSG_BOX_ERR(IDC_MSG_OPEN_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return EUE_OPEN_FILE_ERR;
        }
        if ((data = (uint8_t *) calloc(1, buf_len)) == NULL)
        {
            fclose(fp);
            return EUE_OUT_OF_MEMORY;
        }
        else
        {
            fseek(fp, pre_offset, SEEK_SET);
        }
    }
    while (true)
    {
        if ((len = fread((char *) data, 1, buf_len, fp)) <= 0)
        {
            break;
        }
        if (!is_utf8 && pnode->codepage > IDM_UNI_UTF8B && pnode->codepage < IDM_OTHER_BIN)
        {
            char *pdst = NULL;
            size_t dst_len = 0;
            euconv_t evd = { 0 };
            evd.src_from = eu_query_encoding_name(pnode->codepage);
            evd.dst_to = "utf-8";
            err = on_encoding_do_iconv(&evd, (char *) (data), &len, &pdst, &dst_len);
            if (err == (size_t) -1)
            {
                printf("on_encoding_do_iconv error in %s\n", __FUNCTION__);
                err = EUE_ICONV_FAIL;
                MSG_BOX(IDC_MSG_ICONV_FAIL2, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            if (pdst)
            {
                if (pnode->eol < 0)
                {
                    pnode->eol = on_encoding_line_mode(pdst, dst_len);
                    if (pnode->eol < 0)
                    {
                        pnode->eol = eu_get_config()->new_file_eol;
                    }
                }
                eu_sci_call(pnode, SCI_ADDTEXT, dst_len, (LPARAM)(pdst));
                eu_safe_free(pdst);
            }
        }
        else
        {
            eu_sci_call(pnode, SCI_ADDTEXT, len, (LPARAM)(data));
        }
    }
    if (fp)
    {
        fclose(fp);
    }
    return (int)err;
}

static bool
file_open_if(const TCHAR *pfile)
{
    bool  res = false;
    eu_tabpage *pnode = NULL;
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        if (!TabCtrl_GetItem(g_tabpages, index, &tci))
        {
            printf("TabCtrl_GetItem return failed on %s:%d\n", __FILE__, __LINE__);
            break;
        }
        if ((pnode = (eu_tabpage *) (tci.lParam)))
        {
            if (_tcscmp(pnode->pathfile, pfile) == 0)
            {
                on_tabpage_selection(pnode, index);
                res = true;
                break;
            }
            else if (_tcsnicmp(pfile, _T("sftp://"), 7) != 0 && _tcsrchr(pfile, _T('/')))
            {
                TCHAR temp[MAX_PATH+1] = {0};
                _sntprintf(temp, MAX_PATH, _T("%s"), pfile);
                eu_wstr_replace(temp, MAX_PATH, _T("/"), _T("\\"));
                if (_tcscmp(pnode->pathfile, temp) == 0)
                {
                    on_tabpage_selection(pnode, index);
                    res = true;
                    break;
                }
            }
        }
    }
    return res;
}

static void
update_node_time(eu_tabpage *pnode, time_t m)
{
    if (m)
    {
        pnode->st_mtime = m;
    }
    else if (pnode->pathfile[0])
    {
        struct _stat statbuf;
        _tstat(pnode->pathfile, &statbuf);
        pnode->st_mtime = statbuf.st_mtime;
    }
}

static void
active_other_tab(int index)
{
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (!count)
    {
        // 最后一个标签
        file_backup bak = {0};
        share_send_msg(&bak);
    }
    else
    {
        // 激活另一个标签
        on_tabpage_select_index(index < count - 1 ? index : count - 1);
    }
}

int
on_file_only_open(file_backup *pbak)
{
    TCHAR pathname[MAX_PATH];
    TCHAR filename[MAX_PATH];
    eu_tabpage *pnode = NULL;
    HANDLE hfile = NULL;
    if (file_open_if(pbak->rel_path))
    {
        return SKYLARK_OK;
    }
    if ((pnode = (eu_tabpage *) calloc(1, sizeof(eu_tabpage))) == NULL)
    {
        return EUE_OUT_OF_MEMORY;
    }
    if (true)
    {
        pnode->eol = -1;
        pnode->begin_pos = -1;
        on_file_splite_path(pbak->rel_path, NULL, pathname, filename, NULL, NULL);
        _tcsncpy(pnode->pathfile, pbak->rel_path, MAX_PATH - 1);
        _tcsncpy(pnode->pathname, pathname, MAX_PATH - 1);
        _tcsncpy(pnode->filename, filename, MAX_PATH - 1);
        // 有可能是远程文件
        if (_tcsnicmp(pbak->rel_path, _T("sftp://"), 7) == 0)
        {
            remotefs *pserver = on_remote_list_find(pbak->rel_path);
            if (pserver)
            {
                memcpy(&(pnode->fs_server), pserver, sizeof(remotefs));
            }
        }
        if (pbak->bak_path[0])
        {
            _tcsncpy(pnode->bakpath, pbak->bak_path, MAX_PATH - 1);
        }
        pnode->is_blank = pbak->blank;
        if (!pnode->is_blank)
        {
            struct _stat rel_t;
            struct _stat bak_t = {0};
            bak_t.st_mtime = INTPTR_MAX;
            _tstat(pbak->rel_path, &rel_t);
            if (pbak->bak_path[0])
            {
                _tstat(pbak->bak_path, &bak_t);
            }
            if (rel_t.st_mtime > bak_t.st_mtime)
            {
                pbak->status = 0;
            }
            pnode->zoom_level = pbak->zoom;
            update_node_time(pnode, rel_t.st_mtime);
        }
    }
    if (load_file_pre(pnode, pbak))
    {
        eu_safe_free(pnode);
        return EUE_PRESET_FILE_ERR;
    }
    if (on_tabpage_add(pnode))
    {
        eu_safe_free(pnode);
        return EUE_INSERT_TAB_FAIL;
    }
    if (pnode->codepage != IDM_OTHER_BIN)
    {
        on_sci_before_file(pnode);
        eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
        if (on_file_to_tab(pnode, pbak))
        {
            int index = on_tabpage_remove(&pnode);
            active_other_tab(index);
            return EUE_WRITE_TAB_FAIL;
        }
        on_sci_after_file(pnode);
        on_search_add_navigate_list(pnode, 0);
        int count = TabCtrl_GetItemCount(g_tabpages);
        {
            // 选中右边的标签
            on_tabpage_select_index(count - 1);
        }
        if (pbak->lineno > 0)
        {   // 跳转到行
            on_search_jmp_line(pnode, pbak->lineno, 0);
        }
        else
        {
            eu_sci_call(pnode, SCI_GOTOPOS, 0, 0);
        }
        if (strlen(pbak->mark_id) > 0)
        {   // 恢复书签
            on_search_update_mark(pnode, pbak->mark_id);
        }
    }
    if (pbak->status)
    {
        on_tabpage_editor_modify(pnode, "X");
    }
    on_file_push_recent(pnode->pathfile);
    return SKYLARK_OK;
}

static int
open_files(file_backup *pbak)
{
    if (!(pbak->rel_path[0] || pbak->bak_path[0]))
    {
        return on_file_new();
    }
    if (_tcslen(pbak->rel_path) > 0 && pbak->rel_path[_tcslen(pbak->rel_path) - 1] == _T('*'))
    {
        HANDLE hfile = NULL;
        WIN32_FIND_DATA st_file = {0};
        TCHAR base_path[MAX_PATH+1] = {0};
        _tcsncpy(base_path, pbak->rel_path, MAX_PATH);
        if (*base_path != 0)
        {
            if (_tcsrchr(base_path, _T('\\')))
            {
                _tcsrchr(base_path, _T('\\'))[0] = 0;
            }
        }
        if ((hfile = FindFirstFile(pbak->rel_path, &st_file)) == INVALID_HANDLE_VALUE)
        {
            MSG_BOX(IDC_MSG_OPEN_ERR1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_FILE_ATTR_ERR;
        }
        do
        {
            if (_tcscmp(st_file.cFileName, _T(".")) == 0 || _tcscmp(st_file.cFileName, _T("..")) == 0) 
            {
                continue;
            }
            if (!(st_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                file_backup bak = {0};
                _sntprintf(bak.rel_path, MAX_PATH - 1, _T("%s\\%s"), base_path, st_file.cFileName);
                on_file_only_open(&bak);
            }
        } while (FindNextFile(hfile, &st_file));
        FindClose(hfile);
        return SKYLARK_OK;
    }
    else if (!pbak->blank && _tcsnicmp(pbak->rel_path, _T("sftp://"), 7) != 0 && pbak->rel_path[1] != _T(':'))
    {
        TCHAR path[MAX_PATH] = {0};
        _tfullpath(path, pbak->rel_path, MAX_PATH);
        _sntprintf(pbak->rel_path, MAX_PATH, _T("%s"), path);
    }
    return on_file_only_open(pbak);
}

int
on_file_open(void)
{
    int err = SKYLARK_OK;
    TCHAR *file_list = (TCHAR *) calloc(sizeof(TCHAR), BUFF_32K);
    if (file_list == NULL)
    {
        printf("memory allocation failed\n");
        return EUE_OUT_OF_MEMORY;
    }
    if ((err = on_file_opendlg_filename(NULL, file_list, BUFF_32K)) != SKYLARK_OK)
    {
        goto mem_clean;
    }
    if (!(GetFileAttributes(file_list) & FILE_ATTRIBUTE_DIRECTORY))
    {
        file_backup bak = {0};
        _tcsncpy(bak.rel_path, file_list, MAX_PATH);
        err = on_file_only_open(&bak);
    }
    else
    {
        TCHAR pathname[MAX_PATH + 1] = {0};
        _tcsncpy(pathname, file_list, MAX_PATH);
        int path_len = (int) _tcslen(pathname);
        TCHAR *p = file_list + path_len + 1;
        while (*p)
        {
            file_backup bak = {0};
            _sntprintf(bak.rel_path, MAX_PATH, _T("%s\\%s"), pathname, p);
            if ((err = on_file_only_open(&bak)) != SKYLARK_OK)
            {
                goto mem_clean;
            }
            p += _tcslen(p) + 1;
        }
    }
mem_clean:
    if (file_list)
    {
        free(file_list);
    }
    return err;
}

static BOOL
set_window_top(HWND hwnd)
{
    HWND hwnd_force = GetForegroundWindow();
    DWORD dwForeID = GetWindowThreadProcessId(hwnd_force, NULL);
    DWORD dwCurID = GetCurrentThreadId();
    AttachThreadInput(dwCurID, dwForeID, TRUE);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetForegroundWindow(hwnd);
    AttachThreadInput(dwCurID, dwForeID, FALSE);
    return TRUE;
}

int
on_file_redirect(HWND hwnd, file_backup *pbak)
{
    int err = SKYLARK_OK;
    printf("on_file_redirect runing\n");
    if (pbak->status || _tcsnicmp(pbak->rel_path, _T("sftp://"), 7) != 0)
    {
        err = open_files(pbak);
    }
    else
    {
        printf("exec on_file_open_remote()\n");
        err = on_file_open_remote(NULL, pbak);
    }
    if (err != 0 && TabCtrl_GetItemCount(g_tabpages) < 1)
    {   // 打开文件失败与标签小于1,则建立一个空白标签页
        return on_file_new();
    }
    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }
    else if (hwnd != GetForegroundWindow())
    {
        set_window_top(hwnd);
    }
    ShowWindow(hwnd, SW_SHOW);
    return err;
}

int
on_file_drop(HDROP hdrop)
{
    file_backup bak = {0};
    int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
    for (int index = 0; index < count; ++index)
    {
        memset(bak.rel_path, 0, sizeof(bak.rel_path));
        DragQueryFile(hdrop, index, bak.rel_path, MAX_PATH);
        uint32_t attr = GetFileAttributes(bak.rel_path);
        if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            on_file_only_open(&bak);
        }
        else
        {
            _tcsncat(bak.rel_path, _T("\\*"), MAX_PATH);
            open_files(&bak);
            break;
        }
    }
    DragFinish(hdrop);
    return SKYLARK_OK;
}

static size_t
read_remote_file(void *buffer, size_t size, size_t nmemb, void *stream)
{
    eu_tabpage *pnode = (eu_tabpage *) stream;
    char *data = (char *) buffer;
    size_t len;
    size_t offset;
    len = size * nmemb;
    if (!pnode->bytes_remaining)
    {
        on_encoding_set_bom((const uint8_t *) buffer, pnode);
        pnode->codepage = eu_try_encoding((const char *) buffer, len, false, NULL);
        if (pnode->codepage < IDM_UNI_UTF16LEB)
        {
            pnode->eol = on_encoding_line_mode(buffer, len);
        }
        offset = pnode->pre_len;
    }
    else
    {
        offset = 0;
    }
    if (pnode->codepage > IDM_UNI_UTF8B && pnode->codepage < IDM_OTHER_BIN)
    {
        char *pdst = NULL;
        char *psrc = data + offset;
        size_t src_len = len - offset;
        size_t dst_len = 0;
        euconv_t evd = { 0 };
        evd.src_from = eu_query_encoding_name(pnode->codepage);
        evd.dst_to = "utf-8";
        size_t res = on_encoding_do_iconv(&evd, psrc, &src_len, &pdst, &dst_len);
        if (res == (size_t) -1)
        {
            return 0;
        }
        if (pnode->eol < 0)
        {
            pnode->eol = on_encoding_line_mode(pdst, dst_len);
            if (pnode->eol < 0)
            {
                pnode->eol = eu_get_config()->new_file_eol;
            }
        }
        eu_sci_call(pnode, SCI_ADDTEXT, dst_len, (LPARAM)(pdst));
        free(pdst);
    }
    else
    {
        eu_sci_call(pnode, SCI_ADDTEXT, len - offset, (LPARAM)(data + offset));
    }
    pnode->bytes_remaining += len - offset;
    return len;
}

int
on_file_open_remote(remotefs *premote, file_backup *pbak)
{
    char *cnv = NULL;
    CURL *curl = NULL;
    CURLcode res;
    int count = 0;
    TCHAR filename[MAX_PATH];
    TCHAR *full_path = pbak->rel_path;
    eu_tabpage *pnode = NULL;
    remotefs *pserver = premote;
    if (file_open_if(full_path))
    {
        return SKYLARK_OK;
    }
    if (!pserver)
    {
        if ((pserver = on_remote_list_find(pbak->rel_path)) == NULL)
        {
            return EUE_UNKOWN_ERR;
        }
    }
    printf("pserver->networkaddr = %s\n", pserver->networkaddr);
    if ((pnode = (eu_tabpage *) calloc(1, sizeof(eu_tabpage))) == NULL)
    {
        return EUE_OUT_OF_MEMORY;
    }
    if (true)
    {
        pnode->eol = -1;
        pnode->begin_pos = -1;
        on_file_splite_path(full_path, NULL, NULL, filename, NULL, NULL);
        memcpy(&(pnode->fs_server), pserver, sizeof(remotefs));
        _tcsncpy(pnode->pathfile, full_path, MAX_PATH - 1);
        _tcsncpy(pnode->filename, filename, MAX_PATH - 1);
    }
    if ((cnv = eu_utf16_utf8(full_path, NULL)) == NULL)
    {
        eu_safe_free(pnode);
        return EUE_API_CONV_FAIL;
    }
    if (on_tabpage_add(pnode))
    {
        eu_safe_free(pnode);
        return EUE_INSERT_TAB_FAIL;
    }
    on_sci_before_file(pnode);
    eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
    if (!(curl = on_remote_init_socket(cnv, pserver)))
    {
        eu_safe_free(pnode);
        return EUE_CURL_INIT_FAIL;
    }
    eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_remote_file);
    eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, pnode);
#if APP_DEBUG
    eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    eu_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90);
    res = eu_curl_easy_perform(curl);
    eu_curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        TabCtrl_DeleteItem(g_tabpages, pnode->tab_id);
        eu_safe_free(pnode);
        MSG_BOX(IDC_MSG_ATTACH_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return EUE_CURL_NETWORK_ERR;
    }
    else
    {
        pnode->zoom_level = pbak->zoom;
        util_set_title(pnode->pathfile);
        on_sci_after_file(pnode);
        eu_sci_call(pnode, SCI_GOTOPOS, 0, 0);
        on_search_add_navigate_list(pnode, 0);
        count = TabCtrl_GetItemCount(g_tabpages);
        on_tabpage_select_index(count - 1);
    }
    if (pnode->codepage == IDM_OTHER_BIN)
    {
        return hexview_switch_mode(pnode);
    }
    return SKYLARK_OK;
}

static int
do_write_file(eu_tabpage *pnode, TCHAR *pathfilename, bool isbak, bool save_as)
{
    int ret = SKYLARK_OK;
    FILE *fp = NULL;
    _set_errno(0);
    if (!pnode->hex_mode)
    {
        pnode->bytes_remaining = (size_t) eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
        if ((pnode->write_buffer = (uint8_t *)(on_sci_range_text(pnode, 0, pnode->bytes_remaining))) == NULL) 
        {
            ret = EUE_POINT_NULL;
            goto FILE_FINAL;
        }
        if (!save_as && eu_get_config()->m_limit && eu_get_config()->m_limit <= pnode->bytes_remaining/1024/1024)
        {   // 非二进制文件, 不产生备份, 直接写入原文件
            isbak = false;
            pathfilename = (pnode->pathfile[0]?pnode->pathfile:pathfilename);
        }
    }
    else if (pnode->codepage == IDM_OTHER_BIN)
    {
        // 原生的16进制视图
        // 是否自动cache
        bool is_cache = isbak && (!eu_get_config()->m_limit || eu_get_config()->m_limit > pnode->phex->total_items/1024/1024);
        // 既不另存为,又不产生缓存, 则直接写入源文件
        ret = hexview_save_data(pnode, save_as || is_cache ? pathfilename : NULL);
        goto FILE_FINAL;
    }
    else
    {   // 以流打开的16进制视图
        bool is_cache = isbak && (!eu_get_config()->m_limit || eu_get_config()->m_limit > pnode->phex->total_items/1024/1024);
        // hex_ascii为真,是原始的二进制,非utf8转码后的二进制
        if (pnode->phex->hex_ascii && pnode->phex->hmap)
        {
            printf("do not convert this code\n");
            //isbak = true;
            ret = hexview_save_data(pnode, save_as || is_cache ? pathfilename : NULL);
            goto FILE_FINAL;    
        }
        // 转码utf8后生成的二进制编码, 保存时需要转换回去
        pnode->write_buffer = hexview_strdup_data(pnode, &pnode->bytes_remaining);
        if (pnode->write_buffer == NULL)
        {   // 文件过大,没有足够内存
            ret = EUE_POINT_NULL;
            goto FILE_FINAL;
        }        
        if (!is_cache)
        {
            printf("convert this code\n");
            isbak = false;
        }
        pathfilename = save_as || is_cache ? pathfilename : pnode->pathfile;
    }
    if (!isbak && pnode->codepage > IDM_UNI_UTF8B && pnode->codepage < IDM_OTHER_BIN)
    {
        char *pdst = NULL;
        char *pbuf = (char *)pnode->write_buffer;
        size_t dst_len = 0;
        size_t src_len = pnode->bytes_remaining;
        euconv_t evd = { 0 };
        evd.src_from = "utf-8";
        evd.dst_to = eu_query_encoding_name(pnode->codepage);
        printf("convert(%s) to (%s)\n", evd.src_from, evd.dst_to);
        size_t res = on_encoding_do_iconv(&evd, pbuf, &src_len, &pdst, &dst_len);
        if (res != (size_t) -1)
        {  // 释放之前的内存, 指向新的内存块
            free(pnode->write_buffer);
            pnode->write_buffer = (uint8_t *)pdst;
            pnode->bytes_remaining = dst_len;
        }
        else
        {
            ret = EUE_ICONV_FAIL;
            MSG_BOX(IDC_MSG_ICONV_FAIL1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            goto FILE_FINAL;
        }
    }
    if ((fp = _tfopen(pathfilename, _T("wb"))) == NULL)
    {
        ret = EUE_OPEN_FILE_ERR;
        MSG_BOX_ERR(IDC_MSG_OPEN_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        goto FILE_FINAL;
    }   // 文件备份时, 不转换回原编码
    if (!isbak && pnode->pre_len > 0)
    {
        if (fwrite(pnode->pre_context, 1, pnode->pre_len, fp) < pnode->pre_len)
        {
            ret = EUE_WRITE_FILE_ERR;
            MSG_BOX(IDC_MSG_WRITE_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            goto FILE_FINAL;
        }
    }
    if (fwrite(pnode->write_buffer, pnode->bytes_remaining, 1, fp) == -1)
    {
        ret = EUE_WRITE_FILE_ERR;
        MSG_BOX(IDC_MSG_WRITE_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
    }
FILE_FINAL:
    if (pnode->write_buffer)
    {
        eu_safe_free(pnode->write_buffer);
    }
    if (fp)
    {
        fclose(fp);
    }
    if (!save_as)
    {
        update_node_time(pnode, 0);
    }
    return ret;
}

static size_t
write_remote_file(void *buffer, size_t size, size_t nmemb, void *stream)
{
    size_t len;
    eu_tabpage *pnode = (eu_tabpage *) stream;
    if (pnode->bytes_written >= pnode->bytes_remaining) 
    {
        return 0;
    }
    if (pnode->needpre)
    {
        memcpy(buffer, pnode->pre_context, pnode->pre_len);
        len = pnode->pre_len;
        pnode->needpre = false;
        return len;
    }
    len = size * nmemb;
    if (pnode->bytes_written + len > pnode->bytes_remaining)
    {
        len = pnode->bytes_remaining - pnode->bytes_written;
    }
    memcpy(buffer, pnode->write_buffer + pnode->bytes_written, len);
    pnode->bytes_written += len;
    return len;
}

static int
on_file_write_backup(eu_tabpage *pnode)
{
    if (pnode && !util_availed_char(pnode->fs_server.networkaddr[0]))
    {
        size_t len = _tcslen(pnode->pathfile);
        TCHAR *pbakup = len > 0 ? (TCHAR *)calloc(sizeof(TCHAR), len + 2) : NULL;
        if (pbakup)
        {
            _sntprintf(pbakup, len+1, _T("%s~"), pnode->pathfile);
            if (!CopyFile(pnode->pathfile, pbakup, false))
            {
                MSG_BOX(IDC_MSG_COPY_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
                return EUE_COPY_FILE_ERR;
            }
        }
    }
    return 0;
}

int
on_file_save(eu_tabpage *pnode, bool save_as)
{
    int err = SKYLARK_OK;
    char *cnv = NULL;
    char *pdst = NULL;
    char *ptext = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (!on_sci_doc_modified(pnode) && !save_as)
    {
        err = EUE_UNEXPECTED_SAVE;
        goto SAVE_FINAL;
    }
    else if (pnode->is_blank || save_as)
    {
        TCHAR full_path[MAX_PATH] = {0};
        TCHAR filename[MAX_PATH] = {0};
        _tcsncpy(full_path, pnode->filename, MAX_PATH);
        if (_tcslen(full_path) > 0 && full_path[_tcslen(full_path) - 1] == _T('*'))
        {
            full_path[_tcslen(full_path) - 1] = 0;
        }
        if (get_savedlg_filename(full_path, _countof(full_path)))
        {
            err = EUE_LOCAL_FILE_ERR;
            goto SAVE_FINAL;
        }
        on_file_splite_path(full_path, NULL, NULL, filename, NULL, NULL);
        if (do_write_file(pnode, full_path, false, true))
        {
            err = EUE_WRITE_FILE_ERR;
            goto SAVE_FINAL;
        }
        _tcsncpy(pnode->pathfile, full_path, MAX_PATH - 1);
        _tcsncpy(pnode->filename, filename, MAX_PATH - 1);
        // 有可能是远程服务器文件, 清除网址
        pnode->fs_server.networkaddr[0] = 0;
        update_node_time(pnode, 0);
        util_set_title(pnode->pathfile);
        on_file_push_recent(pnode->pathfile);
        pnode->doc_ptr = on_doc_get_type(pnode->filename);
        on_sci_before_file(pnode);
        on_sci_after_file(pnode);
        if (pnode->is_blank)
        {
            pnode->is_blank = false;
        }
    }
    else if (util_availed_char(pnode->fs_server.networkaddr[0]))
    {
        CURL *curl = NULL;
        CURLcode res;
        size_t buf_len = 0;
        if (pnode->codepage != IDM_OTHER_BIN)
        {
            if (pnode->hex_mode)
            {
                ptext = (char *)hexview_strdup_data(pnode, &buf_len);
                if (ptext == NULL)
                {
                    err = EUE_POINT_NULL;
                    goto SAVE_FINAL;
                }
            }
            else if (!(ptext = util_strdup_content(pnode, &buf_len)))
            {
                err = EUE_POINT_NULL;
                goto SAVE_FINAL;
            }
            if (pnode->codepage > IDM_UNI_UTF8B && pnode->codepage < IDM_OTHER_BIN)
            {
                char *psrc = ptext;
                size_t src_len = buf_len;
                size_t dst_len = 0;
                euconv_t evd = {0};
                evd.src_from = "utf-8";
                evd.dst_to = eu_query_encoding_name(pnode->codepage);
                size_t res = on_encoding_do_iconv(&evd, psrc, &src_len, (char **)&pnode->write_buffer, &pnode->bytes_remaining);
                if (res == (size_t) -1)
                {
                    MSG_BOX(IDC_MSG_ICONV_FAIL1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    err = EUE_ICONV_FAIL;
                    goto SAVE_FINAL;
                }
            }
            else
            {
                pnode->bytes_remaining = buf_len;
                pnode->write_buffer = (uint8_t *)ptext;
            }
        }
        else
        {
            pnode->write_buffer = hexview_strdup_data(pnode, &pnode->bytes_remaining);
            if (pnode->write_buffer == NULL)
            {
                err = EUE_POINT_NULL;
                goto SAVE_FINAL;
            }
        }
        pnode->bytes_written = 0;
        if (pnode->pre_len > 0)
        {
            pnode->needpre = true;
        }
        if ((cnv = eu_utf16_utf8(pnode->pathfile, NULL)) == NULL)
        {
            err = EUE_API_CONV_FAIL;
            goto SAVE_FINAL;
        }
        if (!(curl = on_remote_init_socket(cnv, &pnode->fs_server)))
        {
            printf("on_remote_init_socket return false\n");
            err = EUE_CURL_INIT_FAIL;
            goto SAVE_FINAL;
        }
        eu_curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_READFUNCTION, write_remote_file);
        eu_curl_easy_setopt(curl, CURLOPT_READDATA, pnode);
    #if defined(APP_DEBUG) && (APP_DEBUG > 0) 
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif    
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        eu_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120);
        res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        if (res != CURLE_OK)
        {
            err = EUE_CURL_NETWORK_ERR;
            MSG_BOX(IDC_MSG_ATTACH_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            goto SAVE_FINAL;
        }
        // 刷新状态栏时间
        pnode->st_mtime = 0;
    }    
    else
    {
        if (eu_get_config()->m_write_copy && on_file_write_backup(pnode))
        {
            err = EUE_WRITE_FILE_ERR;
            goto SAVE_FINAL;
        }
        if (do_write_file(pnode, pnode->pathfile, false, false))
        {
            err = EUE_WRITE_FILE_ERR;
        }
    }
SAVE_FINAL:
    eu_safe_free(ptext);
    eu_safe_free(pdst);
    eu_safe_free(cnv);
    eu_safe_free(pnode->write_buffer);
    if (!err)
    {
        // 发送SCI_SETSAVEPOINT消息
        eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
        if (!(pnode->is_blank || save_as))
        {
            if (pnode->doc_ptr && pnode->doc_ptr->fn_reload_symlist)
            {
                pnode->doc_ptr->fn_reload_symlist(pnode);
            }
        }
        menu_update_all(eu_module_hwnd(), pnode);
        eu_window_resize(NULL);
    }
    return err;
}

int
on_file_save_as(eu_tabpage *pnode)
{
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    return on_file_save(pnode, true);
}

int
on_file_all_save(void)
{
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        eu_tabpage *pnode = (eu_tabpage *) (tci.lParam);
        if (pnode)
        {
            on_file_save(pnode, false);
        }
    }
    return SKYLARK_OK;
}

static void 
create_guid(TCHAR *buf, int len)
{
    GUID  guid;
    if (S_OK == CoCreateGuid(&guid))
    {
        _sntprintf(buf,
                   len,
                   _T("%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x"),
                   guid.Data1, guid.Data2, guid.Data3,
                   guid.Data4[0], guid.Data4[1],
                   guid.Data4[2], guid.Data4[3],
                   guid.Data4[4], guid.Data4[5],
                   guid.Data4[6], guid.Data4[7]);
    }
    else
    {
        eu_rand_str(buf, 32);
    }
}

static void
save_file_backup(eu_tabpage *pnode)
{
    TCHAR buf[ACNAME_LEN] = {0};
    file_backup filebak = {0};
    filebak.cp = pnode->codepage;
    filebak.bakcp = pnode->codepage == IDM_OTHER_BIN ? IDM_OTHER_BIN : IDM_UNI_UTF8;
    if (on_sci_doc_modified(pnode))
    {
        create_guid(buf, ACNAME_LEN - 1);
        _sntprintf(filebak.bak_path, MAX_PATH, _T("%s\\conf\\cache\\%s"), eu_module_path, buf);
        do_write_file(pnode, filebak.bak_path, true, false);
        filebak.status = 1;
        if (pnode->hex_mode && pnode->phex && pnode->phex->hex_ascii)
        {
            filebak.bakcp = pnode->codepage;
        }
    }
    _tcscpy(filebak.rel_path, pnode->pathfile);
    filebak.tab_id = pnode->tab_id;
    filebak.eol = pnode->eol;
    filebak.blank = pnode->is_blank;
    filebak.hex = pnode->hex_mode;
    filebak.focus = pnode->last_focus;
    filebak.zoom = pnode->zoom_level > SELECTION_ZOOM_LEVEEL ? pnode->zoom_level : 0;
    on_search_page_mark(pnode, filebak.mark_id, MAX_BUFFER-1);
    sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    if (pos < 0)
    {
        filebak.lineno = -1;
    }
    else
    {
        filebak.lineno = (size_t)eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
    }
    eu_update_backup_table(&filebak);
}

int
on_file_close(eu_tabpage *pnode, CLOSE_MODE mode)
{
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (!mode && eu_get_config()->m_session && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0))
    {
        save_file_backup(pnode);
    }
    else if (on_sci_doc_modified(pnode))
    {
        TCHAR msg[100 + MAX_PATH];
        if (pnode->pathfile[0] == 0)
        {
            LOAD_I18N_RESSTR(IDC_MSG_SAVE_STR1, save_str);
            _sntprintf(msg, _countof(msg) - 1, save_str);
        }
        else
        {
            LOAD_I18N_RESSTR(IDC_MSG_SAVE_STR2, save_str);
            _sntprintf(msg, _countof(msg) - 1, save_str, pnode->pathfile);
        }
        LOAD_APP_RESSTR(IDS_APP_TITLE, title);
        /* 如果需要确认, 选中该标签 */
        on_tabpage_selection(pnode, -1);
        int decision = eu_msgbox(eu_module_hwnd(), msg, title, MB_YESNOCANCEL);
        if (decision == IDCANCEL)
        {
            printf("abort closing file\n");
            return 1;
        }
        else if (decision == IDYES)
        {
            on_file_save(pnode, false);
        }
    }
    /* 清理该文件的位置导航信息 */
    on_search_clean_navigate_this(pnode);
    int index = on_tabpage_remove(&pnode);
    if (mode == FILE_ONLY_CLOSE)
    {
        active_other_tab(index);
    }
    return SKYLARK_OK;
}

int
on_file_all_close(void)
{
    int this_index = 0;
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, this_index, &tci);
        eu_tabpage *pnode = (eu_tabpage *) (tci.lParam);
        if (on_file_close(pnode, FILE_ALL_CLOSE))
        {
            ++this_index;
        }
    }
    if (!this_index)
    {
        // 最后一个标签
        file_backup bak = {0};
        share_send_msg(&bak);
    }
    return SKYLARK_OK;
}

int
on_file_exclude_close(eu_tabpage *pnode)
{
    int this_index = 0;
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, this_index, &tci);
        eu_tabpage *p = (eu_tabpage *) (tci.lParam);
        if (p == pnode)
        {
            ++this_index;
            continue;
        }
        if (on_file_close(p, FILE_EXCLUDE_CLOSE))
        {
            ++this_index;
        }
    }
    on_tabpage_selection(pnode, 0);
    return SKYLARK_OK;
}

static unsigned __stdcall
check_files_save(void *lp)
{
    int err = 0;
    int at_focus = -1;
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (eu_get_config()->m_session)
    {
        eu_clear_backup_table();
        at_focus = TabCtrl_GetCurSel(g_tabpages);
    }
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, err, &tci);
        eu_tabpage *pnode = (eu_tabpage *) (tci.lParam);
        if (pnode)
        {
            pnode->tab_id = index;
            pnode->zoom_level = pnode->zoom_level > SELECTION_ZOOM_LEVEEL ? (int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0) : 0;
            if (at_focus >= 0)
            {
                pnode->last_focus = pnode->tab_id == at_focus;
            }
            if (on_file_close(pnode, FILE_SHUTDOWN))
            {
                ++err;
                continue;
            }
        }
    }
    if (!err)
    {
        PostMessage(eu_module_hwnd(), WM_BACKUP_OVER, 0, 0);
    }
    _InterlockedExchange(&file_close_id, 0);
    return err;
}

static void 
all_windows_shutdown(void)
{
    if (!file_close_id)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, check_files_save, NULL, 0, (DWORD *)&file_close_id));
        if (!file_event_final)
        {
            share_envent_create_close_sem(&file_event_final);
        }
        else
        {
            SetEvent(file_event_final);
        }
    }
}

void
on_file_finish_wait(void)
{
    if (file_event_final)
    {   // we destroy windows, that it should wait for file close thread exit
        WaitForSingleObject(file_event_final, INFINITE);
        safe_close_handle(file_event_final);
    }
}

void
on_file_edit_exit(HWND hwnd)
{
    if (eu_get_config()->m_session)
    {
        all_windows_shutdown();
        if (hwnd)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        if (eu_get_search_hwnd())
        {
            ShowWindow(eu_get_search_hwnd(), SW_HIDE);
        }
    }
    else
    {
        check_files_save(NULL);
    }
}

void
on_file_edit_restart(HWND hwnd)
{
    HANDLE handle = NULL;
    TCHAR process[MAX_PATH +1] = {0};
    if (GetModuleFileName(NULL , process , MAX_PATH) > 0)
    {
        int len = 0;
        _tcsncat(process, _T(" -restart "), MAX_PATH);
        len = (int)_tcslen(process);
        _sntprintf(process + len, MAX_PATH - len, _T("%lu"), GetCurrentProcessId());
        on_file_edit_exit(hwnd);
        CloseHandle(eu_new_process(process, NULL, NULL, 0, NULL));
    }
}

void
on_file_backup_menu(void)
{
    if (!eu_get_config()->m_write_copy)
    {
        eu_get_config()->m_write_copy = true;
    }
    else
    {
        eu_get_config()->m_write_copy = false;
    }
    menu_update_all(eu_module_hwnd(), NULL);
}

void
on_file_session_menu(void)
{
    if (!eu_get_config()->m_session)
    {
        eu_get_config()->m_session = true;
    }
    else
    {
        eu_get_config()->m_session = false;
    }
    menu_update_all(eu_module_hwnd(), NULL);
}

void
on_file_new_eols(eu_tabpage *pnode, int eol_mode)
{
    eu_get_config()->new_file_eol = eol_mode;
    menu_update_all(eu_module_hwnd(), NULL);
}

void
on_file_new_encoding(eu_tabpage *pnode, int new_enc)
{
    eu_get_config()->new_file_enc = new_enc;
    menu_update_all(eu_module_hwnd(), NULL);
}
