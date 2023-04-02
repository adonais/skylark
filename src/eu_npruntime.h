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

#ifndef _H_SKYLARK_NPRUNTIME_
#define _H_SKYLARK_NPRUNTIME_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <windows.h>

#define NPERR_BASE                     0
#define NP_NO_ERROR                    (NPERR_BASE + 0)
#define NP_GENERIC_ERROR               (NPERR_BASE + 1)
#define NP_INVALID_INSTANCE            (NPERR_BASE + 2)
#define NP_INVALID_FUNCTABLE           (NPERR_BASE + 3)
#define NP_MODULE_LOAD_FAILED          (NPERR_BASE + 4)
#define NP_OUT_OF_MEMORY               (NPERR_BASE + 5)
#define NP_INVALID_PLUGIN              (NPERR_BASE + 6)
#define NP_INVALID_PLUGIN_DIR          (NPERR_BASE + 7)
#define NP_INCOMPATIBLE_VERSION        (NPERR_BASE + 8)
#define NP_INVALID_PARAM               (NPERR_BASE + 9)
#define NP_INVALID_URL                 (NPERR_BASE + 10)
#define NP_FILE_NOT_FOUND              (NPERR_BASE + 11)
#define NP_NO_DATA                     (NPERR_BASE + 12)
#define NP_STREAM_NOT_SEEKABLE         (NPERR_BASE + 13)
#define NPP_DOC_MODIFY                 (WM_USER+30000)
#define NPP_DOC_STATUS                 (WM_USER+30001)
#define NPP_EXPORT                     __declspec(dllexport)

/*
 * Values for mode passed to NPP_New:
 */
#define NP_EMBED 1
#define NP_FULL  2

/*
 * Values for stream type passed to NPP_NewStream:
 */
#define NP_NORMAL     1
#define NP_SEEK       2
#define NP_ASFILE     3
#define NP_ASFILEONLY 4

#define DW_SIZE       32
#define QW_SIZE       64
#define TITLE_SZIE    512

#ifndef MAX_BUFFER
#define MAX_BUFFER    1024
#endif

#ifndef VALUE_LEN
#define VALUE_LEN     4096
#endif

#define NP_MAXREADY (((unsigned)(~0)<<1)>>1)

typedef struct _npp_t
{
  void* pdata;      /* plug-in private data */
  void* ndata;      /* reserve private data */
} npp_t;

typedef npp_t *NPP;
typedef HMODULE NMM;

typedef enum
{
    NV_TABTITLE = 1,
    NV_TMPNAME,
    NV_STREAM,
    NV_THEME_CHANGE,
    NV_PATH_CHANGE,
    NV_OBJETCT
} npp_variable;

typedef enum
{
    OPERATE_NONE = 0,
    OPERATE_SAVE,
    OPERATE_SAVEAS
} wparam_variable;

typedef struct _npstream
{
  void*    pdata;         /* plug-in private data */
  void*    ndata;         /* reserve private data */
  const    char* url;
  intptr_t end;
  uint32_t lastmodified;
  void*    notifydata;
  const    char* headers; /* Response headers from host.
                           * Exists only for >= NPVERS_HAS_RESPONSE_HEADERS.
                           * Used for HTTP only; NULL for non-HTTP.
                           * Available from NPP_NewStream onwards.
                           * Plugin should copy this data before storing it.
                           * Includes HTTP status line and all headers,
                           * preferably verbatim as received from server,
                           * headers formatted as in HTTP ("Header: Value"),
                           * and newlines (\n, NOT \r\n) separating lines.
                           * Terminated by \n\0 (NOT \n\n\0). */
} npstream;

typedef struct _npwindow
{
    void *window;   /* Platform specific window handle */
    int32_t x;      /* Position of top left corner relative */
    int32_t y;      /* to a netscape page. */
    uint32_t width; /* Maximum window size */
    uint32_t height;
    int type;       /* Is this a window or a drawable? */
} npwindow;

typedef struct _npfull_print
{
    bool plugin;      /* Set true if plugin handled fullscreen printing */
    bool copy;        /* true if plugin should print one copy to default printer */
    void *platform;   /* Platform-specific printing info */
} npfull_print;

typedef struct _npembed_print
{
    npwindow window;
    void *platform; /* Platform-specific printing info */
} npembed_print;

typedef struct _npprint
{
    uint16_t mode; /* NP_FULL or NP_EMBED */
    union
    {
        npfull_print fullprint;   /* if mode is NP_FULL */
        npembed_print embedprint; /* if mode is NP_EMBED */
    } print;
} npprint;

typedef struct _nppsave
{
  int32_t len;
  void*   buf;
} nppsave;

struct styleclass
{
    char font[DW_SIZE];
    int fontsize;
    uint32_t color;
    uint32_t bgcolor;
    int bold;
};

struct styletheme
{
    struct styleclass linenumber;
    struct styleclass foldmargin;

    struct styleclass text;
    struct styleclass caretline;
    struct styleclass indicator;

    struct styleclass keywords0;
    struct styleclass keywords1;
    struct styleclass string;
    struct styleclass character;
    struct styleclass number;
    struct styleclass operators;
    struct styleclass preprocessor;
    struct styleclass comment;
    struct styleclass commentline;
    struct styleclass commentdoc;

    struct styleclass tags;
    struct styleclass unknowtags;
    struct styleclass attributes;
    struct styleclass unknowattributes;
    struct styleclass entities;
    struct styleclass tagends;
    struct styleclass cdata;
    struct styleclass phpsection;
    struct styleclass aspsection;

    struct styleclass activetab;
    struct styleclass caret;
    struct styleclass symbolic;
    struct styleclass hyperlink;
};

typedef struct _npn_rect
{
    int x, y;
    int dx, dy;
}npn_rect;

typedef struct eu_theme
{
    char pathfile[MAX_PATH];
    char name[QW_SIZE];
    struct styletheme item;
} *npn_theme;

typedef struct _eue_toolbar
{
    int imsg;
    int icmd;
    char isvg[VALUE_LEN];
} eue_toolbar, *npn_toolbar;

typedef struct  _npn_nmhdr
{
    NMHDR nm;
    bool modified;
    void *pnode;
} npn_nmhdr;

typedef struct _npn_stream* pf_stream;
typedef void (*npn_stream_close)(pf_stream pstream);
typedef struct  _npn_stream
{
    size_t               size;
    uintptr_t            base;
    npn_stream_close     close;
} npn_stream;

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*npp_new_ptr)(NPP instance, nppsave* saved);
typedef int (*npp_destroy_ptr)(const NPP instance, nppsave **save);
typedef int (*npp_setwindow_ptr)(NPP instance, npwindow *window);
typedef int (*npp_newstream_ptr)(NPP instance, npstream* stream, bool seekable, uint16_t* stype);
typedef int (*npp_destroystream_ptr)(NPP instance, npstream* stream, uint16_t reason);
typedef int (*npp_handle_ptr)(NPP instance, void *event);
typedef int (*npp_getvalue_ptr)(NPP instance, npp_variable v, void **value);
typedef int (*npp_setvalue_ptr)(NPP instance, npp_variable v, void *value);
typedef int (*npp_stream2file_ptr)(NPP instance, npstream* stream);
typedef void (*npp_print_ptr)(NPP instance, npprint *platform);
typedef void (*npp_savefile_ptr)(const NPP instance);
typedef void (*npp_savefileas_ptr)(const NPP instance, const wchar_t *fname);
typedef uint32_t (*npp_writeready_ptr)(NPP instance, npstream* stream);
typedef uint32_t (*npp_write_ptr)(NPP instance, npstream* stream, uint32_t offset, uint32_t len, void* buffer);

typedef struct _npp_funcs
{
    uint16_t size;
    npp_new_ptr newp;
    npp_destroy_ptr destroy;
    npp_setwindow_ptr setwindow;
    npp_newstream_ptr newstream;
    npp_destroystream_ptr destroystream;
    npp_stream2file_ptr asfile;
    npp_writeready_ptr writeready;
    npp_write_ptr write;
    npp_savefile_ptr savefile;
    npp_savefileas_ptr savefileas;
    npp_getvalue_ptr getvalue;
    npp_setvalue_ptr setvalue;
    npp_print_ptr print;
    npp_handle_ptr event;
} npp_funcs;

typedef int  (*np_entry_ptr)(npp_funcs *pfunc);
typedef void (*np_init_ptr)(const NMM hmod);
typedef void (*np_shutdown_ptr)(void);
typedef bool (*np_mimetype_ptr)(const wchar_t *name);

typedef bool      (*npn_translation_ptr)(uint16_t id, wchar_t *str, int len);
typedef bool      (*npn_openfile_ptr)(const wchar_t *path, pf_stream pstream);
typedef wchar_t*  (*npn_utf8_utf16_ptr)(const char *utf8, size_t *out_len);
typedef wchar_t*  (*npn_wstr_replace_ptr)(wchar_t* in, size_t in_size, const wchar_t* pattern, const wchar_t* by);
typedef HANDLE    (*npn_new_process_ptr)(const wchar_t* wcmd, const wchar_t* param, const wchar_t* pcd, int flags, uint32_t *o);
typedef npn_theme (*npn_theme_ptr)(void);

static bool
npn_client_rect(HWND hwnd, npn_rect *prect)
{
    RECT rc;
    if (prect && GetClientRect(hwnd, &rc))
    {
        prect->x = rc.left;
        prect->y = rc.top;
        prect->dx = rc.right - rc.left;
        prect->dy = rc.bottom - rc.top;
        return true;
    }
    return false;
}

static bool
npn_to_rect(npn_rect *prc, RECT *rect)
{
    if (prc && rect)
    {
        rect->left = prc->x;
        rect->top = prc->y;
        rect->right = prc->x + prc->dx;
        rect->bottom = prc->y + prc->dy;
        return true;
    }
    return false;
}

static intptr_t
npn_send_notify(HWND hwnd, uint32_t code, WPARAM local, npn_nmhdr *phdr)
{
    phdr->nm.hwndFrom = hwnd;
    phdr->nm.code = code;
    return (intptr_t)SendMessage(GetParent(hwnd), WM_NOTIFY, local, (LPARAM) phdr);
}

#ifdef __cplusplus
}
#endif

#endif // _H_SKYLARK_NPRUNTIME_
