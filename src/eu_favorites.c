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

#define on_favorite_delete(_pdata)             \
TreeView_DeleteItem(g_filetree, _pdata->hti);  \
on_treebar_data_destoy(&_pdata)

static TCHAR *g_init_path = NULL;

static HTREEITEM
on_favorite_get_root(void)
{
    if (g_filetree)
    {
        return TreeView_GetRoot(g_filetree);
    }
    return NULL;
}

static void
on_favorite_set_node(HTREEITEM pitem, tree_data *tvd, const TCHAR *rname, const TCHAR *rpath, const TCHAR *rtag)
{
    TVITEM tvi = {TVIF_TEXT | TVIF_HANDLE | TVIF_PARAM};
    if (pitem && tvd && STR_NOT_NUL(rname))
    {
        free(tvd->filename);
        tvd->filename = _tcsdup(rname);
        EU_VERIFY(tvd->filename != NULL);
        if (STR_NOT_NUL(rpath))
        {
            free(tvd->filepath);
            tvd->filepath = _tcsdup(rpath);
            EU_VERIFY(tvd->filepath != NULL);
        }
        if (STR_NOT_NUL(rtag))
        {
            free(tvd->tagdesc);
            tvd->tagdesc = _tcsdup(rtag);
            EU_VERIFY(tvd->tagdesc != NULL);
        }
        tvi.hItem = pitem;
        tvi.pszText = tvd->filename;
        tvi.lParam = (LPARAM) tvd;
        TreeView_SetItem(g_filetree, &tvi);
    }
}

static tree_data *
on_favorite_get_child(HTREEITEM hitem, const int img_index)
{
    tree_data *tvd = NULL;
    HTREEITEM hchild = TreeView_GetChild(g_filetree, hitem);
    while (hchild)
    {
        if ((tvd = on_treebar_get_treeview(hchild)) != NULL && tvd->img_index == img_index)
        {
            return tvd;
        }
        hchild = TreeView_GetNextSibling(g_filetree, hchild);
    }
    return NULL;
}

static int
on_favorite_get_count(void)
{
    int result = 0;
    HTREEITEM root = on_favorite_get_root();
    HTREEITEM hchild = TreeView_GetChild(g_filetree, root);
    while (hchild)
    {
        ++result;
        hchild = TreeView_GetNextSibling(g_filetree, hchild);
    }
    return result;
}

void
on_favorite_reload_root(void)
{
    HTREEITEM root = on_favorite_get_root();
    if (root)
    {
        TCHAR *pname = NULL;
        tree_data *tvc = NULL;
        tree_data *tvd = on_treebar_get_treeview(root);
        if (tvd && tvd->filename && (pname = (TCHAR *)calloc(sizeof(TCHAR), MAX_LOADSTRING)) != NULL)
        {
            if (eu_i18n_load_str(IDS_USER_FAVORITES, pname, MAX_LOADSTRING - 1))
            {
                on_favorite_set_node(root, tvd, pname, NULL, NULL);
                if ((tvc = on_favorite_get_child(root, IMG_BULB)) != NULL)
                {
                    TCHAR *pzone = (TCHAR *)calloc(sizeof(TCHAR), MAX_LOADSTRING);
                    if (pzone && eu_i18n_load_str(IDC_MSG_HISTORY_ZERO, pzone, MAX_LOADSTRING - 1))
                    {
                        on_favorite_set_node(tvc->hti, tvc, pzone, NULL, NULL);
                    }
                }
                util_redraw(g_filetree, false);
            }
            free(pname);
        }
    }
}

static int
on_favorite_init_callback(void *data, int count, char **column, char **names)
{
    favorite_data *pbak = NULL;
    favorite_data favdata = {0};
    if (data)
    {
        pbak = *(favorite_data **)data;
    }
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szId"))
        {
            favdata.szid = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szName"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, favdata.szname, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, favdata.szpath, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szTag"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, favdata.sztag, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szGroup"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, favdata.szgroup, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szStatus"))
        {
            favdata.szstatus = atoi(column[i]);
        }
    }
    if (favdata.szname[0] || favdata.szpath[0])
    {
        cvector_push_back(pbak, favdata);
    }
    if (data && cvector_size(pbak))
    {
        *(favorite_data **)data = pbak;
    }
    return 0;
}

static void
on_favorite_batch(favorite_data *pvec, void *root)
{
    on_treebar_add_favorite((HTREEITEM)root, IMG_SHORTCUT, pvec);
}

static int
on_favorite_foreach_select(HTREEITEM hitem, favorite_data *pdata)
{
    int result = IDOK;
    tree_data *tvd = NULL;
    HTREEITEM hchild = TreeView_GetChild(g_filetree, hitem);
    while (hchild && pdata)
    {
        if ((tvd = on_treebar_get_treeview(hchild)) != NULL && STR_NOT_NUL(tvd->filepath))
        {
            if (_tcscmp(tvd->filepath, pdata->szpath) == 0)
            {
                on_favorite_delete(tvd);
                break;
            }
            else if (STR_NOT_NUL(tvd->filename) && _tcscmp(tvd->filename, pdata->szname) == 0)
            {
                MSG_BOX_SEL(IDS_FAVORITE_ADD_TIPS, IDC_MSG_TIPS, MB_ICONSTOP | MB_OKCANCEL, result);
                break;
            }
        }
        hchild = TreeView_GetNextSibling(g_filetree, hchild);
    }
    return result;
}

static void
on_favorite_add_null(HTREEITEM root)
{
    if (root)
    {
        favorite_data data = {0};
        if (eu_i18n_load_str(IDC_MSG_HISTORY_ZERO, data.szname, MAX_PATH - 1))
        {
            on_treebar_add_favorite(root, IMG_BULB, &data);
        }
    }
}

unsigned __stdcall
on_favorite_up_config(void *lp)
{
    HTREEITEM root = on_favorite_get_root();
    if (root)
    {
        cvector_vector_type(favorite_data) vinit = NULL;
        if (on_sql_favotrite_select(on_favorite_init_callback, &vinit) == SKYLARK_OK && cvector_size(vinit) > 0)
        {
            cvector_for_each_and_do(vinit, on_favorite_batch, (void *)root);
        }
        else
        {
            on_favorite_add_null(root);
        }
        cvector_free(vinit);
        util_redraw(g_filetree, false);
    }
    return 0;
}

static void
on_favorite_add_item(eu_tabpage *p)
{
    HTREEITEM root = on_favorite_get_root();
    if (root && p && !p->is_blank)
    {
        favorite_data data = {0};
        _sntprintf(data.szpath, MAX_BUFFER - 1, _T("%s"), p->pathfile);
        _sntprintf(data.szname, MAX_PATH - 1, _T("%s"), p->filename);
        _sntprintf(data.sztag, MAX_PATH - 1, _T("%s"), p->filename);
        if (data.szpath[0] && data.szname[0])
        {
            if (on_sql_favotrite_insert(&data) != SKYLARK_OK)
            {
                eu_logmsg("%s: on_sql_favotrite_insert return false\n", __FUNCTION__);
            }
            else
            {
                if (on_favorite_foreach_select(root, &data) == IDOK)
                {
                    on_treebar_add_favorite(root, IMG_SHORTCUT, &data);
                }
                tree_data *ptree = on_favorite_get_child(root, IMG_BULB);
                if (ptree)
                {
                    on_favorite_delete(ptree);
                }
            }
        }
    }
}

void
on_favorite_remove_node(void)
{
    tree_data *tvd = NULL;
    HTREEITEM hti_select = on_treebar_get_path(&tvd);
    if (hti_select && tvd && tvd->filepath)
    {
        if (on_sql_favotrite_delete(tvd->filepath) == SKYLARK_OK)
        {
             on_favorite_delete(tvd);
             if (on_favorite_get_count() < 1)
             {
                 on_favorite_add_null(on_favorite_get_root());
             }
        }
    }
}

void
on_favorite_add(eu_tabpage *pnode)
{
    if (g_tabpages && pnode)
    {
        eu_tabpage *p = NULL;
        cvector_vector_type(int) v = NULL;
        int num = on_tabpage_sel_number(&v, false);
        for (int i = 0; i < num; ++i)
        {
            eu_tabpage *p = on_tabpage_get_ptr(v[i]);
            if (p && !p->is_blank)
            {
                on_favorite_add_item(p);
            }
        }
        cvector_freep(&v);
    }
}

static int
on_favorite_pos(HWND hdlg)
{
    RECT rc;
    GetWindowRect(hdlg, &rc);
    if (eu_get_config() && eu_get_config()->file_tree_width)
    {
        const long left = eu_get_config()->file_tree_width + rc.left;
        return SetWindowPos(hdlg, 0, left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    return 1;
}

static void
on_favorite_edit_init(HWND hdlg, tree_data *tvd)
{
    if (hdlg && tvd)
    {
        HWND hname = GetDlgItem(hdlg, IDC_FAVORITE_NAME_EDIT);
        HWND hpath = GetDlgItem(hdlg, IDC_FAVORITE_PATH_EDIT);
        HWND htag = GetDlgItem(hdlg, IDC_FAVORITE_TAG_EDIT);
        if (hname && hpath && htag)
        {
            if (STR_NOT_NUL(tvd->filename))
            {
                Edit_SetText(hname, tvd->filename);
            }
            if (STR_NOT_NUL(tvd->filepath))
            {
                Edit_SetText(hpath, tvd->filepath);
                g_init_path = _tcsdup(tvd->filepath);
            }
            if (STR_NOT_NUL(tvd->tagdesc))
            {
                Edit_SetText(htag, tvd->tagdesc);
            }
        }
    }
}

static void
on_favorite_edit_save(HWND hdlg, tree_data *tvd)
{
    if (hdlg && tvd)
    {
        HWND hname = GetDlgItem(hdlg, IDC_FAVORITE_NAME_EDIT);
        HWND hpath = GetDlgItem(hdlg, IDC_FAVORITE_PATH_EDIT);
        HWND htag = GetDlgItem(hdlg, IDC_FAVORITE_TAG_EDIT);
        TCHAR *ptr_name = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH + 1);
        TCHAR *ptr_path = (TCHAR *)calloc(sizeof(TCHAR), MAX_BUFFER + 1);
        TCHAR *ptr_tag = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH + 1);
        if (hname && hpath && htag && ptr_name && ptr_path && ptr_tag)
        {
            Edit_GetText(hname, ptr_name, MAX_PATH);
            Edit_GetText(hpath, ptr_path, MAX_BUFFER);
            Edit_GetText(htag, ptr_tag, MAX_PATH);
            if (STR_NOT_NUL(ptr_name) && STR_NOT_NUL(ptr_path) && (url_has_remote(ptr_path) || eu_exist_file(ptr_path)))
            {
                favorite_data data = {0};
                _sntprintf(data.szpath, MAX_BUFFER - 1, _T("%s"), ptr_path);
                _sntprintf(data.szname, MAX_PATH - 1, _T("%s"), ptr_name);
                if (ptr_tag[0])
                {
                    _sntprintf(data.sztag, MAX_PATH - 1, _T("%s"), ptr_tag);
                }
                if (on_sql_favotrite_insert(&data) == SKYLARK_OK && STR_NOT_NUL(g_init_path))
                {
                    eu_logmsg("%s: on_sql_favotrite_insert ok!\n", __FUNCTION__);
                    if (_tcscmp(g_init_path, data.szpath) == 0)
                    {
                        on_favorite_set_node(tvd->hti, tvd, data.szname, data.szpath, data.sztag);
                    }
                    else
                    {
                        on_treebar_add_favorite(on_favorite_get_root(), IMG_SHORTCUT, &data);
                    }
                }
            }
        }
        eu_safe_free(ptr_name);
        eu_safe_free(ptr_path);
        eu_safe_free(ptr_tag);
    }
}

static INT_PTR CALLBACK
on_favorite_proc(HWND hdlg, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            if (on_dark_enable())
            {
                const int buttons[] = { IDOK, IDCANCEL };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            on_favorite_edit_init(hdlg, (tree_data *)lParam);
            return (INT_PTR)on_favorite_pos(hdlg);
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_SETTINGCHANGE:
        {
            if (on_dark_enable() && on_dark_color_scheme_change(lParam))
            {
                SendMessage(hdlg, WM_THEMECHANGED, 0, 0);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = { IDOK, IDCANCEL };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hdlg);
            }
            break;
        }
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            tree_data *tvd = NULL;
            switch (mid)
            {
                case IDOK:
                    if ((tvd = (tree_data *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_favorite_edit_save(hdlg, tvd);
                    }
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                case IDCANCEL:
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_CLOSE:
            eu_safe_free(g_init_path);
            eu_logmsg("%s: release the global pointer\n", __FUNCTION__);
            return (INT_PTR)EndDialog(hdlg, LOWORD(wParam));
        default:
            break;
    }
    return 0;
}

void
on_favorite_manager(void)
{
    tree_data *tvd = NULL;
    HTREEITEM hti_select = on_treebar_get_path(&tvd);
    if (hti_select && tvd)
    {
        i18n_dlgbox(eu_module_hwnd(), IDD_FAVORITE_DIALOG, on_favorite_proc, (LPARAM)tvd);
    }
    else
    {
        eu_logmsg("%s: error, point null\n", __FUNCTION__);
    }
}
