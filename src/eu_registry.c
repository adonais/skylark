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
#include <lm.h>
#include <objbase.h>
#include <shellapi.h>

#define EDIT_FILE    _T("skylark_file")
#define EDIT_BACKUP  _T("skylark_backup")
#define EDIT_DOC     _T("skylark Document")
#define SUPPORTED_LANG 10
#define EXT_LEN 27
#define NAME_MAX 18
#define EXT_NAME 32

static bool is_customize;

bool
on_reg_admin(void)
{
    BOOL result = FALSE;
    DWORD status = 0, ps_size = sizeof(PRIVILEGE_SET);
    HANDLE token = NULL;
    HANDLE impersonation_token = NULL;
    PRIVILEGE_SET ps = { 0 };
    GENERIC_MAPPING gm = { 0 };
    PACL pacl = NULL;
    PSID psid = NULL;
    PSECURITY_DESCRIPTOR sd = NULL;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    do
    {
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &token))
        {
            if (GetLastError() != ERROR_NO_TOKEN || !OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &token))
            {
                break;
            }
        }
        if (!DuplicateToken(token, SecurityImpersonation, &impersonation_token)) 
        {
            break;
        }
        if (!AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psid))
        {
            break;
        }
        sd = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (!sd || !InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
        {
            break;
        }
        DWORD acl_size = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + (size_t) GetLengthSid(psid) - sizeof(DWORD);
        pacl = (PACL) LocalAlloc(LPTR, (SIZE_T) acl_size);

        if (!pacl || !InitializeAcl(pacl, acl_size, ACL_REVISION2) ||
            !AddAccessAllowedAce(pacl, ACL_REVISION2, ACCESS_READ | ACCESS_WRITE, psid) ||
            !SetSecurityDescriptorDacl(sd, TRUE, pacl, FALSE))
        {
            break;
        }
        SetSecurityDescriptorGroup(sd, psid, FALSE);
        SetSecurityDescriptorOwner(sd, psid, FALSE);
        if (!IsValidSecurityDescriptor(sd))
        {
            break;
        }
        gm.GenericRead = ACCESS_READ;
        gm.GenericWrite = ACCESS_WRITE;
        gm.GenericExecute = 0;
        gm.GenericAll = ACCESS_READ | ACCESS_WRITE;
        if (!AccessCheck(sd, impersonation_token, ACCESS_READ, &gm, &ps, &ps_size, &status, &result))
        {
            result = FALSE;
            break;
        }
    } while (0);
    if (pacl)
    {
        LocalFree(pacl);
    }
    if (sd)
    {
        LocalFree(sd);
    }
    if (psid)
    {
        FreeSid(psid);
    }
    if (impersonation_token)
    {
        CloseHandle(impersonation_token);
    }
    if (token)
    {
        CloseHandle(token);
    }
    return result ? true : false;
}

bool
run_as_admin(int argc, TCHAR **argv)
{
    bool result = true;
    const HMODULE hlib = LoadLibrary(_T("ole32.dll"));
    if (!hlib)
    {
        return false;
    }
    typedef HRESULT(WINAPI * CoInitializePtr)(LPVOID pvReserved);
    typedef HRESULT(WINAPI * CoUninitializePtr)(void);
    const CoInitializePtr fnCoInitialize = (CoInitializePtr) GetProcAddress(hlib, "CoInitialize");
    const CoUninitializePtr fnCoUninitialize = (CoUninitializePtr) GetProcAddress(hlib, "CoUninitialize");
    if (!(fnCoInitialize && fnCoUninitialize))
    {
        FreeLibrary(hlib);
        return false;
    }
    const HRESULT hrComInit = fnCoInitialize(NULL);

    if ((hrComInit == RPC_E_CHANGED_MODE) || SUCCEEDED(hrComInit))
    {
        TCHAR *args = (TCHAR *) calloc(sizeof(TCHAR), MAX_PATH + 1);
        for (int i = 1; i < argc; i++)
        {
            _tcsncat(args, _T(" "), MAX_PATH);
            _tcsncat(args, argv[i], MAX_PATH);
        }
        SHELLEXECUTEINFO shex = { 0 };
        TCHAR path[MAX_PATH] = { 0 };
        TCHAR directory[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, path, _countof(path));
        _tcscpy(directory, path);
        TCHAR *p = _tcsrchr(directory, _T('\\'));
        if (p)
        {
            *p = _T('\0');
        }
        shex.cbSize = sizeof(shex);
        shex.fMask = SEE_MASK_NOZONECHECKS | SEE_MASK_FLAG_NO_UI;
        shex.lpVerb = _T("runas");
        shex.nShow = SW_NORMAL;
        shex.lpFile = path;
        shex.lpDirectory = directory;
        shex.lpParameters = args;
        if (ShellExecuteEx(&shex))
        {
            result = true;
        }
        else
        {
            result = false;
            printf("Run by administrative privileges failed. cause: %lu", GetLastError());
        }
        fnCoUninitialize();
    }
    FreeLibrary(hlib);
    return result;
}

static bool
check_reg_str(HKEY key, LPCTSTR txt)
{
    bool exist = false;
    HKEY regkey = NULL;
    LSTATUS lsret = ERROR_FILE_NOT_FOUND;
    TCHAR process[MAX_PATH] = {0};
    TCHAR sz_path[MAX_PATH] = {0};
    uint32_t dw_size = sizeof(sz_path);
    uint32_t dw_type = REG_SZ;    
    if (!GetModuleFileName(NULL, process, MAX_PATH))
    {
        return false;
    }
    lsret = RegOpenKeyEx(key, txt, 0, KEY_READ, &regkey);
    if (lsret == ERROR_SUCCESS)
    {
        lsret = RegQueryValueEx(regkey, NULL, 0, &dw_type, (uint8_t *) sz_path, &dw_size);
        if (lsret == ERROR_SUCCESS)
        {
            if (*sz_path == _T('"') && _tcslen(sz_path) > 1)
            {
                exist = _tcsnicmp(sz_path + 1, process, _tcslen(process)) == 0;
            }
            else
            {
                exist = _tcsnicmp(sz_path, process, _tcslen(process)) == 0;
            }
        }
        RegCloseKey(regkey);
    }
    return exist;
}

unsigned __stdcall
on_reg_update_menu(void *lp)
{
    HWND hwnd = (HWND)lp;
    if (!hwnd)
    {
        hwnd = eu_module_hwnd();
    }
    if (check_reg_str(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark\\command")))
    {
        util_set_menu_item(hwnd, IDM_ENV_FILE_POPUPMENU, true);
    }
    else
    {
        util_set_menu_item(hwnd, IDM_ENV_FILE_POPUPMENU, false);
    }
    if (check_reg_str(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2\\command")))
    {
        util_set_menu_item(hwnd, IDM_ENV_DIRECTORY_POPUPMENU, true);
    }
    else
    {
        util_set_menu_item(hwnd, IDM_ENV_DIRECTORY_POPUPMENU, false);
    }
    return 0;
}


int
eu_undo_file_popup(void)
{
    HKEY regkey_edit = NULL;
    HKEY regkey_cmd = NULL;
    LSTATUS lsret;
    TCHAR process[MAX_PATH] = {0};
    if (!GetModuleFileName(NULL, process, MAX_PATH))
    {
        return -1;
    }
    if (!check_reg_str(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark\\command")))
    {
        TCHAR cmd_str[MAX_PATH];
        lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark"), &regkey_edit);
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        LOAD_I18N_RESSTR(IDC_MSG_OPEN_FILE_EDIT, msg_edit);
        do
        {
            lsret = RegSetValue(regkey_edit, NULL, REG_SZ, msg_edit, (DWORD) sizeof(msg_edit));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR2, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetKeyValue(regkey_edit, NULL, _T("Icon"), REG_SZ, process, (DWORD) sizeof(process));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR3, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_edit);
            regkey_edit = NULL;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), 0, KEY_ALL_ACCESS, &regkey_edit) == ERROR_SUCCESS)
            {
                DWORD data = 0x29a;
                RegSetValueEx(regkey_edit, _T("MultipleInvokePromptMinimum"), 0, REG_DWORD, (LPBYTE) &data, sizeof(data));
            }
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark\\command"), &regkey_cmd);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            _sntprintf(cmd_str, MAX_PATH - 1, _T("\"%s\" \"%%1\""), process);
            lsret = RegSetValue(regkey_cmd, NULL, REG_SZ, cmd_str, (DWORD) sizeof(cmd_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            util_set_menu_item(eu_module_hwnd(), IDM_ENV_FILE_POPUPMENU, true);
        } while (0);
        if (regkey_edit)
        {
            RegCloseKey(regkey_edit);
        }
        if (regkey_cmd)
        {
            RegCloseKey(regkey_cmd);
        }
    }
    else
    {
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark\\command"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR6, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("*\\shell\\skylark"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR7, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        else
        {   /* 完成 */
            MSG_BOX(IDC_MSG_REGIST_ERR8, IDC_MSG_TIPS, MB_OK);
        }
        util_set_menu_item(eu_module_hwnd(), IDM_ENV_FILE_POPUPMENU, false);
    }
    return lsret;
}

int
on_reg_file_popup_menu(void)
{
    int ret = 1;
    if (on_reg_admin())
    {
        eu_undo_file_popup();
    }
    else if (strcmp(eu_get_config()->window_theme, "black") == 0)
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg1=1")};
        ret = !run_as_admin(2, argv);
    }
    else
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg1")};
        ret = !run_as_admin(2, argv);
    }
    return ret;
}

int
eu_undo_dir_popup(void)
{
    HKEY regkey_edit = NULL;
    HKEY regkey_cmd = NULL;
    LSTATUS lsret;
    TCHAR process[MAX_PATH] = { 0 };
    if (!GetModuleFileName(NULL, process, MAX_PATH))
    {
        return -1;
    }
    if (!check_reg_str(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2\\command")))
    {
        TCHAR cmd_str[MAX_PATH];
        TCHAR loc_str[MAX_LOADSTRING] = {0};
        LOAD_I18N_RESSTR(IDC_MSG_DIRECTORYFILES, ofile_str);
        /* Directory注册Edit */
        lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark"), &regkey_edit);
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR9, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        do
        {
            lsret = RegSetValue(regkey_edit, NULL, REG_SZ, ofile_str, (DWORD) sizeof(ofile_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR10, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetKeyValue(regkey_edit, NULL, _T("Icon"), REG_SZ, process, (DWORD) sizeof(process));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_edit);
            regkey_edit = NULL;
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark\\command"), &regkey_cmd);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            _sntprintf(cmd_str, _countof(cmd_str) - 1, _T("\"%s\" \"%%1\\*\""), process);
            lsret = RegSetValue(regkey_cmd, NULL, REG_SZ, cmd_str, (DWORD) sizeof(cmd_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR13, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_cmd);
            regkey_cmd = NULL;
            /* Directory注册EditUltra2 */
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2"), &regkey_edit);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR14, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            if (!eu_i18n_load_str(IDC_MSG_LOC_DIRECTORY, loc_str, 0))
            {
                break;
            }
            lsret = RegSetValue(regkey_edit, NULL, REG_SZ, loc_str, (DWORD) sizeof(loc_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR15, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }

            lsret = RegSetKeyValue(regkey_edit, NULL, _T("Icon"), REG_SZ, process, (DWORD) sizeof(process));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR16, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_edit);
            regkey_edit = NULL;
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2\\command"), &regkey_cmd);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR17, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            _sntprintf(cmd_str, _countof(cmd_str) - 1, _T("\"%s\" \"%%1\\\\\""), process);
            lsret = RegSetValue(regkey_cmd, NULL, REG_SZ, cmd_str, (DWORD) sizeof(cmd_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR18, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_cmd);
            regkey_cmd = NULL;
            /* Drive注册Edit */
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark"), &regkey_edit);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR19, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetValue(regkey_edit, NULL, REG_SZ, ofile_str, (DWORD) sizeof(ofile_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR20, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetKeyValue(regkey_edit, NULL, _T("Icon"), REG_SZ, process, (DWORD) sizeof(process));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR21, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_edit);
            regkey_edit = NULL;
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark\\command"), &regkey_cmd);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR22, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            _sntprintf(cmd_str, _countof(cmd_str) - 1, _T("\"%s\" \"%%1\\*\""), process);
            lsret = RegSetValue(regkey_cmd, NULL, REG_SZ, cmd_str, (DWORD) sizeof(cmd_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR23, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            RegCloseKey(regkey_cmd);
            regkey_cmd = NULL;
            /* Drive注册Edit2 */
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark2"), &regkey_edit);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR24, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetValue(regkey_edit, NULL, REG_SZ, loc_str, (DWORD) sizeof(loc_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR25, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegSetKeyValue(regkey_edit, NULL, _T("Icon"), REG_SZ, process, (DWORD) sizeof(process));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR26, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            lsret = RegCreateKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark2\\command"), &regkey_cmd);
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR27, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            memset(cmd_str, 0, sizeof(cmd_str));
            _sntprintf(cmd_str, _countof(cmd_str) - 1, _T("\"%s\" \"%%1\\\""), process);
            lsret = RegSetValue(regkey_cmd, NULL, REG_SZ, cmd_str, (DWORD) sizeof(cmd_str));
            if (lsret != ERROR_SUCCESS)
            {
                MSG_BOX(IDC_MSG_REGIST_ERR28, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            util_set_menu_item(eu_module_hwnd(), IDM_ENV_DIRECTORY_POPUPMENU, true);
        } while (0);
        if (regkey_edit)
        {
            RegCloseKey(regkey_edit);
        }
        if (regkey_cmd)
        {
            RegCloseKey(regkey_cmd);
        }
    }
    else
    {
        /* Directory注销Edit */
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark\\command"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR29, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR30, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        /* Directory注销Edit2 */
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2\\command"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR31, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Directory\\shell\\skylark2"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR32, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        /* Drive注销Edit */
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark\\command"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR33, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR34, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        /* Drive注销Edit2 */
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark2\\command"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR35, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        lsret = RegDeleteKey(HKEY_CLASSES_ROOT, _T("Drive\\shell\\skylark2"));
        if (lsret != ERROR_SUCCESS)
        {
            MSG_BOX(IDC_MSG_REGIST_ERR36, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        else
        {   /* 完成 */
            MSG_BOX(IDC_MSG_REGIST_ERR8, IDC_MSG_TIPS, MB_OK);
        }
        util_set_menu_item(eu_module_hwnd(), IDM_ENV_DIRECTORY_POPUPMENU, false);
    }
    return lsret;
}

int
on_reg_dir_popup_menu(void)
{
    int ret = 1;
    if (on_reg_admin())
    {
        eu_undo_dir_popup();
    }
    else if (strcmp(eu_get_config()->window_theme, "black") == 0)
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg2=1")};
        ret = !run_as_admin(2, argv);
    }
    else
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg2")};
        ret = !run_as_admin(2, argv);
    }
    return ret;
}

const TCHAR 
ext_array[SUPPORTED_LANG][EXT_LEN][NAME_MAX] =
{
    {_T("Text"),_T(".txt"), _T(".log")},
    {_T("ms ini/inf"),_T(".ini"), _T(".inf")},
    {_T("c, c++, objc"),
     _T(".h"), _T(".hh"), _T(".hpp"), _T(".hxx"), _T(".c"), _T(".cpp"), _T(".cxx"), _T(".cc"),
     _T(".m"), _T(".mm"),
     _T(".vcxproj"), _T(".vcproj"), _T(".props"), _T(".vsprops"), _T(".manifest")
    },
    {_T("java, c#, pascal"),_T(".java"), _T(".cs"), _T(".pas"), _T(".pp"), _T(".inc")},
    {_T("web script"),
     _T(".html"), _T(".htm"), _T(".shtml"), _T(".shtm"), _T(".hta"),
     _T(".asp"), _T(".aspx"),
     _T(".css"), _T(".js"), _T(".json"), _T(".jsm"), _T(".jsp"),
     _T(".php"), _T(".php3"), _T(".php4"), _T(".php5"), _T(".phps"), _T(".phpt"), _T(".phtml"),
     _T(".xml"), _T(".xhtml"), _T(".xht"), _T(".xul"), _T(".kml"), _T(".xaml"), _T(".xsml")
    },
    {_T("public script"),
     _T(".sh"), _T(".bsh"), _T(".bash"), _T(".bat"), _T(".cmd"), _T(".nsi"),
     _T(".nsh"), _T(".lua"), _T(".pl"), _T(".pm"), _T(".py")
    },
    {_T("property script"),_T(".rc"), _T(".as"), _T(".mx"), _T(".vb"), _T(".vbs")},
    {_T("fortran, TeX, SQL"),_T(".f"), _T(".for"), _T(".f90"), _T(".f95"), _T(".f2k"), _T(".tex"), _T(".sql")},
    {_T("misc"),_T(".nfo"), _T(".mak")},
    {_T("customize")}
};

static int 
get_sub_key(HKEY h_key)
{
    int m_sub;
    long result = RegQueryInfoKey(h_key, NULL, NULL, NULL, (LPDWORD)&m_sub, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    return (result == ERROR_SUCCESS)?m_sub:0;
}

static int 
get_sub_value(HKEY h_key)
{
    int m_value;
    long result = RegQueryInfoKey(h_key, NULL, NULL, NULL, NULL, NULL, NULL, (LPDWORD)&m_value, NULL, NULL, NULL, NULL);
    return (result == ERROR_SUCCESS)?m_value:0;
}
    
static void 
get_registered_exts(HWND hdlg)
{
    int m_sub = get_sub_key(HKEY_CLASSES_ROOT);
    for (int i = 0 ; i < m_sub ; ++i)
    {
        TCHAR ext_name[EXT_NAME];
        int ext_at_len = EXT_NAME;
        int res = RegEnumKeyEx(HKEY_CLASSES_ROOT, i, ext_name, (LPDWORD)(&ext_at_len), NULL, NULL, NULL, NULL);
        if ((res == ERROR_SUCCESS) && (ext_name[0] == '.'))
        {
            TCHAR data[EXT_NAME];
            DWORD data_len = EXT_NAME * sizeof(TCHAR);
            DWORD type;
            HKEY hkey_check;
            ext_at_len = EXT_NAME;
            RegOpenKeyEx(HKEY_CLASSES_ROOT, ext_name, 0, KEY_ALL_ACCESS, &hkey_check);
            RegQueryValueEx(hkey_check, _T(""), NULL, &type, (LPBYTE)(data), &data_len);
            if ((type == REG_SZ) && (!_tcscmp(data, EDIT_FILE)))
            {
                SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, (LPARAM)(ext_name));
            }
            RegCloseKey(hkey_check);
        }
    }
}

static void 
get_supported_exts(HWND hdlg)
{
    for (int i = 0 ; i < SUPPORTED_LANG ; ++i)
    {
        SendDlgItemMessage(hdlg, IDC_REGEXT_LANG_LIST, LB_ADDSTRING, 0, (LPARAM)(ext_array[i][0]));
    }
}

static void 
add_ext(TCHAR *ext)
{
    HKEY  h_key;
    DWORD dw_disp;
    long  ret;
    ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ext, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &h_key, &dw_disp);
    if (ret == ERROR_SUCCESS)
    {
        TCHAR data[MAX_PATH];
        DWORD data_len = MAX_PATH * sizeof(TCHAR);
        if (dw_disp == REG_OPENED_EXISTING_KEY)
        {
            int res = RegQueryValueEx(h_key, _T(""), NULL, NULL, (LPBYTE)(data), &data_len);
            if (res == ERROR_SUCCESS)
            {
                RegSetValueEx(h_key, EDIT_BACKUP, 0, REG_SZ, (LPBYTE)(data), data_len);
            }
        }
        RegSetValueEx(h_key, NULL, 0, REG_SZ, (const uint8_t *)(EDIT_FILE), (uint32_t)((_tcslen(EDIT_FILE) + 1) * sizeof(TCHAR)));
        RegCloseKey(h_key);
    }
}


static bool 
delete_exts(const TCHAR *ext_deleted)
{
    HKEY h_key;
    RegOpenKeyEx(HKEY_CLASSES_ROOT, ext_deleted, 0, KEY_ALL_ACCESS, &h_key);

    int m_value = get_sub_value(h_key);
    int m_key = get_sub_key(h_key);

    if ((m_value <= 1) && (!m_key))
    {
        TCHAR sub_key[32] = _T("\\");
        _tcsncat(sub_key, ext_deleted, 32);
        RegDeleteKey(HKEY_CLASSES_ROOT, sub_key);
    }
    else
    {
        TCHAR data[EXT_NAME];
        DWORD data_len = EXT_NAME*sizeof(TCHAR);
        DWORD type;
        int res = RegQueryValueEx(h_key, EDIT_BACKUP, NULL, &type, (LPBYTE)data, &data_len);
        if (res == ERROR_SUCCESS)
        {
            RegSetValueEx(h_key, NULL, 0, type, (LPBYTE)data, data_len);
            RegDeleteValue(h_key, EDIT_BACKUP);
        }
        else
        {
            RegDeleteValue(h_key, NULL);
        }
    }
    return true;
}

static void
write_edit_path(HWND hdlg)
{
    HKEY  h_key, hRootKey;
    DWORD dw_disp;
    long  ret;
    TCHAR reg_str[MAX_PATH];
    
    _sntprintf(reg_str, MAX_PATH-1, _T("%s%s"), EDIT_FILE, _T("\\shell\\open\\command"));
    ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, reg_str, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &h_key, &dw_disp);
        
    if (ret == ERROR_SUCCESS)
    {
        // Write the value for new document
        RegOpenKeyEx(HKEY_CLASSES_ROOT, EDIT_FILE, 0, KEY_ALL_ACCESS, &hRootKey);
        RegSetValueEx(hRootKey, NULL, 0, REG_SZ, (const uint8_t *)EDIT_DOC, (uint32_t)((_tcslen(EDIT_DOC)+1)*sizeof(TCHAR)));
        RegCloseKey(hRootKey);

        TCHAR path[MAX_PATH];
        GetModuleFileName(eu_module_handle(), path, MAX_PATH);

        TCHAR path_param[MAX_PATH] = _T("\""); 
        _tcsncat(path_param, path, MAX_PATH);
        _tcsncat(path_param, _T("\" \"%1\""), MAX_PATH);

        RegSetValueEx(h_key, NULL, 0, REG_SZ, (const uint8_t *)path_param, (uint32_t)sizeof(path_param));
        RegCloseKey(h_key);
    }
    //Set default icon value
    _sntprintf(reg_str, MAX_PATH-1, _T("%s%s"), EDIT_FILE, _T("\\DefaultIcon"));
    ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, reg_str, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &h_key, &dw_disp);

    if (ret == ERROR_SUCCESS)
    {
        TCHAR path[MAX_PATH];
        GetModuleFileName(eu_module_handle(), path, MAX_PATH);

        TCHAR path_param[MAX_PATH] = _T("\"");
        _tcsncat(path_param, path, MAX_PATH);
        _tcsncat(path_param, _T("\",2"), MAX_PATH);

        RegSetValueEx(h_key, NULL, 0, REG_SZ, (const uint8_t *)path_param, (uint32_t)sizeof(path_param));
        RegCloseKey(h_key);
    }
}

static INT_PTR CALLBACK 
reg_proc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HICON m_icon = LoadIcon(eu_module_handle(), MAKEINTRESOURCE(IDI_SKYLARK));
            if (m_icon)
            {
                SetClassLongPtr(hdlg, GCLP_HICONSM, (LONG_PTR)m_icon);
            }
            get_registered_exts(hdlg);
            get_supported_exts(hdlg);
            EnableWindow(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), false);
            EnableWindow(GetDlgItem(hdlg, IDC_REMOVEEXT_BUTTON), false);
            SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, EM_SETLIMITTEXT, NAME_MAX - 1, 0);
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hdlg, IDC_REMOVEEXT_BUTTON), L"Explorer", NULL);
                SendMessage(hdlg, WM_THEMECHANGED, 0, 0);
            }
            return 1;
        }
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
                const int buttons[] = {IDC_ADDFROMLANGEXT_BUTTON, IDC_REMOVEEXT_BUTTON};
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
            // 处理扩展名列表
            if (LOWORD(wParam) == IDC_REGEXT_LANGEXT_LIST || LOWORD(wParam) == IDC_REGEXT_REGISTEREDEXTS_LIST)
            {
                // 双击列表中的项目，然后在另外两个列表之间切换
                if (HIWORD(wParam) == LBN_DBLCLK)
                {
                    // 检查鼠标是否在空白区域上单击
                    if (-1 != SendDlgItemMessage(hdlg, LOWORD(wParam), LB_GETCURSEL, 0, 0))
                    {
                        if ((HWND)lParam == GetDlgItem(hdlg, IDC_REGEXT_LANGEXT_LIST))
                        {
                            SendMessage(hdlg, WM_COMMAND, IDC_ADDFROMLANGEXT_BUTTON, 0);
                        }
                        else
                        {
                            SendMessage(hdlg, WM_COMMAND, IDC_REMOVEEXT_BUTTON, 0);
                        }    
                    }
                    return 1;
                }
            }
            switch (wParam)
            {
                case IDC_ADDFROMLANGEXT_BUTTON:
                {
                    write_edit_path(hdlg);
                    TCHAR ext_add[NAME_MAX] = _T("");
                    if (!is_customize)
                    {
                        LRESULT index_add = SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_GETCURSEL, 0, 0);
                        LRESULT text_len = SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_GETTEXTLEN, index_add, 0);
                        if (text_len > NAME_MAX - 1)
                        {
                            return 1;
                        }
                        SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_GETTEXT, index_add, (LPARAM)(ext_add));
                        add_ext(ext_add);
                        SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, index_add, 0);
                    }
                    else
                    {
                        SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, NAME_MAX, (LPARAM)(ext_add));
                        LRESULT i = SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, (LPARAM)(ext_add));
                        if (i != LB_ERR)
                        {
                            return 1;
                        }
                        add_ext(ext_add);
                        SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, (LPARAM)(_T("")));
                    }
                    SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, (LPARAM)(ext_add));
                    EnableWindow(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), false);
                    return 1;
                }
                case IDC_REMOVEEXT_BUTTON:
                {
                    TCHAR ext_sup[NAME_MAX] = _T("");
                    LRESULT index_sup = SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETCURSEL, 0, 0);
                    LRESULT text_len = SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETTEXTLEN, index_sup, 0);
                    if (text_len > NAME_MAX - 1)
                    {
                        return 1;
                    }
                    SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETTEXT, index_sup, (LPARAM)(ext_sup));
                    if (delete_exts(ext_sup))
                    {
                        SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_DELETESTRING, index_sup, 0);
                    }
                    LRESULT lang_index = SendDlgItemMessage(hdlg, IDC_REGEXT_LANG_LIST, LB_GETCURSEL, 0, 0);
                    EnableWindow(GetDlgItem(hdlg, IDC_REMOVEEXT_BUTTON), false);
                    if (lang_index != LB_ERR)
                    {
                        for (int i = 1 ; i < EXT_LEN ; ++i)
                        {
                            if (!_tcsicmp(ext_sup, ext_array[lang_index][i]))
                            {
                                SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, (LPARAM)(ext_sup));
                                return 1;
                            }
                        }
                    }
                    return 1;
                }
                case IDCANCEL:
                {
                    return !!EndDialog(hdlg, 0);
                }
            }
            if (HIWORD(wParam) == EN_CHANGE)
            {
                TCHAR text[NAME_MAX] = _T("");
                SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, NAME_MAX, (LPARAM)(text));
                if ((_tcslen(text) == 1) && (text[0] != '.'))
                {
                    text[1] = text[0];
                    text[0] = '.';
                    text[2] = '\0';
                    SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, (LPARAM)(text));
                    SendDlgItemMessage(hdlg, IDC_CUSTOMEXT_EDIT, EM_SETSEL, 2, 2);
                }
                EnableWindow(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), (_tcslen(text) > 1));
                return 1;
            }
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                LRESULT i = SendDlgItemMessage(hdlg, LOWORD(wParam), LB_GETCURSEL, 0, 0);
                if (LOWORD(wParam) == IDC_REGEXT_LANG_LIST)
                {
                    if (i != LB_ERR)
                    {
                        TCHAR itemName[EXT_NAME + 1];
                        LRESULT text_len = SendDlgItemMessage(hdlg, LOWORD(wParam), LB_GETTEXTLEN, i, 0);
                        if (text_len > EXT_NAME)
                        {
                            return 1;
                        }
                        SendDlgItemMessage(hdlg, LOWORD(wParam), LB_GETTEXT, i, (LPARAM)(itemName));

                        if (!_tcsicmp(ext_array[SUPPORTED_LANG-1][0], itemName))
                        {
                            ShowWindow(GetDlgItem(hdlg, IDC_REGEXT_LANGEXT_LIST), SW_HIDE);
                            ShowWindow(GetDlgItem(hdlg, IDC_CUSTOMEXT_EDIT), SW_SHOW);
                            is_customize = true;
                        }
                        else
                        {
                            if (is_customize)
                            {
                                ShowWindow(GetDlgItem(hdlg, IDC_REGEXT_LANGEXT_LIST), SW_SHOW);
                                ShowWindow(GetDlgItem(hdlg, IDC_CUSTOMEXT_EDIT), SW_HIDE);
                                is_customize = false;
                            }
                            LRESULT count = SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_GETCOUNT, 0, 0);
                            for (count -= 1 ; count >= 0 ; count--)
                            {
                                SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, count, 0);
                            }
                            for (int j = 1 ; j < EXT_LEN ; ++j)
                            {
                                if (_tcscmp(_T(""), ext_array[i][j]))
                                {
                                    LRESULT index = SendDlgItemMessage(hdlg, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, (LPARAM)(ext_array[i][j]));
                                    if (index == -1)
                                    {
                                        SendDlgItemMessage(hdlg, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, (LPARAM)(ext_array[i][j]));
                                    }
                                }
                            }
                        }
                        EnableWindow(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), false);
                    }
                }
                else if (LOWORD(wParam) == IDC_REGEXT_LANGEXT_LIST)
                {
                    if (i != LB_ERR)
                    {
                        EnableWindow(GetDlgItem(hdlg, IDC_ADDFROMLANGEXT_BUTTON), true);
                    }
                }
                else if (LOWORD(wParam) == IDC_REGEXT_REGISTEREDEXTS_LIST)
                {
                    if (i != LB_ERR)
                    {
                        EnableWindow(GetDlgItem(hdlg, IDC_REMOVEEXT_BUTTON), true);
                    }
                }
            }
            break;
        }
        case WM_CLOSE:
        {
            return !!EndDialog(hdlg, LOWORD(wParam));
        }
        default:
            return 0;
    }
    return 0;
}

int 
eu_create_registry_dlg(void)
{
    int ret = 1;
    HANDLE m_map = NULL;
    HWND *memory = NULL;
    TCHAR ui_dest[ACNAME_LEN+1] = {0};
    m_map = share_open(FILE_MAP_READ, SKYLARK_LOCK_NAME);
    if (!m_map)
    {
        printf("share_open return false in %s\n", __FUNCTION__);
        return 1;
    }
    do
    {
        memory = (HWND *) share_map(m_map, sizeof(HANDLE), FILE_MAP_READ);
        if (!memory)
        {
            printf("share_map return false in %s\n", __FUNCTION__);
            break;
        } 
        if (i18n_dlgbox(*memory, IDD_REGEXT_BOX, reg_proc, 0) > 0)
        {
            ret = 0;
        }
    } while(0);
    if (memory)
    {
        share_unmap(memory);
    }
    share_close(m_map);
    return ret;
}

int
on_reg_files_association(void)
{
    int ret = 1;
    if (on_reg_admin())
    {
        ret = eu_create_registry_dlg();
    }
    else if (strcmp(eu_get_config()->window_theme, "black") == 0)
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg3=1")};
        ret = !run_as_admin(2, argv);
    }
    else
    {
        TCHAR *argv[] = {__ORIGINAL_NAME, _T("-reg3")};
        ret = !run_as_admin(2, argv);
    }
    return ret;
}

