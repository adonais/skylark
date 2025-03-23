#include "framework.h"

uint32_t
on_profiles_used(const eu_tabpage *pnode)
{
    uint32_t ret = 0;
    if (pnode)
    {
        const size_t len = _tcslen(eu_config_path);
        if (_tcslen(pnode->pathname) == len + 1 && _tcsnicmp(pnode->pathname, eu_config_path, len) == 0)
        {
            wchar_t *t16 = NULL;
            if (_tcscmp(pnode->filename, L"skylark.conf") == 0)
            {
                ret = EU_CONFG_RELOAD;
            }
            else if (_tcscmp(pnode->filename, L"skylark_input.conf") == 0)
            {
                ret = EU_ACCEL_RELOAD;
            }
            else if (_tcscmp(pnode->filename, L"skylark_toolbar.conf") == 0)
            {
                ret = EU_TBBAR_RELOAD;
            }
            else if ((t16 = eu_utf8_utf16(eu_get_theme()->pathfile, NULL)) != NULL && (_tcsicmp(pnode->pathfile, t16) == 0))
            {
                ret = EU_THEME_RELOAD;
            }
            if (t16)
            {
                free(t16);
            }
        }
    }
    return ret;
}

bool
on_profiles_warn(void)
{
    int result = IDOK;
    MSG_BOX_SEL(IDS_PRIFILES_WARNING, IDC_MSG_WARN, MB_ICONSTOP | MB_OKCANCEL, result);
    return (result == IDOK);
}

int
on_profiles_reload(eu_tabpage *pnode, const uint32_t flag)
{
    int result = 0;
    if (pnode && flag > 0)
    {
        TCHAR *pdll = NULL;
        HWND hwnd = eu_hwnd_self();
        struct eu_config *pconf = eu_get_config();
        if (on_update_check())
        {
            on_update_cancel();
        }
        if (on_session_check())
        {
            on_session_cancel();
        }
        Sleep(EU_SESSION_INTERVAL);
        switch (flag)
        {
            case EU_CONFG_RELOAD:
            {
                char m_lang[QW_SIZE] = {0};
                strncpy(m_lang, pconf->m_language, QW_SIZE);
                if (on_config_lua_run(_T("eu_main.lua"), "+") && (pconf = eu_get_config()))
                {
                    eu_logmsg("Profile: reload skylark.conf ok\n");
                    if (strcmp(pconf->m_language, "auto") && strcmp(pconf->m_language, m_lang))
                    {
                        if (strcmp(pconf->m_language, "zh-cn.dll") || strcmp(pconf->m_language, "en-us.dll"))
                        {
                            if ((pdll = eu_utf8_utf16(pconf->m_language, NULL)))
                            {
                                result = i18n_locale_loader(hwnd, pdll);
                                free(pdll);
                            }
                        }
                    }
                    if ((pdll = eu_utf8_utf16(pconf->window_theme, NULL)))
                    {
                        result = on_view_theme_loader(hwnd, pdll, 0);
                        free(pdll);
                    }
                }
                else
                {
                    result = SKYLARK_CONF_FAILED;
                }
                break;
            }
            case EU_ACCEL_RELOAD:
            {
                eu_logmsg("Profile: we reload accel\n");
                result = on_config_accel_loader();
                break;
            }
            case EU_TBBAR_RELOAD:
            {
                HWND h_tool = NULL;
                if (pconf->m_toolbar && (h_tool = GetDlgItem(hwnd, IDC_TOOLBAR)))
                {
                    DestroyWindow(h_tool);
                    if (eu_config_load_toolbar())
                    {
                        result = on_toolbar_create_dlg(hwnd);
                    }
                }
                break;
            }
            case EU_THEME_RELOAD:
            {
                if (pconf && (pdll = eu_utf8_utf16(pconf->window_theme, NULL)))
                {
                    eu_logmsg("Profile: we reload theme\n");
                    result = on_view_theme_loader(hwnd, pdll, 0);
                    free(pdll);
                }
                break;
            }
            default:
            {
                break;
            }
        }
        on_session_run(UPCHECK_INDENT_MAIN);
    }
    return result;
}
