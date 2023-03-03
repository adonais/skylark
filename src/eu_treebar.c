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

typedef enum _GENERATE_TYPE
{
    MD5_GENERATE,
    SHA1_GENERATE,
    SHA256_GENERATE,
    BASE64_GENERATE
} GENERATE_TYPE;

HWND g_treebar = NULL;
HWND g_filetree = NULL;

static int img_drive;
static int img_fold;
static int img_close;
static int img_text;
static int img_general;
static int img_exe;
static WNDPROC filetree_wnd;
static WNDPROC treebar_wnd;

#define GET_UTF8_NAME(ptree, ptv, pathname, filename, new_name)                             \
char pathname[MAX_PATH] = {0};                                                              \
char filename[MAX_PATH] = {0};                                                              \
char new_name[MAX_PATH] = {0};                                                              \
do                                                                                          \
{                                                                                           \
    WideCharToMultiByte(CP_UTF8, 0, ptree->pathname, -1, pathname, MAX_PATH-1, NULL, NULL); \
    WideCharToMultiByte(CP_UTF8, 0, ptree->filename, -1, filename, MAX_PATH-1, NULL, NULL); \
    WideCharToMultiByte(CP_UTF8, 0, ptv->pszText, -1, new_name, MAX_PATH-1, NULL, NULL);    \
}                                                                                           \
while(0)

// 根据某个节点得到所有打开的子节点
static void CALLBACK
get_all_child(HTREEITEM hitem)
{
    HTREEITEM hchild = TreeView_GetChild(g_filetree, hitem);
    while (hchild)
    {
        tree_data *tvd = NULL;
        if ((tvd = on_treebar_get_treeview(hchild)) != NULL)
        {
            //释放所有内存
            eu_safe_free(tvd);
        }
        get_all_child(hchild);
        hchild = TreeView_GetNextItem(g_filetree, hchild, TVGN_NEXT);
    }
}

// 得到跟节点下的所有兄弟节点
static void
get_child_nodes(HTREEITEM hitem)
{
    HTREEITEM root = hitem;
    tree_data *tvd = NULL;
    if (!hitem)
    {
        root = TreeView_GetFirstVisible(g_filetree);
    }
    while (root)
    {
        get_all_child(root);
        if ((tvd = on_treebar_get_treeview(root)) != NULL)
        {
            eu_safe_free(tvd);
        }
        root = TreeView_GetNextSibling(g_filetree, root);
    }
}

// 手动销毁资源与内存
static void
destroy_filetree(void)
{
    if (g_filetree)
    {
        get_child_nodes(NULL);
        g_filetree = NULL;
    }
}

static size_t
write_remote_buffer(void *buffer, size_t size, size_t nmemb, void *stream)
{
    return 0;
}

static size_t
read_remote_buffer(void *buffer, size_t size, size_t nmemb, void *stream)
{
    remotefb *rfb = (remotefb *) stream;
    size_t transfer_len;
    size_t bytes_read;

    bytes_read = transfer_len = size * nmemb;
    if (bytes_read > rfb->remain_len)
    {
        bytes_read = rfb->remain_len;
    }
    memcpy(rfb->buf + rfb->str_len, buffer, bytes_read);
    rfb->str_len += bytes_read;
    rfb->remain_len -= bytes_read;
    return transfer_len;
}


static tree_data *
alloc_view_data(remotefs *server, TCHAR *pathname, TCHAR *filepath, TCHAR *filename, bool is_load, int img_index)
{
    tree_data *tvd = NULL;
    if ((tvd = (tree_data *) calloc(1, sizeof(tree_data))) != NULL)
    {
        tvd->server = server;
        _tcscpy(tvd->pathname, pathname);
        _tcscpy(tvd->filepath, filepath);
        _tcscpy(tvd->filename, filename);
        tvd->is_load = is_load;
        tvd->img_index = img_index;
    }
    return tvd;
}

static tree_data *
add_tree_node(HWND hwnd, HTREEITEM parent, int img_index, remotefs *server, TCHAR *pathname, TCHAR *filepath, TCHAR *filename, bool is_load)
{
    tree_data *tvd = alloc_view_data(server, pathname, filepath, filename, is_load, img_index);
    if (tvd != NULL)
    {
        TVITEM tvi = {TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_DI_SETITEM | TVIF_PARAM};
        tvi.iImage = tvi.iSelectedImage = img_index;
        tvi.pszText = tvd->filename;
        tvi.lParam = (LPARAM) tvd;
        TVINSERTSTRUCT tvis = {parent};
        tvis.item = tvi;
        tvis.hInsertAfter = (parent == TVI_ROOT ? TVI_LAST : TVI_SORT);
        tvd->hti = TreeView_InsertItem(hwnd, &tvis);
    }
    return tvd;
}

static void CALLBACK
set_curl_opt(CURL *curl)
{
    eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_remote_buffer);
#if APP_DEBUG
    eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
}

static bool
my_split(const char *str, char **sp)
{
    if ((*sp = strchr(str, '/')))
    {
        if ((*sp = strchr(*sp + 1, '/')))
        {
            *sp = strchr(*sp + 1, '/');
            return (*sp != NULL);
        }
    }
    return false;
}

static int
on_node_rename(TVITEM *ptvi)
{
    tree_data *old = (tree_data *) GetWindowLongPtr(g_filetree, GWLP_USERDATA);
    if (!old)
    {
        printf("GetWindowLongPtr(g_filetree) failed\n");
        return EUE_POINT_NULL;
    }
    if (old->server == NULL)
    {
        TCHAR new_name[MAX_PATH + 1];
        _sntprintf(new_name, MAX_PATH, _T("%s/%s"), old->pathname, ptvi->pszText);
        if (!MoveFile(old->filepath, new_name))
        {
            printf("rename [%ls] to [%ls] failed!\n", old->filepath, new_name);
            return EUE_MOVE_FILE_ERR;
        }
    }
    else
    {
        char *p = NULL;
        CURL *curl = NULL;
        char postquote[MAX_PATH + 1];
        struct curl_slist *headerlist = NULL;
        GET_UTF8_NAME(old, ptvi, pathname, filename, new_name);
        if (!(curl = on_remote_init_socket(pathname, old->server)))
        {
            return EUE_CURL_INIT_FAIL;
        }
        if (!my_split(pathname, &p))
        {
            return EUE_PARSE_FILE_ERR;
        }
        if (old->server->accesss == 0)
        {
            p = strchr(p + 1, '/');
        }
        if (!p)
        {
            printf("maybe path error\n");
        }
        if (old->server->accesss == 0)
        {
            _snprintf(postquote, MAX_PATH, "rename %s%s %s%s", p + 1, filename, p + 1, new_name);
        }
        else
        {
            _snprintf(postquote, MAX_PATH, "rename /%s%s /%s%s", p + 1, filename, p + 1, new_name);
        }
        headerlist = eu_curl_slist_append(headerlist, postquote);
        eu_curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        eu_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        eu_curl_slist_free_all(headerlist);
        if (res != CURLE_OK)
        {
            printf("rename [%s] to [%s] failed, cause:%s\n", filename, new_name, eu_curl_easy_strerror(res));
            return EUE_CURL_NETWORK_ERR;
        }
    }
    if (true)
    {
        TreeView_SetItem(g_filetree, ptvi);
        _tcsncpy(old->filename, ptvi->pszText, MAX_PATH - 1);
        _sntprintf(old->filepath, MAX_PATH - 1, _T("%s/%s"), old->pathname, ptvi->pszText);
        HTREEITEM hti_parent = TreeView_GetParent(g_filetree, ptvi->hItem);
        if (hti_parent)
        {
            on_treebar_refresh_node(hti_parent);
        }
    }
    return SKYLARK_OK;
}

HTREEITEM
on_treebar_get_path(tree_data **ptvd)
{
    HTREEITEM hti_select = TreeView_GetSelection(g_filetree);
    if (!hti_select)
    {
        return NULL;
    }
    if ((*ptvd = on_treebar_get_treeview(hti_select)) == NULL)
    {
        hti_select = NULL;
    }
    return hti_select;
}

static int
on_node_delete(void)
{
    HTREEITEM hti_select = NULL;
    tree_data *tvd = NULL;
    if ((hti_select = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    if (tvd->server == NULL)
    {
        BOOL m_del = FALSE;
        DWORD attr = GetFileAttributes(tvd->filepath);
        if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (attr & FILE_ATTRIBUTE_READONLY)
            {
                attr &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes(tvd->filepath, attr);
            }
            m_del = DeleteFile(tvd->filepath);
        }
        else
        {
            m_del = RemoveDirectory(tvd->filepath);
        }
        if (!m_del)
        {
            printf("delete node [%ls] failed, cause:%lu\n", tvd->filepath, GetLastError());
            return EUE_DELETE_FILE_ERR;
        }
    }
    else
    {
        char *p = NULL;
        CURL *curl = NULL;
        bool is_file = false;
        char postquote[MAX_PATH + 1];
        struct curl_slist *headerlist = NULL;
        char pathname[MAX_PATH] = {0};
        char filepath[MAX_PATH] = {0};
        WideCharToMultiByte(CP_UTF8, 0, tvd->pathname, -1, pathname, MAX_PATH-1, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, tvd->filepath, -1, filepath, MAX_PATH-1, NULL, NULL);
        if (!(curl = on_remote_init_socket(pathname, tvd->server)))
        {
            return EUE_CURL_INIT_FAIL;
        }
        if (!my_split(filepath, &p))
        {
            return EUE_PARSE_FILE_ERR;
        }
        if (tvd->server->accesss == 0)
        {
            p = strchr(p + 1, '/');
        }
        if (!p)
        {
            printf("maybe path error\n");
        }
        if (tvd->filepath[strlen(filepath) - 1] != '/')
        {
            is_file = true;
        }
        if (tvd->server->accesss == 0)
        {
            if (is_file)
            {
                snprintf(postquote, sizeof(postquote) - 1, "rm %.*s", strlen(p + 1), p + 1);
            }
            else
            {
                snprintf(postquote, sizeof(postquote) - 1, "rmdir %.*s", strlen(p + 1) - 1, p + 1);
            }
        }
        else
        {
            if (is_file)
            {
                snprintf(postquote, sizeof(postquote) - 1, "rm /%.*s", strlen(p + 1), p + 1);
            }
            else
            {
                snprintf(postquote, sizeof(postquote) - 1, "rmdir /%.*s", strlen(p + 1) - 1, p + 1);
            }
        }
        headerlist = eu_curl_slist_append(headerlist, postquote);
        eu_curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        eu_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        eu_curl_slist_free_all(headerlist);
        if (res != CURLE_OK)
        {
            printf("deletel [%s] failed, cause:%s", filepath, eu_curl_easy_strerror(res));
            return EUE_CURL_NETWORK_ERR;
        }
    }
    if (true)
    {
        TreeView_DeleteItem(g_filetree, hti_select);
        free(tvd);
        UpdateWindow(g_filetree);
    }
    return SKYLARK_OK;
}

static int
on_new_directory(void)
{
    HTREEITEM hti_select;
    tree_data *tvd = NULL;
    TCHAR dir_name[MAX_PATH] = {0};
    TCHAR full_dir_path[MAX_PATH];
    if ((hti_select = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    LOAD_I18N_RESSTR(IDC_MSG_DIR_SUB, m_sub);
    if (!eu_input(m_sub, dir_name, MAX_PATH))
    {
        return EUE_UNKOWN_ERR;
    }
    if (dir_name[0] == 0)
    {
        return EUE_PARSE_FILE_ERR;
    }
    else
    {
        _sntprintf(full_dir_path, MAX_PATH - 1, _T("%s/%s"), tvd->filepath, dir_name);
    }
    if (tvd->server == NULL)
    {
        if (!CreateDirectory(full_dir_path, NULL))
        {
            MSG_BOX(IDC_MSG_DIR_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_LOCAL_FILE_ERR;
        }
    }
    else
    {
        char *p = NULL;
        CURL *curl = NULL;
        char postquote[MAX_PATH];
        struct curl_slist *headerlist = NULL;
        char u8_dir[MAX_PATH] = {0};
        char filepath[MAX_PATH] = {0};
        WideCharToMultiByte(CP_UTF8, 0, dir_name, -1, u8_dir, MAX_PATH-1, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, tvd->filepath, -1, filepath, MAX_PATH-1, NULL, NULL);
        if (!(curl = on_remote_init_socket(filepath, tvd->server)))
        {
            printf("curl is null\n");
            return EUE_CURL_INIT_FAIL;
        }
        if (!my_split(filepath, &p))
        {
            return EUE_PARSE_FILE_ERR;
        }
        if (tvd->server->accesss == 0)
        {
            p = strchr(p + 1, '/');
        }
        if (!p)
        {
            printf("maybe path error\n");
        }
        if (tvd->server->accesss == 0)
        {
            snprintf(postquote, sizeof(postquote) - 1, "mkdir %s%s", p + 1, u8_dir);
        }
        else
        {
            snprintf(postquote, sizeof(postquote) - 1, "mkdir /%s%s", p + 1, u8_dir);
        }
        headerlist = eu_curl_slist_append(headerlist, postquote);
        eu_curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        eu_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        eu_curl_slist_free_all(headerlist);
        if (res != CURLE_OK)
        {
            MSG_BOX(IDC_MSG_DIR_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_CURL_NETWORK_ERR;
        }
    }
    if (true)
    {
        add_tree_node(g_filetree, hti_select, img_close, tvd->server ? tvd->server : NULL, tvd->filepath, full_dir_path, dir_name, false);
        UpdateWindow(g_filetree);
    }
    return SKYLARK_OK;
}

static int
create_new_file(void)
{
    HTREEITEM hti_select;
    tree_data *tvd = NULL;
    TCHAR filepath[MAX_PATH];
    TCHAR ac_file[MAX_PATH] = {0};
    tree_data *tvd_new = NULL;
    if ((hti_select = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    LOAD_I18N_RESSTR(IDC_MSG_MK_FILE, m_name);
    if (!eu_input(m_name, ac_file, sizeof(ac_file)))
    {
        return EUE_UNKOWN_ERR;
    }
    if (ac_file[0] == 0)
    {
        return EUE_UNKOWN_ERR;
    }
    else
    {
        _sntprintf(filepath, MAX_PATH-1, _T("%s/%s"), tvd->filepath, ac_file);
    }
    if (tvd->server == NULL)
    {
        FILE *fp = _tfopen(filepath, _T("r"));
        if (fp)
        {
            fclose(fp);
            MSG_BOX(IDC_MSG_EXIST_FILE, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_LOCAL_FILE_ERR;
        }
        fp = _tfopen(filepath, _T("w"));
        if (fp == NULL)
        {
            MSG_BOX(IDC_MSG_FILE_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_LOCAL_FILE_ERR;
        }
        fclose(fp);
    }
    else
    {
        CURL *curl = NULL;
        char url[MAX_PATH] = {0};
        char u8_file[MAX_PATH] = {0};
        char u8_filepath[MAX_PATH] = {0};
        WideCharToMultiByte(CP_UTF8, 0, ac_file, -1, u8_file, MAX_PATH-1, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, tvd->filepath, -1, u8_filepath, MAX_PATH-1, NULL, NULL);
        snprintf(url, sizeof(url) - 1, "%s%s", u8_filepath, u8_file);
        if (!(curl = on_remote_init_socket(url, tvd->server)))
        {
            printf("curl is null\n");
            return EUE_CURL_INIT_FAIL;
        }
        eu_curl_easy_setopt(curl, CURLOPT_READFUNCTION, write_remote_buffer);
        eu_curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_READDATA, NULL);
    #if defined(APP_DEBUG) && (APP_DEBUG > 0)
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        CURLcode res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
        if (res != CURLE_OK)
        {
            MSG_BOX(IDC_MSG_FILE_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_CURL_NETWORK_ERR;
        }
    }
    if (true)
    {
        tvd_new = add_tree_node(g_filetree, hti_select, img_text, tvd->server ? tvd->server : NULL, tvd->filepath, filepath, ac_file, true);
        TreeView_SelectItem(g_filetree, tvd_new->hti);
        UpdateWindow(g_filetree);
    }
    return SKYLARK_OK;
}

static int
on_file_copy(void)
{
    HTREEITEM hti_select;
    tree_data *tvd = NULL;
    TCHAR new_file_name[MAX_PATH] = {0};
    TCHAR new_path_name[MAX_PATH];
    if ((hti_select = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    _tcscpy(new_file_name, tvd->filename);
    LOAD_I18N_RESSTR(IDC_MSG_MK_FILE, m_input);
    if (!eu_input(m_input, new_file_name, MAX_PATH))
    {
        return EUE_UNKOWN_ERR;
    }
    if (*new_file_name == 0)
    {
        return EUE_UNKOWN_ERR;
    }
    if (tvd->server == NULL)
    {
        _sntprintf(new_path_name, MAX_PATH - 1, _T("%s/%s"), tvd->pathname, new_file_name);
        if (!CopyFile(tvd->filepath, new_path_name, true))
        {
            MSG_BOX(IDC_MSG_COPY_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            return EUE_COPY_FILE_ERR;
        }
        else
        {
            tree_data *tvd_new =
                add_tree_node(g_filetree, TreeView_GetParent(g_filetree, hti_select), img_text, NULL, tvd->pathname, new_path_name, new_file_name, true);
            if (tvd_new)
            {
                TreeView_SelectItem(g_filetree, tvd_new->hti);
            }
        }
    }
    else
    {
        MSG_BOX(IDC_MSG_SFTP_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        return EUE_UNKOWN_ERR;
    }
    UpdateWindow(g_filetree);
    return SKYLARK_OK;
}

static int
append_file_child(HWND hwnd, tree_data *tpnode)
{
    WIN32_FIND_DATA st_file;
    HANDLE hfile;
    TCHAR filepath[MAX_PATH];
    TCHAR find_path[MAX_PATH];
    tree_data *tvd = NULL;
    _stprintf(find_path, _T("%s/*"), tpnode->filepath);
    hfile = FindFirstFile(find_path, &st_file);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        // we can skip some questions
        return 0;
    }
    do
    {
        if (_tcscmp(st_file.cFileName, _T(".")) == 0 || _tcscmp(st_file.cFileName, _T("..")) == 0)
        {
            continue;
        }
        if (st_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            _sntprintf(filepath, MAX_PATH - 1, _T("%s/%s"), tpnode->filepath, st_file.cFileName);
            tvd = add_tree_node(hwnd, tpnode->hti, img_close, NULL, tpnode->filepath, filepath, st_file.cFileName, false);
            if (tvd == NULL)
            {
                FindClose(hfile);
                return EUE_POINT_NULL;
            }
        }
        else
        {
            _sntprintf(filepath, MAX_PATH - 1, _T("%s/%s"), tpnode->filepath, st_file.cFileName);
            tvd = add_tree_node(hwnd, tpnode->hti, img_text, NULL, tpnode->filepath, filepath, st_file.cFileName, true);
            if (tvd == NULL)
            {
                FindClose(hfile);
                return EUE_POINT_NULL;
            }
        }
    } while (FindNextFile(hfile, &st_file));
    FindClose(hfile);
    tpnode->is_load = true;
    return SKYLARK_OK;
}

static int
load_drives_tree(HWND hwnd)
{
    TCHAR dri_name[MAX_PATH];
    TCHAR *pdri = NULL;
    size_t dri_len;
    tree_data *tvd = NULL;

    GetLogicalDriveStrings(MAX_PATH - 1, dri_name);
    pdri = dri_name;
    while (pdri[0])
    {
        dri_len = _tcslen(pdri);
        if (_tcschr(pdri, _T('\\')))
        {
            *(_tcschr(pdri, _T('\\'))) = '\0';
        }
        tvd = add_tree_node(hwnd, TVI_ROOT, img_drive, NULL, pdri, pdri, pdri, true);
        if (tvd == NULL)
        {
            return EUE_POINT_NULL;
        }
        if (append_file_child(hwnd, tvd))
        {
            break;
        }
        pdri += dri_len + 1;
    }
    return SKYLARK_OK;
}

static int
append_remote_child(tree_data *tpnode)
{
    char *p1 = NULL;
    char *p2 = NULL;
    char filetype;
    char u8_filepath[MAX_PATH];
    char u8_filename[MAX_PATH];
    tree_data *tvd = NULL;
    remotefb *rfb = NULL;
    CURL *curl = NULL;
    if (!(tpnode && tpnode->server))
    {
        return EUE_POINT_NULL;
    }
    rfb = (remotefb *) calloc(1, sizeof(remotefb));
    if (rfb == NULL)
    {
        printf("malloc error\n");
        return EUE_OUT_OF_MEMORY;
    }
    WideCharToMultiByte(CP_UTF8, 0, tpnode->filepath, -1, u8_filepath, MAX_PATH-1, NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, tpnode->filename, -1, u8_filename, MAX_PATH-1, NULL, NULL);
    printf("tpnode->server->networkaddr = %s, u8_filepath = %s, u8_filename = %s\n", tpnode->server->networkaddr, u8_filepath, u8_filename);
    if (!(curl = on_remote_init_socket(u8_filepath, tpnode->server)))
    {
        free(rfb);
        return EUE_CURL_INIT_FAIL;
    }
    rfb->remain_len = REMOTE_FILE_BUFFER - 2;
    eu_curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "ls");
    eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, rfb);
    eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    if (tpnode->server->curl_opt)
    {
        (tpnode->server->curl_opt)(curl);
    }
    CURLcode res = eu_curl_easy_perform(curl);
    eu_curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        MSG_BOX(IDC_MSG_ATTACH_FAIL, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        free(rfb);
        return EUE_CURL_NETWORK_ERR;
    }
    p1 = rfb->buf;
    while ((*p1))
    {
        p2 = strchr(p1 + 1, '\n');
        if (p2)
        {
            (*p2) = '\0';
            if (p2 - 1 > p1 && *(p2 - 1) == '\r')
            {
                *(p2 - 1) = '\0';
            }
        }
        else
        {
            p2 = p1 + strlen(p1);
        }
        filetype = (*p1);
        for (int n = 0; n < 8; n++)
        {
            /* 搜索直到非空格 */
            for (; (*p1); p1++)
            {
                if ((*p1) == ' ')
                {
                    break;
                }
            }
            if ((*p1) == '\0')
            {
                break;
            }
            /* 搜索直到空格 */
            for (; (*p1); p1++)
            {
                if ((*p1) != ' ')
                {
                    break;
                }
            }
            if ((*p1) == '\0')
            {
                break;
            }
        }
        if ((*p1) == ' ')
        {
            p1 = p2 + 1;
            continue;
        }
        /* 如果是sftp服务器, 可能需要做utf8转换 */
        memset(u8_filename, 0, sizeof(u8_filename));
        strncpy(u8_filename, p1, MAX_PATH - 1);
        if (strcmp(u8_filename, ".") == 0 || strcmp(u8_filename, "..") == 0)
        {
            p1 = p2 + 1;
            continue;
        }
        if (filetype == 'd')
        {
            TCHAR filepath[MAX_PATH] = {0};
            TCHAR filename[MAX_PATH] = {0};
            memset(u8_filepath, 0, sizeof(u8_filepath));
            WideCharToMultiByte(CP_UTF8, 0, tpnode->filepath, -1, u8_filepath, MAX_PATH-1, NULL, NULL);
            strncat(u8_filepath, u8_filename, MAX_PATH);
            strncat(u8_filepath, "/", MAX_PATH);
            MultiByteToWideChar(CP_UTF8, 0, u8_filepath, -1, filepath, MAX_PATH);
            MultiByteToWideChar(CP_UTF8, 0, u8_filename, -1, filename, MAX_PATH);
            tvd = add_tree_node(g_filetree, tpnode->hti, img_close, tpnode->server, tpnode->filepath, filepath, filename, false);
            if (tvd == NULL)
            {
                free(rfb);
                return -1;
            }
        }
        else if (filetype == '-')
        {
            TCHAR filepath[MAX_PATH] = {0};
            TCHAR filename[MAX_PATH] = {0};
            memset(u8_filepath, 0, sizeof(u8_filepath));
            WideCharToMultiByte(CP_UTF8, 0, tpnode->filepath, -1, u8_filepath, MAX_PATH-1, NULL, NULL);
            strncat(u8_filepath, u8_filename, MAX_PATH);
            MultiByteToWideChar(CP_UTF8, 0, u8_filepath, -1, filepath, MAX_PATH);
            MultiByteToWideChar(CP_UTF8, 0, u8_filename, -1, filename, MAX_PATH);
            tvd = add_tree_node(g_filetree, tpnode->hti, img_text, tpnode->server, tpnode->filepath, filepath, filename, true);
            if (tvd == NULL)
            {
                free(rfb);
                return EUE_POINT_NULL;
            }
        }
        p1 = p2 + 1;
    }
    free(rfb);
    tpnode->is_load = true;
    return SKYLARK_OK;
}

static bool
on_treebar_node_exist(remotefs *pserver)
{
    HTREEITEM hti_root = NULL;
    tree_data *tvd = NULL;
    if (pserver && (hti_root = TreeView_GetFirstVisible(g_filetree)))
    {
        while ((hti_root = TreeView_GetNextSibling(g_filetree, hti_root)))
        {
            if ((tvd = on_treebar_get_treeview(hti_root)) != NULL && tvd->server != NULL)
            {
                if (strcmp(tvd->server->servername, pserver->servername) == 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static int
load_fslist_tree(HWND hwnd)
{
    remotefs *pserver = NULL;
    list_for_each_entry(pserver, &list_server, remotefs, node_server)
    {
        if (!on_treebar_node_exist(pserver))
        {
            on_treebar_load_remote(hwnd, pserver);
        }
    }
    return SKYLARK_OK;
}

unsigned WINAPI
update_node_tree(void *lp)
{
    HTREEITEM hti_parent = (HTREEITEM) lp;
    tree_data *tpnode = NULL;
    HTREEITEM hti_child;
    int nret = SKYLARK_OK;
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    tpnode = on_treebar_get_treeview(hti_parent);
    if (tpnode->server && tpnode->img_index == img_drive && !tpnode->is_load)
    {
        tpnode->server->curl_opt = set_curl_opt;
        if (append_remote_child(tpnode))
        {
            return EUE_UNKOWN_ERR;
        }
        else
        {
            InvalidateRect(g_filetree, NULL, true);
            UpdateWindow(g_filetree);
        }
    }
    hti_child = TreeView_GetChild(g_filetree, hti_parent);
    while (hti_child)
    {
        tree_data *tvd_child = on_treebar_get_treeview(hti_child);
        if (!tvd_child->is_load)
        {
            if (tvd_child->server == NULL)
            {
                nret = append_file_child(g_filetree, tvd_child);
            }
            else
            {
                tvd_child->server->curl_opt = set_curl_opt;
                nret = append_remote_child(tvd_child);
            }
            if (nret)
            {
                break;
            }
            else
            {
                InvalidateRect(g_filetree, NULL, true);
                UpdateWindow(g_filetree);
            }
        }
        hti_child = TreeView_GetNextSibling(g_filetree, hti_child);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return nret;
}

static void
on_tree_expand(NMTREEVIEW *lpnmtv)
{
    if (lpnmtv && lpnmtv->action == 0x2)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, update_node_tree, (void *) lpnmtv->itemNew.hItem, 0, NULL));
    }
}

static int
on_node_dbclick(void)
{
    int err = SKYLARK_OK;
    HTREEITEM hti;
    tree_data *tvd = NULL;
    file_backup bak = {0};
    if ((hti = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    if (!(tvd && tvd->is_load))
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, update_node_tree, (void *) hti, 0, NULL));
        return 0;
    }
    if (tvd->img_index != img_text)
    {
        return 0;
    }
    if (tvd->server != NULL)
    {
        _tcscpy(bak.rel_path, tvd->filepath);
        err = (on_file_open_remote(tvd->server, &bak, true) >= 0 ? SKYLARK_OK : SKYLARK_NOT_OPENED);
    }
    else
    {
        _tcscpy(bak.rel_path, tvd->filepath);
        eu_wstr_replace(bak.rel_path, MAX_PATH, _T("/"), _T("\\"));
        err = (on_file_only_open(&bak, true) >= 0 ? SKYLARK_OK : SKYLARK_NOT_OPENED);
    }
    if (!err && TabCtrl_GetItemCount(g_tabpages) < 1)
    {   // 建立一个空白标签页
        err = on_file_new();
    }
    return err;
}

static size_t
write_file_back(void *buffer, size_t size, size_t nmemb, void *userdata)
{
    DWORD written = 0;
    if (!WriteFile((HANDLE)userdata, buffer, (DWORD)(size*nmemb), &written, NULL))
    {
        return 0;
    }
    return written;
}

static HANDLE
on_treebar_download_file(const TCHAR *path, remotefs *pserver, TCHAR *out_path)
{
    HANDLE pfile = NULL;
    CURL *curl = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    char *cnv = NULL;
    do
    {
        if ((cnv = eu_utf16_utf8(path, NULL)) == NULL)
        {
            break;
        }
        if (!(curl = on_remote_init_socket(cnv, pserver)))
        {
            break;
        }
        if ((pfile = util_mk_temp(out_path, NULL)) == INVALID_HANDLE_VALUE)
        {
            break;
        }
        eu_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_back);
        eu_curl_easy_setopt(curl, CURLOPT_WRITEDATA, pfile);
    #if APP_DEBUG
        eu_curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        eu_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90);
        res = eu_curl_easy_perform(curl);
        eu_curl_easy_cleanup(curl);
    } while(0);
    if (cnv)
    {
        free(cnv);
    }
    if (res != CURLE_OK)
    {
        MSG_BOX(IDC_MSG_ATTACH_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        if (pfile)
        {
            share_close(pfile);
            DeleteFile(out_path);
        }
    }
    return pfile;
}

static void
on_treebar_insert_edit(const TCHAR *ext, const char *str)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pnode && STR_NOT_NUL(ext) && STR_NOT_NUL(str))
    {
        sptr_t cur_pos = -1;
        if ((cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0)) < 0)
        {
            cur_pos = eu_sci_call(pnode, SCI_GETANCHOR, 0, 0);
        }
        if (cur_pos >= 0)
        {
            sptr_t len = (sptr_t)strlen(str) + 32;
            char *text = (char *)calloc(1, len + 1);
            if (text)
            {
                char *u8_ext = eu_utf16_utf8(ext, NULL);
                _snprintf(text, len, "data:image/%s;base64,%s", STR_NOT_NUL(u8_ext) ? u8_ext : "jpeg", str);
                eu_sci_call(pnode, SCI_INSERTTEXT, cur_pos, (sptr_t)text);
                eu_sci_call(pnode, SCI_GOTOPOS, cur_pos, 0);
                free(text);
                eu_safe_free(u8_ext);
            }
        }
    }
}

static int
on_node_generate_enc(GENERATE_TYPE type)
{
    int err = 0;
    HTREEITEM hti;
    tree_data *tvd = NULL;
    TCHAR *p = NULL, *out = NULL;
    char *u8_out = NULL;
    TCHAR temp_path[MAX_PATH+1] = {0};
    if ((hti = on_treebar_get_path(&tvd)) == NULL || tvd == NULL)
    {
        return EUE_POINT_NULL;
    }
    if (tvd->server)
    {
        
        HANDLE pfile = on_treebar_download_file(tvd->filepath, tvd->server, temp_path);
        if (!pfile)
        {
            return EUE_LOCAL_FILE_ERR;
        }
        share_close(pfile);
        p = temp_path;
    }
    else
    {
        p = tvd->filepath;
    }
    switch (type) 
    {
        case MD5_GENERATE:
            err = util_file_md5(p, &out);
            break;
        case SHA1_GENERATE:
            err = util_file_sha1(p, &out);
            break;
        case SHA256_GENERATE:
            err = util_file_sha256(p, &out);
            break;
        case BASE64_GENERATE:
            err = util_file_base64(p, &u8_out);
            break;
        default:
            err = EUE_OPENSSL_ENC_ERR;
            break;
    }
    if (out || u8_out)
    {
        if (type != BASE64_GENERATE)
        {
            on_edit_push_clipboard(out);
        }
        else
        {
            on_treebar_insert_edit(util_path_ext(tvd->filename), (const char *)u8_out);
        }
        eu_safe_free(out);
        eu_safe_free(u8_out);
    }
    if (temp_path[0])
    {
        DeleteFile(temp_path);
    }
    return err;
}

unsigned WINAPI
flush_curl_thread(void *lp)
{
    HTREEITEM hti_parent = (HTREEITEM) lp;
    tree_data *tvd = on_treebar_get_treeview(hti_parent);
    if (!tvd)
    {
        return EUE_POINT_NULL;
    }
    else
    {
        tvd->server->curl_opt = set_curl_opt;
    }
    if (append_remote_child(tvd))
    {
        return EUE_CURL_NETWORK_ERR;
    }
    return update_node_tree(hti_parent);
}

int
on_treebar_refresh_node(HTREEITEM hti_parent)
{
    int nret = SKYLARK_OK;
    HTREEITEM hti_first;
    tree_data *tpnode = NULL;
    if (!hti_parent)
    {
        return EUE_POINT_NULL;
    }
    tpnode = on_treebar_get_treeview(hti_parent);
    while (1)
    {
        tree_data *tvd = NULL;
        hti_first = TreeView_GetChild(g_filetree, hti_parent);
        if (hti_first == NULL) break;
        tvd = on_treebar_get_treeview(hti_first);
        free(tvd);
        TreeView_DeleteItem(g_filetree, hti_first);
    }
    if (tpnode->server == NULL)
    {
        nret = append_file_child(g_filetree, tpnode);
        if (!nret)
        {
            nret = update_node_tree(hti_parent);
        }
    }
    else
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, flush_curl_thread, (void *) hti_parent, 0, NULL));
    }
    return nret;
}

static int
on_refresh_tree(HWND hwnd)
{
    HTREEITEM hti_parent;
    TVHITTESTINFO tvhti;

    GetCursorPos(&tvhti.pt);
    ScreenToClient(hwnd, &tvhti.pt);
    tvhti.flags = 0;
    TreeView_HitTest(hwnd, &tvhti);
    if (tvhti.flags == 0x400)
    {
        tvhti.pt.x -= 100;
        TreeView_HitTest(hwnd, &tvhti);
    }
    hti_parent = TreeView_GetParent(g_filetree, tvhti.hItem);
    if (hti_parent == NULL)
    {
        hti_parent = TreeView_GetFirstVisible(g_filetree);
        while (hti_parent)
        {
            TreeView_SelectItem(hwnd, hti_parent);
            on_treebar_refresh_node(hti_parent);
            Sleep(200);
            hti_parent = TreeView_GetNextSibling(g_filetree, hti_parent);
        }
    }
    else
    {
        TreeView_SelectItem(hwnd, hti_parent);
        on_treebar_refresh_node(hti_parent);
    }
    return SKYLARK_OK;
}

static int
on_search_tree(HWND hwnd)
{
    HTREEITEM hti_select = NULL;
    tree_data *tvd = NULL;
    if ((hti_select = on_treebar_get_path(&tvd)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    if (tvd->server != NULL)
    {
        MSG_BOX(IDC_MSG_SEARCH_ERR13, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        return EUE_UNKOWN_ERR;
    }
    TCHAR path[MAX_PATH+1] = {0};
    _tcscpy(path, tvd->filepath);
    eu_wstr_replace(path, MAX_PATH, _T("/"), _T("\\"));
    if (!eu_exist_dir(path))
    {
        eu_suffix_strip(path);
    }
    return on_search_file_thread(path);
}

void
on_treebar_update_addr(remotefs *pserver)
{
    HTREEITEM hti_root = NULL;
    tree_data *tvd = NULL;
    if (!pserver)
    {
        return;
    }
    if (!(hti_root = TreeView_GetFirstVisible(g_filetree)))
    {
        return;
    }
    while ((hti_root = TreeView_GetNextSibling(g_filetree, hti_root)))
    {
        if ((tvd = on_treebar_get_treeview(hti_root)) != NULL && tvd->server != NULL)
        {
            if (strcmp(tvd->server->servername, pserver->servername) == 0)
            {
                TreeView_DeleteItem(g_filetree, hti_root);
                on_treebar_load_remote(g_filetree , pserver);
                break;
            }
        }
    }
}

bool
on_treebar_variable_initialized(HWND *pd)
{
    int i = 60;
    while (!*pd && i--)
    {
        SleepEx(100, false);
    }
    return (*pd != NULL);
}

static unsigned __stdcall
on_treebar_wait_thread(void *lp)
{
    return on_treebar_variable_initialized(&g_filetree);
}

void
on_treebar_wait_hwnd(void)
{
    HANDLE thread = (HANDLE) _beginthreadex(NULL, 0, on_treebar_wait_thread, NULL, 0, NULL);
    if (thread)
    {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

static void
on_filetree_menu_callback(HMENU hpop, void *param)
{
    int number = eu_int_cast((intptr_t)param);
    if (hpop)
    {
        util_enable_menu_item(hpop, IDM_RENAME_DIRECTORY, number != img_drive);
        util_enable_menu_item(hpop, IDM_DELETE_DIRECTORY, number != img_drive);
    }
}

static void
on_filetree_menu_callback2(HMENU hpop, void *param)
{
    tree_data *tvd = (tree_data *)param;
    if (hpop && tvd)
    {
        const TCHAR *ext = util_path_ext(tvd->filename);
        bool ssl = eu_exist_libssl();
        bool enable = !(_tcsicmp(ext, _T("jpg")) && _tcsicmp(ext, _T("jpeg")) && _tcsicmp(ext, _T("gif")) && _tcsicmp(ext, _T("png")) && 
                        _tcsicmp(ext, _T("bmp")) && _tcsicmp(ext, _T("ico")) && _tcsicmp(ext, _T("webp")) && _tcsicmp(ext, _T("svg")));
        util_enable_menu_item(hpop, IDM_FILE_MD5_CLIP, ssl);
        util_enable_menu_item(hpop, IDM_FILE_SHA1_CLIP, ssl);
        util_enable_menu_item(hpop, IDM_FILE_SHA256_CLIP, ssl);
        util_enable_menu_item(hpop, IDM_PIC_CONVERT_BASE64, enable && ssl);
    }
}

LRESULT CALLBACK
filetree_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_NOTIFY:
        {
            NMHDR *lpnmhdr = (NMHDR *) lParam;
            NMTREEVIEW *lpnmtv = (NMTREEVIEW *) lParam;
            switch (lpnmhdr->code)
            {
                case TVN_ITEMEXPANDING:
                {
                    POINT pt = {0};
                    TVHITTESTINFO tvhti;
                    TVITEM item = lpnmtv->itemNew;
                    item.stateMask = TVIS_STATEIMAGEMASK;
                    GetCursorPos(&pt);
                    ScreenToClient(hwnd, &pt);
                    tvhti.pt.x = pt.x;
                    tvhti.pt.y = pt.y;
                    tvhti.flags = 0;
                    TreeView_HitTest(hwnd, &tvhti);
                    if (tvhti.flags & TVHT_ONITEMBUTTON)
                    {
                        TreeView_SelectItem(hwnd, tvhti.hItem);
                    }
                    on_tree_expand(lpnmtv);
                    if (lpnmtv->action == TVE_COLLAPSE)
                    {
                        if ((item.iSelectedImage) != img_drive)
                        {
                            item.iSelectedImage = img_close;
                        }
                    }
                    else if (lpnmtv->action == TVE_EXPAND)
                    {
                        if ((item.iSelectedImage) != img_drive)
                        {
                            item.iSelectedImage = img_fold;
                        }
                    }
                    TreeView_SetItem(hwnd, &item);
                }
                break;
                case NM_DBLCLK:
                    on_node_dbclick();
                    break;
                case TVN_BEGINLABELEDIT:
                {
                    LPNMTVDISPINFO pinfo = (LPNMTVDISPINFO) lpnmhdr;
                    tree_data *old_tvd = on_treebar_get_treeview(pinfo->item.hItem);
                    if (!old_tvd)
                    {
                        break;
                    }
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (uintptr_t) old_tvd);
                }
                break;
                case TVN_ENDLABELEDIT:
                {
                    LPNMTVDISPINFO pinfo = (LPNMTVDISPINFO) lpnmhdr;
                    if (pinfo->item.pszText)
                    {
                        on_node_rename(&pinfo->item);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case TVI_LOADREMOTE:
        {
            load_fslist_tree(hwnd);
            break;
        }
        case WM_COMMAND:
        {
            uint16_t wid = LOWORD(wParam);
            switch (wid)
            {
                case IDM_RELOAD_FILETREE:
                    on_refresh_tree(hwnd);
                    break;
                case IDM_RELOAD_FILESEARCH:
                    on_search_tree(hwnd);
                    break;
                case IDM_CREATE_SUB_DIRECTORY:
                    on_new_directory();
                    break;
                case IDM_CREATE_FILE:
                    create_new_file();
                    break;
                case IDM_COPY_FILE:
                    on_file_copy();
                    break;
                case IDM_RENAME_DIRECTORY:
                case IDM_RENAME_FILE:
                    TreeView_EditLabel(hwnd, TreeView_GetSelection(hwnd));
                    break;
                case IDM_DELETE_DIRECTORY:
                case IDM_DELETE_FILE:
                    on_node_delete();
                    break;
                case IDM_FILE_MD5_CLIP:
                    on_node_generate_enc(MD5_GENERATE);
                    break;
                case IDM_FILE_SHA1_CLIP:
                    on_node_generate_enc(SHA1_GENERATE);
                    break;
                case IDM_FILE_SHA256_CLIP:
                    on_node_generate_enc(SHA256_GENERATE);
                    break;
                case IDM_PIC_CONVERT_BASE64:
                    on_node_generate_enc(BASE64_GENERATE);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT pt = {GET_X_LPARAM(lParam), pt.y = GET_Y_LPARAM(lParam)};
            TVHITTESTINFO tvhti = {0};
            tree_data *tvd = NULL;
            memcpy(&(tvhti.pt), &pt, sizeof(POINT));
            TreeView_HitTest(hwnd, &tvhti);
            if (tvhti.flags & TVHT_ONITEM)
            {
                TreeView_SelectItem(hwnd, tvhti.hItem);
                tvd = on_treebar_get_treeview(tvhti.hItem);
                if (!tvd)
                {
                    break;
                }
                if (tvd->img_index == img_drive || tvd->img_index == img_fold || tvd->img_index == img_close)
                {
                    menu_pop_track(hwnd, IDR_FILETREE_DIR_POPUPMENU, 0, -1, on_filetree_menu_callback, (void *)(intptr_t)tvd->img_index);
                }
                else
                {
                    menu_pop_track(hwnd, IDR_FILETREE_FILE_POPUPMENU, 0, -1, on_filetree_menu_callback2, tvd);
                }
            }
            else if (tvhti.flags > 0x1 && tvhti.flags < 0x41)
            {
                return menu_pop_track(hwnd, IDR_FILETREE_POPUPMENU, 0, -1, NULL, NULL);
            }
            break;
        }
        case WM_DESTROY:
        {
            HIMAGELIST img_list = TreeView_GetImageList(hwnd, TVSIL_NORMAL);
            if (img_list != NULL)
            {
                ImageList_Destroy(img_list);
            }
            destroy_filetree();
            printf("filetree WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(filetree_wnd, hwnd, message, wParam, lParam);
}

static HIMAGELIST
load_tree_imglist(HWND hwnd)
{
    HIMAGELIST himl; // handle to image list
    HICON hicon;     // handle to icon
    HINSTANCE hinst = eu_module_handle();
    if ((himl = ImageList_Create(16, 16, ILC_COLOR32, 6, 0)) == NULL)
    {
        return NULL;
    }
    // Add the open file, closed file, and document bitmaps.
    hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDB_DRIVE));
    img_drive = ImageList_AddIcon(himl, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDB_OPENFOLD));
    img_fold = ImageList_AddIcon(himl, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDB_CLSDFOLD));
    img_close = ImageList_AddIcon(himl, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDB_TXT));
    img_general = ImageList_AddIcon(himl, hicon);
    DestroyIcon(hicon);

    hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDB_DOC));
    img_text = ImageList_AddIcon(himl, hicon);
    DestroyIcon(hicon);

    // Fail if not all of the images were added.
    if (ImageList_GetImageCount(himl) < 5)
    {
        ImageList_Destroy(himl);
        return NULL;
    }
    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hwnd, himl, TVSIL_NORMAL);
    return himl;
}

void
on_treebar_update_theme(void)
{
    SendMessage(g_filetree, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
    SendMessage(g_filetree, TVM_SETTEXTCOLOR, 0, eu_get_theme()->item.text.color);
    SendMessage(g_filetree, TVM_SETBKCOLOR, 0, eu_get_theme()->item.text.bgcolor);
    // 向控件发送消息, 要不然滚动条可能不会重绘
    on_dark_set_theme(g_filetree, on_dark_enable() ? L"DarkMode_Explorer" : L"", NULL);
}

LRESULT CALLBACK
treebar_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ERASEBKGND:
            if (!on_dark_enable())
            {
                break;
            }
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
        case WM_PAINT:
        {
            if (GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)
            {
                PAINTSTRUCT    ps;
                HDC hdc = BeginPaint(hwnd, & ps);
                HBRUSH hbr_bkgnd = (HBRUSH)on_dark_get_brush();
                // 绘制管理器标签
                HGDIOBJ old_font = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
                if (old_font)
                {
                    RECT rc;
                    TabCtrl_GetItemRect(hwnd, 0, &rc);
                    set_btnface_color(hdc, true);
                    set_text_color(hdc, true);
                    FrameRect(hdc, &rc, GetSysColorBrush(COLOR_3DDKSHADOW));
                    LOAD_I18N_RESSTR(IDC_MSG_EXPLORER, m_text);
                    if (STR_NOT_NUL(m_text))
                    {
                        DrawText(hdc, m_text, (int)_tcslen(m_text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    }
                    SelectObject(hdc, old_font);
                }
                EndPaint(hwnd, &ps);
            }
            break;
        }
        case WM_DPICHANGED:
        {
            printf("g_treebar WM_DPICHANGED\n");
            on_treebar_update_theme();
            break;
        }
        case WM_THEMECHANGED:
        {
            printf("g_treebar WM_THEMECHANGED\n");
            uintptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if (on_dark_enable())
            {
                style & TCS_OWNERDRAWFIXED ? (void)0 : SetWindowLongPtr(hwnd, GWL_STYLE, style | TCS_OWNERDRAWFIXED);
            }
            else if (style & TCS_OWNERDRAWFIXED)
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style & ~TCS_OWNERDRAWFIXED);
            }
            break;
        }
        case WM_DESTROY:
        {
            g_treebar = NULL;
            printf("g_treebar WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(treebar_wnd, hwnd, message, wParam, lParam);
}

int
on_treebar_create_box(HWND hwnd)
{
    TCITEM tci = {TCIF_TEXT};
    if (g_treebar)
    {
        DestroyWindow(g_treebar);
    }
    g_treebar = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD|TCS_FOCUSNEVER, 0, 0, 0, 0, hwnd, (HMENU) IDM_TREE_BAR, eu_module_handle(), NULL);
    if (g_treebar == NULL)
    {
        return EUE_POINT_NULL;
    }
    TabCtrl_SetPadding(g_treebar, 20, 5);
    LOAD_I18N_RESSTR(IDC_MSG_EXPLORER, m_text);
    tci.pszText = m_text;
    if (TabCtrl_InsertItem(g_treebar, 0, &tci) == -1)
    {
        printf("TabCtrl_InsertItem failed\n");
        DestroyWindow(g_treebar);
        return EUE_INSERT_TAB_FAIL;
    }
    if (!(treebar_wnd = (WNDPROC) SetWindowLongPtr(g_treebar, GWLP_WNDPROC, (LONG_PTR) treebar_proc)))
    {
        printf("SetWindowLongPtr(g_filetree) failed on %s\n", __FUNCTION__);
        DestroyWindow(g_treebar);
        return EUE_POINT_NULL;
    }
    SendMessage(g_treebar, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    return SKYLARK_OK;
}

int
on_treebar_create_dlg(HWND hwnd)
{
    int err = SKYLARK_OK;
    if (on_treebar_create_box(hwnd))
    {
        return EUE_POINT_NULL;
    }
    g_filetree = CreateWindow(WC_TREEVIEW,
                              NULL,
                              WS_CHILD | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_EDITLABELS,
                              0,
                              0,
                              0,
                              0,
                              hwnd,
                              NULL,
                              eu_module_handle(),
                              NULL);
    do
    {
        if (g_filetree == NULL)
        {
            err = EUE_POINT_NULL;
            break;
        }
        HIMAGELIST img_list = load_tree_imglist(g_filetree);
        if (!img_list)
        {
            err = EUE_POINT_NULL;
            break;
        }
        load_drives_tree(g_filetree);
        if (!(filetree_wnd = (WNDPROC) SetWindowLongPtr(g_filetree, GWLP_WNDPROC, (LONG_PTR) filetree_proc)))
        {
            err = EUE_POINT_NULL;
            break;
        }
        if (!on_splitter_init_treebar(hwnd))
        {
            err = EUE_POINT_NULL;
            break;
        }
        if (!on_splitter_init_symbar(hwnd))
        {
            err = EUE_POINT_NULL;
            break;
        }
        if (!on_splitter_init_editbar(hwnd))
        {
            err = EUE_POINT_NULL;
            break;
        }
        if (!on_splitter_init_tablebar(hwnd))
        {
            err = EUE_POINT_NULL;
            break;
        }
        on_treebar_update_theme();
    }while(0);
    if (err)
    {
        if (g_treebar)
        {
            DestroyWindow(g_treebar);
            g_treebar = NULL;
        }
        if (g_filetree)
        {
            DestroyWindow(g_filetree);
            g_filetree = NULL;
        }
    }
    return err;
}

void
on_treebar_adjust_box(RECT *ptf)
{
    RECT rect_main;
    GetClientRect(eu_module_hwnd(), &rect_main);
    if (!eu_get_config()->m_ftree_show)
    {
        ptf->left = 0;
        ptf->right = 0;
        ptf->top = rect_main.top + on_toolbar_height();
        ptf->bottom = rect_main.bottom;
    }
    else
    {
        ptf->left = rect_main.left;
        ptf->right = rect_main.left + eu_get_config()->file_tree_width;
        ptf->top = rect_main.top + on_toolbar_height();
        ptf->bottom = rect_main.bottom - on_statusbar_height();
    }
}

void
on_treebar_adjust_filetree(RECT *rect_filebar, RECT *rect_filetree)
{
    rect_filetree->left = FILETREE_MARGIN_LEFT;
    rect_filetree->right = rect_filebar->right - FILETREE_MARGIN_RIGHT;
    rect_filetree->top = rect_filebar->top + TABS_HEIGHT_DEFAULT + FILETREE_MARGIN_TOP;
    rect_filetree->bottom = rect_filebar->bottom - FILETREE_MARGIN_BOTTOM;
}

tree_data *
on_treebar_get_treeview(HTREEITEM hti)
{
    TVITEM tviChild = {0};
    if (!hti)
    {
        return NULL;
    }
    tviChild.mask = TVIF_HANDLE | TVIF_PARAM;
    tviChild.hItem = hti;
    if (!TreeView_GetItem(g_filetree, &tviChild))
    {
        return NULL;
    }
    tree_data *tvd = (tree_data *) (tviChild.lParam);
    return tvd;
}

int
on_treebar_load_remote(HWND hwnd, remotefs *pserver)
{
    TCHAR filepath[MAX_PATH] = {0};
    TCHAR networkaddr[MAX_PATH+1] = {0};
    TCHAR servername[100+1] = {0};
    if (!(pserver && *pserver->networkaddr))
    {
        return SKYLARK_OK;
    }
    else
    {
        MultiByteToWideChar(CP_UTF8, 0, pserver->networkaddr, -1, networkaddr, MAX_PATH);
        MultiByteToWideChar(CP_UTF8, 0, pserver->servername, -1, servername, 100);
    }
    if (pserver->accesss == 0)
    {
        _sntprintf(filepath, MAX_PATH - 1, _T("sftp://%s:%d/~/"), networkaddr, pserver->port);
    }
    else
    {
        _sntprintf(filepath, MAX_PATH - 1, _T("sftp://%s:%d/"), networkaddr, pserver->port);
    }
    if (!add_tree_node(hwnd, TVI_ROOT, img_drive, pserver, filepath, filepath, servername, false))
    {
        return EUE_UNKOWN_ERR;
    }
    return SKYLARK_OK;
}

static int
locate_remote_path(TCHAR *pathname)
{
    TCHAR *m_dup = NULL;
    TCHAR *pdir = NULL;
    tree_data *tvd = NULL;
    HTREEITEM hti_child = NULL;
    HTREEITEM hti = NULL;
    if ((m_dup = _tcsdup(pathname)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    hti = TreeView_GetRoot(g_filetree);
    do
    {
        if ((tvd = on_treebar_get_treeview(hti)) == NULL)
        {
            break;
        }
        if (tvd->server == NULL)
        {
            continue;
        }
        if (!tvd->is_load)
        {
            if (flush_curl_thread(hti))
            {
                free(m_dup);
                return EUE_UNKOWN_ERR;
            }
        }
        if (tvd->server->servername[0])
        {
            TCHAR servername[100] = {0};
            MultiByteToWideChar(CP_UTF8, 0, tvd->server->servername, -1, servername, 100);
            if (_tcsicmp(tvd->filename, servername) == 0)
            {
                printf("tvd->filename = servername = %s\n", tvd->server->servername);
                break;
            }
        }
    } while ((hti = TreeView_GetNextSibling(g_filetree, hti)) != NULL);
    if (hti)
    {
        hti_child = TreeView_GetChild(g_filetree, hti);
        pdir = _tcstok(m_dup+7, _T("/"));
        if (pdir)
        {
            pdir = _tcstok(NULL, _T("/"));
        }
        if (pdir && _tcsicmp(pdir, _T("~")) == 0)
        {
            pdir = _tcstok(NULL, _T("/"));
        }
    }
    while (hti_child && pdir)
    {
        hti = hti_child;
        do
        {
            if ((tvd = on_treebar_get_treeview(hti)) == NULL)
            {
                break;
            }
            if (_tcsicmp(tvd->filename, pdir) == 0)
            {
                break;
            }
            hti = TreeView_GetNextSibling(g_filetree, hti);
        } while (hti);
        if (hti)
        {
            hti_child = TreeView_GetChild(g_filetree, hti);
        }
        pdir = _tcstok(NULL, _T("/"));
    }
    if (pdir == NULL)
    {
        eu_get_config()->m_ftree_show = true;
        TreeView_SelectItem(g_filetree, hti);
        TreeView_EnsureVisible(g_filetree, hti);
        SendMessage(g_filetree, WM_SETFOCUS, 0, 0);
        eu_window_resize(eu_module_hwnd());
    }
    free(m_dup);
    return SKYLARK_OK;
}

int
on_treebar_locate_path(TCHAR *pathname)
{
    TCHAR *m_dup = NULL;
    TCHAR *pdir = NULL;
    tree_data *tvd = NULL;
    HTREEITEM hti = NULL, hti_child = NULL;
    if (url_has_remote(pathname))
    {
        return locate_remote_path(pathname);
    }
    if ((m_dup = _tcsdup(pathname)) == NULL)
    {
        return EUE_POINT_NULL;
    }
    hti_child = TreeView_GetRoot(g_filetree);
    pdir = _tcstok(m_dup, _T("\\/"));
    while (pdir)
    {
        hti = hti_child;
        do
        {
            tvd = on_treebar_get_treeview(hti);
            if (tvd == NULL)
            {
                break;
            }
            else if (!tvd->is_load)
            {
                if (append_file_child(g_filetree, tvd))
                {
                    free(m_dup);
                    printf("append_file_child failed\n");
                    return EUE_UNKOWN_ERR;
                }
            }
            if (_tcsicmp(tvd->filename, pdir) == 0)
            {
                break;
            }
            hti = TreeView_GetNextSibling(g_filetree, hti);
        } while (hti);
        if (hti == NULL)
        {
            break;
        }
        hti_child = TreeView_GetChild(g_filetree, hti);
        pdir = _tcstok(NULL, _T("\\/"));
    }
    if (pdir == NULL)
    {
        eu_get_config()->m_ftree_show = true;
        TreeView_SelectItem(g_filetree, hti);
        TreeView_EnsureVisible(g_filetree, hti);
        SendMessage(g_filetree, WM_SETFOCUS, 0, 0);
        eu_window_resize(eu_module_hwnd());
    }
    free(m_dup);
    return SKYLARK_OK;
}
