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

#include "eu_nphost.h"

bool
np_process_path(wchar_t *path, const int len)
{
    wchar_t *p = NULL;
    if (!GetModuleFileNameW(NULL , path , len - 1))
    {
        return false;
    }
    p = wcsrchr(path , L'\\');
    if(p)
    {
        *p = 0 ;
    }
    return !!path[0];
}

void
np_plugins_savefile(const npp_funcs *pfunc, const NPP instance)
{
    if (pfunc && pfunc->savefile && instance)
    {
        pfunc->savefile(instance);
    }
}

void
np_plugins_savefileas(const npp_funcs *pfunc, const NPP instance, const wchar_t *path)
{
    if (pfunc && pfunc->savefileas && instance)
    {
        pfunc->savefileas(instance, path);
    }
}

bool
np_plugins_lookup(const wchar_t *file, const wchar_t *name, NMM *hmod)
{
    bool ret = false;
    if (file && name && name[0] && hmod)
    {
        wchar_t path[MAX_PATH] = {0};
        if (np_process_path(path, MAX_PATH))
        {
            wcsncat(path, L"\\plugins\\", MAX_PATH);
            wcsncat(path, file, MAX_PATH);
        }
        if (path[0] && (*hmod = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) != NULL)
        {
            np_mimetype_ptr fn_mimetype = (np_mimetype_ptr)GetProcAddress(*hmod, "npp_mime_type");
            if (fn_mimetype)
            {
                ret = fn_mimetype(name);
            }
        }
        if (!ret && *hmod)
        {
            FreeLibrary(*hmod);
            *hmod = NULL;
        }
    }
    return ret;
}

int
np_plugins_initialize(const NMM hmod, npdata **pdata)
{
    int ret = NP_GENERIC_ERROR;
    if (hmod && pdata)
    {
        np_entry_ptr fn_entry = (np_entry_ptr)GetProcAddress(hmod, "npp_entry");
        np_init_ptr fn_init = (np_init_ptr)GetProcAddress(hmod, "npp_init");
        if (fn_entry && fn_init)
        {
            if ((*pdata = (npdata *)calloc(1, sizeof(npdata))) != NULL)
            {
                ret = fn_entry(&(*pdata)->funcs);
                fn_init(hmod);
                ret = NP_NO_ERROR;
            }
        }
    }
    return ret;
}

void
np_plugins_print(const npp_funcs *pfunc, const NPP instance, npprint *platform)
{
    if (pfunc && pfunc->print && instance)
    {
        pfunc->print(instance, platform);
    }
}

int
np_plugins_getvalue(const npp_funcs *pfunc, const NPP instance, npp_variable v, void **value)
{
    if (pfunc && pfunc->getvalue && value)
    {
        return pfunc->getvalue(instance, v, value);
    }
    return NP_GENERIC_ERROR;
}

int
np_plugins_setvalue(const npp_funcs *pfunc, const NPP instance, npp_variable v, void *value)
{
    if (pfunc && pfunc->setvalue)
    {
        return pfunc->setvalue(instance, v, value);
    }
    return NP_GENERIC_ERROR;
}

int
np_plugins_destroy(const npp_funcs *pfunc, const NPP instance, nppsave **data)
{
    if (pfunc && pfunc->destroy)
    {
        pfunc->destroy(instance, data);
    }
    return NP_NO_ERROR;
}

void
np_plugins_shutdown(HMODULE *hmod, npdata **plugin)
{
    if (hmod && *hmod && plugin && *plugin)
    {
        np_shutdown_ptr fn_shutdown = NULL;
        if ((fn_shutdown = (np_shutdown_ptr)GetProcAddress(*hmod, "npp_shutdown")) != NULL)
        {
            fn_shutdown();
        }
        free(*plugin);
        *plugin = NULL;
    }
}
