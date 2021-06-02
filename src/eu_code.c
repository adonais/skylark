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

static char *
get_database_config(char *out, int len, const char *str)
{
    const  char *CONNECTION_CONFIG =
    BEGIN_DATABASE_CONNECTION_CONFIG
        "%s"
        "-- DBTYPE : MySQL|Oracle|Sqlite3|PostgreSQL%s"
        "-- DBHOST : MySQL/PostgreSQL Server IP|ORACLESID%s"
        "-- DBPORT : MySQL/PostgreSQL Port%s"
        "-- DBUSER : MySQL/PostgreSQL User%s"
        "-- DBPASS : MySQL/PostgreSQL Password%s"
        "-- DBNAME : MySQL/PostgreSQL Database|Sqlite Path%s"
    END_DATABASE_CONNECTION_CONFIG
        "%s"
        "%s";
    _snprintf(out, len, CONNECTION_CONFIG, str, str, str, str, str, str, str, str, str);
    return out;
}

static char *
get_redis_config(char *out, int len, const char *str)
{
    const  char *CONNECTION_CONFIG =
    BEGIN_REDIS_CONNECTION_CONFIG
        "%s"
        "-- HOST : 127.0.0.1%s"
        "-- PORT : 6379%s"
        "-- PASS : DBPASS%s"
        "-- DBSL : 0%s"
    END_REDIS_CONNECTION_CONFIG
        "%s"
        "%s";
    _snprintf(out, len, CONNECTION_CONFIG, str, str, str, str, str, str, str);
    return out;
}

void
on_code_switch_fold(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t fold_line;
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t fold_level = eu_sci_call(pnode, SCI_GETFOLDLEVEL, line, 0);
        if (fold_level & SC_FOLDLEVELHEADERFLAG)
        {
            fold_line = line;
        }
        else
        {
            fold_line = eu_sci_call(pnode, SCI_GETFOLDPARENT, line, 0);
        }
        if (fold_line >= 0)
        {
            eu_sci_call(pnode, SCI_GOTOLINE, fold_line, 0);
            eu_sci_call(pnode, SCI_TOGGLEFOLD, fold_line, 0);
        }
    }
}

void
on_code_block_contract(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t fold_line;
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t fold_level = eu_sci_call(pnode, SCI_GETFOLDLEVEL, line, 0);
        if (fold_level & SC_FOLDLEVELHEADERFLAG)
        {
            fold_line = line;
        }
        else
        {
            fold_line = eu_sci_call(pnode, SCI_GETFOLDPARENT, line, 0);
        }
        if (fold_line >= 0)
        {
            bool is_expand = (bool) eu_sci_call(pnode, SCI_GETFOLDEXPANDED, fold_line, 0);
            if (is_expand)
            {
                eu_sci_call(pnode, SCI_GOTOLINE, fold_line, 0);
                eu_sci_call(pnode, SCI_FOLDLINE, fold_line, SC_FOLDACTION_CONTRACT);
            }
        }
    }
}

void
on_code_block_expand(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t fold_line;
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t fold_level = eu_sci_call(pnode, SCI_GETFOLDLEVEL, line, 0);
        if (fold_level & SC_FOLDLEVELHEADERFLAG)
        {
            fold_line = line;
        }
        else
        {
            fold_line = eu_sci_call(pnode, SCI_GETFOLDPARENT, line, 0);
        }
        if (fold_line >= 0)
        {
            bool is_expand = (bool) eu_sci_call(pnode, SCI_GETFOLDEXPANDED, fold_line, 0);
            if (!is_expand)
            {
                eu_sci_call(pnode, SCI_GOTOLINE, fold_line, 0);
                eu_sci_call(pnode, SCI_FOLDLINE, fold_line, SC_FOLDACTION_EXPAND);
            }
        }
    }
}

void
on_code_block_contract_all(eu_tabpage *pnode)
{
    eu_sci_call(pnode, SCI_FOLDALL, SC_FOLDACTION_CONTRACT, 0);
}

void
on_code_block_expand_all(eu_tabpage *pnode)
{
    eu_sci_call(pnode, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
}

void
on_code_block_complete(void)
{
    uint32_t state = GetMenuState(GetMenu(eu_module_hwnd()), IDM_SOURCEE_ENABLE_ACSHOW, MF_BYCOMMAND);
    if (state == -1)
    {
        return;
    }
    else if (state == MF_CHECKED)
    {
        util_set_menu_item(eu_module_hwnd(), IDM_SOURCEE_ENABLE_ACSHOW, false);
        eu_get_config()->m_acshow = false;
    }
    else
    {
        util_set_menu_item(eu_module_hwnd(), IDM_SOURCEE_ENABLE_ACSHOW, true);
        eu_get_config()->m_acshow = true;
    }
}

void
on_code_set_complete_chars(eu_tabpage *pnode)
{
    TCHAR input_chars[3] = {0};
    _sntprintf(input_chars, _countof(input_chars)-1, _T("%d"), eu_get_config()->acshow_chars);
    LOAD_I18N_RESSTR(IDC_MSG_AC_STR, ac_str);
    if (eu_input(ac_str, input_chars, _countof(input_chars)))
    {
        if (input_chars[0])
        {
            eu_get_config()->acshow_chars = _tstoi(input_chars);
            menu_update_all(eu_module_hwnd(), pnode);
        }
    }
}

void
on_code_block_calltip(void)
{
    uint32_t state = GetMenuState(GetMenu(eu_module_hwnd()), IDM_SOURCE_ENABLE_CTSHOW, MF_BYCOMMAND);
    if (state == -1)
    {
        return;
    }
    else if (state == MF_CHECKED)
    {
        util_set_menu_item(eu_module_hwnd(), IDM_SOURCE_ENABLE_CTSHOW, false);
        eu_get_config()->m_ctshow = false;
    }
    else
    {
        util_set_menu_item(eu_module_hwnd(), IDM_SOURCE_ENABLE_CTSHOW, true);
        eu_get_config()->m_ctshow = true;
    }
}

void
on_code_insert_config(eu_tabpage *pnode)
{
    if (pnode)
    {
        const char *eol_str = on_encoding_get_eol(pnode);
        char dbase_config[MAX_BUFFER+1] = {0};
        if (pnode->doc_ptr && pnode->doc_ptr->doc_type == DOCTYPE_SQL)
        {
            eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) get_database_config(dbase_config, MAX_BUFFER, eol_str));
        }
        else if (pnode->doc_ptr && pnode->doc_ptr->doc_type == DOCTYPE_REDIS)
        {
            eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) get_redis_config(dbase_config, MAX_BUFFER, eol_str));
        }
    }
}
