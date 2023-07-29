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

#ifndef _EU_NPHOST_H_
#define _EU_NPHOST_H_

#include "eu_npruntime.h"

#define NPP_PDFVIEW (L"np_pdfviewer.dll")

typedef struct _npdata
{
    npp_t npp;
    npp_funcs funcs;
    npwindow win;
    npstream stream;
} npdata;

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

bool np_plugins_lookup(const wchar_t *file, const wchar_t *name, NMM *hmod);
int  np_plugins_initialize(const NMM hmod, npdata **pdata);
int  np_plugins_destroy(const npp_funcs *pfunc, const NPP instance, nppsave **data);
int  np_plugins_getvalue(const npp_funcs *pfunc, const NPP instance, npp_variable v, void **value);
int  np_plugins_setvalue(const npp_funcs *pfunc, const NPP instance, npp_variable v, void *value);
void np_plugins_shutdown(HMODULE *hmod, npdata **plugin);
void np_plugins_savefile(const npp_funcs *pfunc, const NPP instance);
void np_plugins_savefileas(const npp_funcs *pfunc, const NPP instance, const wchar_t *path);
void np_plugins_print(const npp_funcs *pfunc, const NPP instance, npprint *platform);
HMODULE np_load_plugin_library(const TCHAR *filename, const bool sys);

#ifdef __cplusplus
}
#endif

#endif // _EU_NPHOST_H_
