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

#include "eu_npruntime.h"
#include <process.h>

#define PREFS_FILE_NAME L"SumatraPDF-settings.txt"
#define EXECUTE_FILE_NAME L"SumatraPDF.exe"

#ifndef IDS_PLUGINS_MSG1
#define IDS_PLUGINS_MSG1 44030
#endif
#ifndef IDM_SAVEAS_PLUGIN
#define IDM_SAVEAS_PLUGIN 204
#endif
#ifndef IDM_PRINT_PLUGIN
#define IDM_PRINT_PLUGIN 205
#endif
#ifndef IDM_SAVE_PLUGIN
#define IDM_SAVE_PLUGIN 260
#endif
#ifndef IDM_TITLE_PLUGIN
#define IDM_TITLE_PLUGIN 1224
#endif
#ifndef IDM_THEME_PLUGIN
#define IDM_THEME_PLUGIN 1225
#endif
#ifndef VALUE_LEN
#define VALUE_LEN 4096
#endif
#ifndef HVM_SETBKCOLOR
#define HVM_SETBKCOLOR (WM_USER + 105)
#endif
#define BUF_LEN 64

HINSTANCE dll_instance = NULL;

/* ::::: Plugin Window Procedure ::::: */
typedef struct _instance_data 
{
    npwindow    *npwin;
    WCHAR       message[MAX_PATH];
    WCHAR       filepath[MAX_PATH];
    HANDLE      hfile;
    HANDLE      hprocess;
    WCHAR       exepath[MAX_PATH];
    float       progress, prev_progress;
    uint32_t    total_size, curr_size;
    bool        istmp;
} instance_data;

typedef struct _double_buffer
{
    void (*init)(struct _double_buffer *_this, HWND hwnd, npn_rect rect);
    HWND hwnd_target;
    HDC hdc_canvas, hdc_buffer;
    HBITMAP hdb;
    npn_rect rc;
    void (*destroy)(struct _double_buffer *_this);
    HDC (*getdc)(struct _double_buffer *_this);
    void (*flush)(struct _double_buffer *_this, HDC hdc);
} double_buffer;

typedef struct _instance_theme
{
    COLORREF fg;
    COLORREF bg;
} instance_theme;

#define COL_WINDOW_BG RGB(0xcc, 0xcc, 0xcc)

enum {KB = 1024, MB = 1024 * KB, GB = 1024 * MB};

// Note: BufSet() should only be used when absolutely necessary (e.g. when
// handling buffers in OS-defined structures)
// returns the number of characters written (without the terminating \0)
size_t buffer_set(WCHAR *dst, size_t dst_size, const WCHAR *src)
{
    size_t src_size = wcslen(src);
    size_t sz_copy = min(dst_size - 1, src_size);
    if (dst_size > 0 && dst_size - 1 >= src_size)
    {
        wcsncpy(dst, src, sz_copy);
        return sz_copy;
    }
    return 0;
}

static wchar_t*
pdf_utf8_utf16(const char *utf8, size_t *out_len)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_utf8_utf16_ptr fn_utf8_utf16 = (npn_utf8_utf16_ptr)GetProcAddress(hmodule, "eu_utf8_utf16");
        if (fn_utf8_utf16)
        {
            return fn_utf8_utf16(utf8, out_len);
        }
    }
    return false;
}

static wchar_t*
pdf_wstr_replace(wchar_t* in, size_t in_size, const wchar_t* pattern, const wchar_t* by)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_wstr_replace_ptr fn_wstr_replace = (npn_wstr_replace_ptr)GetProcAddress(hmodule, "eu_wstr_replace");
        if (fn_wstr_replace)
        {
            return fn_wstr_replace(in, in_size, pattern, by);
        }
    }
    return NULL;
}

static HANDLE
pdf_new_process(const wchar_t* wcmd, const wchar_t* param, const wchar_t* pcd, int flags, uint32_t *o)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_new_process_ptr fn_new_process = (npn_new_process_ptr)GetProcAddress(hmodule, "eu_new_process");
        if (fn_new_process)
        {
            return fn_new_process(wcmd, param, pcd, flags, o);
        }
    }
    return NULL;    
}

static bool
pdf_translation(uint16_t id, wchar_t *str, int len)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_translation_ptr fn_translation = (npn_translation_ptr)GetProcAddress(hmodule, "eu_i18n_load_str");
        if (fn_translation)
        {
            return fn_translation(id, str, len);
        }
    }
    return false;
}

static bool
pdf_openfile(const wchar_t *path, pf_stream pstream)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_openfile_ptr fn_openfile = (npn_openfile_ptr)GetProcAddress(hmodule, "eu_open_file");
        if (fn_openfile)
        {
            return fn_openfile(path, pstream);
        }
    }
    return false;
}

static npn_theme
pdf_get_theme(void)
{
    HMODULE hmodule = GetModuleHandleW(L"euapi.dll");
    if (NULL != hmodule)
    {
        npn_theme_ptr fn_theme = (npn_theme_ptr)GetProcAddress(hmodule, "eu_get_theme");
        if (fn_theme)
        {
            return fn_theme();
        }
    }
    return NULL;
}

// format a number with a given thousand separator e.g. it turns 1234 into "1,234"
// Caller needs to free() the result.
static WCHAR*
format_thousand_num(size_t num)
{
    WCHAR thousand_sep[4];
    WCHAR buf[BUF_LEN] = {0};
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousand_sep, _countof(thousand_sep)))
    {
        thousand_sep[0] = L',';
    }
    _snwprintf(buf, BUF_LEN - 1, L"%Iu", num);
    size_t resLen = wcslen(buf) + wcslen(thousand_sep) * (wcslen(buf) + 3) / 3 + 1;
    WCHAR *res = (WCHAR *)calloc(sizeof(WCHAR), resLen);
    if (!res)
    {
        return NULL;
    }
    WCHAR *next = res;
    int i = 3 - (wcslen(buf) % 3);
    for (const WCHAR *src = buf; *src; ) 
    {
        *next++ = *src++;
        if (*src && i == 2)
        {
            next += buffer_set(next, resLen - (next - res), thousand_sep);
        }
        i = (i + 1) % 3;
    }
    *next = '\0';
    return res;
}

// Format a floating point number with at most two decimal after the point
static bool
format_thousand_float(WCHAR *buf, const int buf_len, double number)
{
    size_t num = (size_t)(number * 100 + 0.5);
    WCHAR *tmp = format_thousand_num(num / 100);
    if (tmp)
    {
        WCHAR decimal[4] = {0};
        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal, _countof(decimal)))
        {
            decimal[0] = L'.';
        }
        // always add between one and two decimals after the point
        _snwprintf(buf, buf_len, L"%s%s%02zu", tmp, decimal, num % 100);
        free(tmp);
        return true;
    }
    return false;
}

// Format the file size in a short form that rounds to the largest size unit
// e.g. "3.48 GB", "12.38 MB", "23 KB"
static bool
format_int_size(WCHAR *buf, const int buf_len, const size_t size) 
{
    WCHAR sizestr[BUF_LEN] = {0};
    const WCHAR *unit = NULL;
    double s = (double)size;
    if (size > GB)
    {
        s /= GB;
        unit = L"GB";
    }
    else if (size > MB)
    {
        s /= MB;
        unit = L"MB";
    }
    else
    {
        s /= KB;
        unit = L"KB";
    }
    if (!buf || !unit)
    {
        return false;
    }
    if (format_thousand_float(sizestr, BUF_LEN, s))
    {
        _snwprintf(buf, buf_len, L"%s %s", sizestr, unit);
    }
    return (buf[0] != 0);
}

static HDC
pdf_getdc(double_buffer *_this)
{
    return _this->hdc_buffer ? _this->hdc_buffer : _this->hdc_canvas;
}

static void
pdf_flush(double_buffer *_this, HDC hdc)
{
    if (_this->hdc_buffer && hdc != _this->hdc_buffer)
    {
        BitBlt(hdc, _this->rc.x, _this->rc.y, _this->rc.dx, _this->rc.dy, _this->hdc_buffer, 0, 0, SRCCOPY);
    }
}

static void
pdf_destroy_hdc(double_buffer *_this)
{
    if (_this)
    {
        DeleteObject(_this->hdb);
        DeleteDC(_this->hdc_buffer);
        ReleaseDC(_this->hwnd_target, _this->hdc_canvas);
    }
}

static void
pdf_init_hdc(double_buffer *_this, HWND hwnd, npn_rect rect)
{
    _this->hwnd_target = hwnd;
    _this->rc = rect;
    _this->destroy = pdf_destroy_hdc;
    _this->getdc = pdf_getdc;
    _this->flush = pdf_flush;
    _this->hdc_canvas = GetDC(hwnd);
    _this->hdb = _this->hdc_canvas ? CreateCompatibleBitmap(_this->hdc_canvas, rect.dx, rect.dy) : NULL;
    if (_this->hdb)
    {
        _this->hdc_buffer = CreateCompatibleDC(_this->hdc_canvas);
        if (_this->hdc_buffer && rect.x != 0 || rect.y != 0)
        {
            SetGraphicsMode(_this->hdc_buffer, GM_ADVANCED);
            XFORM ctm = { 1.0, 0, 0, 1.0, (float)-rect.x, (float)-rect.y };
            SetWorldTransform(_this->hdc_buffer, &ctm);
        }
        DeleteObject(SelectObject(_this->hdc_buffer, _this->hdb));
    }
}

static bool
pdf_exist_file(const wchar_t *path)
{
    uint32_t attr = GetFileAttributesW(path);
    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }
    return false;
}

static bool 
pdf_exe_path(wchar_t *path, const int len)
{
    wchar_t *p = NULL;
    if (!GetModuleFileNameW(dll_instance , path , len - 1))
    {
        return false;
    }
    p = wcsrchr(path , L'\\');
    if(p)
    {
        *(p+1) = 0 ;
        wcsncat(p, dll_instance ? EXECUTE_FILE_NAME : L"plugins\\"EXECUTE_FILE_NAME, len - 1);
        return true;
    }
    return false;
}

static HANDLE 
pdf_mktmp(WCHAR *file_path)
{
    TCHAR temp_path[MAX_PATH+1];
    HANDLE hfile = INVALID_HANDLE_VALUE;
    if (!GetTempPath(MAX_PATH, temp_path))
    {
        return NULL;
    }
    if (!GetTempFileName(temp_path, L"npv", 0, file_path))
    {
        printf("sp: GetTempFileName return false\n");
        return NULL;
    }
    if ((hfile = CreateFile(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("sp: pdf_mktmp(): CreateFile() failed\n");
        return NULL;
    }
    return hfile;
}

static HFONT
pdf_create_font(HDC hdc, const WCHAR *name, int size)
{
    LOGFONT lf = { 0 };
    lf.lfWidth = 0;
    lf.lfHeight = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), USER_DEFAULT_SCREEN_DPI);
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    wcsncpy(lf.lfFaceName, name, _countof(lf.lfFaceName));
    lf.lfWeight = FW_DONTCARE;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    return CreateFontIndirect(&lf);
}

static void 
to_offset(npn_rect *prc, int _x, int _y) 
{
    prc->x += _x;
    prc->y += _y;
}

static void 
to_inflate(npn_rect *prc, int _x, int _y)
{
    prc->x -= _x; 
    prc->dx += 2 * _x;
    prc->y -= _y;
    prc->dy += 2 * _y;
}

static void 
pdf_draw_text(HDC hdc, npn_rect *prc, const WCHAR *txt, bool rtl)
{
    RECT tmp;
    if (prc && npn_to_rect(prc, &tmp))
    {
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, txt, -1, &tmp, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (rtl ? DT_RTLREADING : 0));
    }
}

LRESULT CALLBACK
pdf_plugin_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_PAINT)
    {
        NPP instance = (NPP)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        instance_data *data = instance ? (instance_data *)instance->pdata : NULL;
        if (!data)
        {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        HBRUSH brush_bg = CreateSolidBrush(COL_WINDOW_BG);
        HFONT hfont = pdf_create_font(hdc, L"MS Shell Dlg", 14);
        
        // set up double buffering
        npn_rect rc_client;
        npn_client_rect(hwnd, &rc_client);
        
        double_buffer buffer = {pdf_init_hdc};
        buffer.init(&buffer, hwnd, rc_client);
        HDC hdc_buffer = buffer.getdc(&buffer);
        
        // display message centered in the window
        RECT rc;
        npn_to_rect(&rc_client, &rc);
        FillRect(hdc_buffer, &rc, brush_bg);
        hfont = (HFONT)SelectObject(hdc_buffer, hfont);
        SetTextColor(hdc_buffer, RGB(0, 0, 0));
        SetBkMode(hdc_buffer, TRANSPARENT);
        pdf_draw_text(hdc_buffer, &rc_client, data->message, false);
        
        // draw a progress bar, if a download is in progress
        if (0 < data->progress && data->progress <= 1)
        {
            SIZE msgSize;
            npn_rect rc_progress = rc_client;
            
            HBRUSH brush_progress = CreateSolidBrush(RGB(0x80, 0x80, 0xff));
            GetTextExtentPoint32(hdc_buffer, data->message, (int)wcslen(data->message), &msgSize);
            to_inflate(&rc_progress, -(rc_progress.dx - msgSize.cx) / 2, -(rc_progress.dy - msgSize.cy) / 2 + 2);
            to_offset(&rc_progress,0, msgSize.cy + 4 + 2);
            RECT rect_progress;
            npn_to_rect(&rc_progress, &rect_progress);
            FillRect(hdc_buffer, &rect_progress, (HBRUSH)GetStockObject(WHITE_BRUSH));
            npn_rect all_progress = rc_progress;
            rc_progress.dx = (int)(data->progress * rc_progress.dx);
            npn_to_rect(&rc_progress, &rect_progress);
            FillRect(hdc_buffer, &rect_progress, brush_progress);
            DeleteObject(brush_progress);
            
            WCHAR pcurr[BUF_LEN + 1] = {0};
            if (format_int_size(pcurr, BUF_LEN, data->curr_size))
            {   
                WCHAR ptotal[BUF_LEN + 1] = {0};
                if (0 == data->total_size || data->curr_size > data->total_size)
                {
                    // total size unknown or bogus => show just the current size
                    pdf_draw_text(hdc_buffer, &all_progress, pcurr, false);
                }
                else if (format_int_size(ptotal, BUF_LEN, data->total_size))
                {
                    WCHAR s[BUF_LEN + 1] = {0};
                    _snwprintf(s, BUF_LEN, L"%s of %s", pcurr, ptotal);
                    pdf_draw_text(hdc_buffer, &all_progress, s, false);
                }
            }
        }
        // draw the buffer on screen
        buffer.flush(&buffer, hdc);
        
        DeleteObject(SelectObject(hdc_buffer, hfont));
        DeleteObject(brush_bg);
        EndPaint(hwnd, &ps);
        HWND hchild = FindWindowEx(hwnd, NULL, NULL, NULL);
        if (hchild)
            InvalidateRect(hchild, NULL, FALSE);
    }
    else if (msg == WM_SIZE)
    {
        HWND hchild = FindWindowEx(hwnd, NULL, NULL, NULL);
        if (hchild)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            MoveWindow(hchild, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
        }
    }
    else if (msg == NPP_DOC_MODIFY)
    {
        npn_nmhdr nphdr = {0};
        nphdr.nm.idFrom = (intptr_t)lparam;
        npn_send_notify(hwnd, NPP_DOC_MODIFY, &nphdr);
    }
    else if (msg == WM_COPYDATA)
    {
        COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lparam;
        if (cds && 0x4C5255 == cds->dwData)  // URL
        {
            printf("sp: npn_geturl %s", (const char *)cds->lpData);
            return TRUE;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static int
pdf_new(NPP instance, nppsave *saved)
{
    if (!instance)
    {
        return NP_INVALID_INSTANCE;
    }
    instance->pdata = calloc(1, sizeof(instance_data));
    if (!instance->pdata)
    {
        printf("sp: pdf_new error: NPERR_OUT_OF_MEMORY_ERROR\n");
        return NP_OUT_OF_MEMORY;
    }
    instance_data *data = (instance_data *)instance->pdata;
    if (pdf_exe_path(data->exepath, MAX_PATH - 1))
    {
        pdf_translation(IDS_PLUGINS_MSG1, data->message, MAX_PATH - 1);
    }
    return NP_NO_ERROR;
}

static int
pdf_destroy(const NPP instance, nppsave **save)
{
    instance_data *data = instance ? (instance_data *)instance->pdata : NULL;
    if (data)
    {
        if (data->istmp)
        {
            DeleteFileW(data->filepath);
        }
        if (data->hprocess)
        {
            CloseHandle(data->hprocess);
        }
        free(instance->pdata);
        instance->pdata = NULL;
    }
    if (save && *save)
    {
        free(*save);
        *save = NULL;
    }
    return NP_NO_ERROR;
}

static void
pdf_savefile(const NPP instance)
{
    if (instance)
    {
        instance_data *data = (instance_data *)instance->pdata;
        if (data && data->npwin)
        {
            HWND hwnd = (HWND)data->npwin->window;
            HWND hchild = hwnd ? FindWindowEx(hwnd, NULL, NULL, NULL) : NULL;
            if (hchild)
            {
                SendMessage(hchild, WM_COMMAND, IDM_SAVE_PLUGIN, 0);
            }
        }
    }
}

static void
pdf_savefileas(const NPP instance, const wchar_t *path)
{
    if (instance)
    {
        instance_data *data = (instance_data *)instance->pdata;
        if (data && data->npwin)
        {
            HWND hwnd = (HWND)data->npwin->window;
            HWND hchild = hwnd ? FindWindowEx(hwnd, NULL, NULL, NULL) : NULL;
            if (hchild)
            {
                PostMessage(hchild, WM_COMMAND, IDM_SAVEAS_PLUGIN, 0);
            }
        }
    }
}

static int
pdf_setwindow(NPP instance, npwindow *npwin)
{
    if (!instance)
    {
        return NP_INVALID_INSTANCE;
    }
    instance_data *data = (instance_data *)instance->pdata;
    if (!npwin)
    {
        data->npwin = NULL;
    }
    else if (data->npwin != npwin)
    {
        HWND hwnd = (HWND)npwin->window;
        data->npwin = npwin;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)instance);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pdf_plugin_proc);
    }
    else
    {   // The plugin's window hasn't changed, just its size
        HWND hwnd = (HWND)npwin->window;
        HWND hchild = FindWindowEx(hwnd, NULL, NULL, NULL);
        if (hchild)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            MoveWindow(hchild, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
        }
    }
    return NP_NO_ERROR;
}

static void
pdf_progress_repaint(instance_data *data)
{
    if (!data || !data->npwin || !data->npwin->window)
    {
        return;
    }
    float diff = data->progress - data->prev_progress;
    if (diff < 0 || diff > 0.01f)
    {
        HWND hwnd = (HWND)data->npwin->window;
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        data->prev_progress = data->progress;
    }
}

static int
pdf_newstream(NPP instance, npstream* stream, bool seekable, uint16_t* stype)
{
    instance_data *data = (instance_data *)instance->pdata;
    if (!*data->exepath)
    {
        printf("sp: pdf_newstream() error: NPERR_FILE_NOT_FOUND\n");
        return NP_FILE_NOT_FOUND;
    }
    // if we can create a temporary file ourselfes, we manage the download
    // process. The reason for that is that NP_ASFILE (where browser manages
    // file downloading) is not reliable and has been broken in almost every
    // browser at some point
    if (stype)
    {
        *stype = NP_ASFILE;
    }
    data->hfile = pdf_mktmp(data->filepath);
    if (data->hfile)
    {
        printf("sp: using temporary file\n");
        data->istmp = true;
        stype ? *stype = NP_NORMAL : (void)0;
    }
    data->total_size = stream->end;
    data->curr_size = 0;
    data->progress = stream->end > 0 ? 0.01f : 0;
    data->prev_progress = -.1f;
    pdf_progress_repaint(data);
    return NP_NO_ERROR;
}

static uint32_t
pdf_writeready(NPP instance, npstream* stream)
{
    int32_t res = stream->end > 0 ? stream->end : UINT_MAX;
    printf("sp:  pdf_writeready() res=%d\n", res);
    return res;
}

static uint32_t
pdf_write(NPP instance, npstream* stream, uint32_t offset, uint32_t len, void* buffer)
{
    instance_data *data = (instance_data *)instance->pdata;
    uint32_t bwritten = len;
    if (data->hfile)
    {
        // Note: we optimistically assume that data comes in sequentially
        // (i.e. next offset will be current offset + bytesWritten)
        BOOL ok = WriteFile(data->hfile, buffer, (DWORD)len, &bwritten, NULL);
        if (!ok)
        {
            printf("sp: pdf_write() failed to write %u bytes at offset %u\n", len, offset);
            return -1;
        }
    }
    data->curr_size = offset + bwritten;
    data->progress = stream->end > 0 ? 1.0f * (offset + len) / stream->end : 0;
    pdf_progress_repaint(data);
    return bwritten;
}

static int
launch_sumatra(instance_data *data, const char *url_utf8)
{
    if (!data)
    {
        return NP_NO_DATA;
    }
    if (!pdf_exist_file(data->filepath))
    {
        printf("sp: pdf_stream2file() error: file doesn't exist\n");
    }
    WCHAR url[VALUE_LEN] = {0};
    int m = MultiByteToWideChar(CP_UTF8, 0, url_utf8 ? url_utf8 : "", -1, url, VALUE_LEN);
    if (m > 0 && m < VALUE_LEN)
    {
        WCHAR *cmd_line = NULL;
        pdf_wstr_replace(url, VALUE_LEN, L"\"", L"%22");
        if (url[wcslen(url) - 1] == L'\\')
        {
            url[wcslen(url) - 1] = 0;
            wcsncat(url, L"%5c", VALUE_LEN);
        }
        if ((cmd_line = (WCHAR *)calloc(sizeof(WCHAR), VALUE_LEN + MAX_PATH)) != NULL)
        {
            if (pdf_get_theme())
            {
                uint32_t fg = pdf_get_theme()->item.text.color;
                uint32_t bg = pdf_get_theme()->item.text.bgcolor;
                _snwprintf(cmd_line, VALUE_LEN + MAX_PATH - 1, L"\"%s\" -plugin \"%s\" %zd \"%s\" -bgcolor #%08x -set-color-range #%08x #%08x",
                           data->exepath, url, data->npwin ? (intptr_t)data->npwin->window : (intptr_t)0, data->filepath, bg, fg, bg);
            }
            else
            {
                _snwprintf(cmd_line, VALUE_LEN + MAX_PATH - 1, L"\"%s\" -plugin \"%s\" %zd \"%s\"",
                           data->exepath, url, data->npwin ? (intptr_t)data->npwin->window : (intptr_t)0, data->filepath);                
            }
            data->hprocess = pdf_new_process(cmd_line, NULL, NULL, 2, NULL);
            if (!data->hprocess)
            {
                printf("sp: pdf_stream2file() error: couldn't run SumatraPDF!\n");
                wcsncpy(data->message, L"Error: Couldn't run SumatraPDF!", MAX_PATH);
            }
        }
    }
    return NP_NO_ERROR;
}

static unsigned WINAPI
pdf_show_progress(void *p)
{
    instance_data *data = (instance_data *)p;
    if (data && data->npwin)
    {
        uint32_t pre_size = (uint32_t)(data->total_size/50);
        HWND hchild = NULL;
        HWND hwnd = (HWND)data->npwin->window;
        for (int i = 540; !hchild && i > 0; --i)
        {
            hchild = FindWindowEx(hwnd, NULL, NULL, NULL);    
            if (data->curr_size < data->total_size)
            {
                if (i > 500)
                {
                    data->curr_size += pre_size;
                }
                else
                {
                    data->curr_size += 400;
                }
            }
            InvalidateRect((HWND)data->npwin->window, NULL, FALSE);
            UpdateWindow((HWND)data->npwin->window);
            Sleep(500);
        }
    }
    return 0;
}

static void
pdf_stream2file(NPP instance, npstream* stream, const char* fname)
{
    instance_data *data = (instance_data *)instance->pdata;
    if (!fname)
    {
        printf("sp: pdf_stream2file() error: fname is NULL\n");
        wcsncpy(data->message, L"Error: The document couldn't be downloaded!", MAX_PATH);
        goto Exit;
    }
    if (data->hfile)
    {
        printf("sp: pdf_stream2file() error: data->hfile is != NULL (should be NULL)\n");
    }
    data->progress = 1.0f;
    data->prev_progress = 0.0f; // force update
    pdf_progress_repaint(data);
    MultiByteToWideChar(CP_UTF8, 0, fname, -1, data->filepath, MAX_PATH);
    struct _stat st;
    _wstat(data->filepath, &st);
    data->total_size = (uint32_t)st.st_size;
    launch_sumatra(data, stream ? stream->url : "");
    // 文件打开慢时, 使用一个不太精确的进度条提示
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, pdf_show_progress, (void *)data, 0, NULL));
Exit:
    if (data->npwin)
    {
        InvalidateRect((HWND)data->npwin->window, NULL, FALSE);
        UpdateWindow((HWND)data->npwin->window);
    }
}

static int
pdf_destroystream(NPP instance, npstream* stream, uint16_t reason)
{
    instance_data *data;
    printf("sp: NPP_DestroyStream() reason: %d\n", reason);
    if (stream)
    {
        if (stream->url)
        {
            printf("url: %s", stream->url);
        }
        printf("end: %d", stream->end);
    }
    if (!instance)
    {
        printf("sp: pdf_destroystream() error: NP_INVALID_INSTANCE\n");
        return NP_INVALID_INSTANCE;
    }
    data = (instance_data *)instance->pdata;
    if (!data)
    {
        printf("sp: pdf_destroystream() error: instance->pdata is NULL\n");
        return NP_OUT_OF_MEMORY;
    }
    if (!data->hfile)
    {
        return NP_FILE_NOT_FOUND;
    }
    CloseHandle(data->hfile);
    data->hfile = NULL;
    launch_sumatra(data, stream ? stream->url : NULL);
    if (data->npwin)
    {
        InvalidateRect((HWND)data->npwin->window, NULL, FALSE);
        UpdateWindow((HWND)data->npwin->window);
    }
    return NP_NO_ERROR;
}

static void
pdf_print(NPP instance, npprint *platform)
{
    if (platform && NP_FULL != platform->mode)
    {
        printf("sp: pdf_print(), platformPrint->mode is %d (!= NP_FULL)\n", platform->mode);
        return;
    }
    if (instance)
    {
        instance_data *data = (instance_data *)instance->pdata;
        if (data && data->npwin)
        {
            HWND hwnd = (HWND)data->npwin->window;
            HWND hchild = hwnd ? FindWindowEx(hwnd, NULL, NULL, NULL) : NULL;
            if (hchild)
            {
                PostMessage(hchild, WM_COMMAND, IDM_PRINT_PLUGIN, 0);
                if (platform)
                {
                    platform->print.fullprint.plugin = true;
                }
            }
        }
    }
}

static int
pdf_event(NPP instance, void *event)
{
    return NP_NO_ERROR;
}

static int
pdf_getvalue(NPP instance, npp_variable variable, void **value)
{
    int ret = NP_GENERIC_ERROR;
    if (instance && value)
    {
        instance_data *data = (instance_data *)instance->pdata;
        if (data && data->npwin && data->hprocess)
        {
            if (variable == NV_TABTITLE)
            {
                HWND hwnd = (HWND)data->npwin->window;
                HWND hchild = hwnd ? FindWindowEx(hwnd, NULL, NULL, NULL) : NULL;
                size_t bytes_value = MAX_PATH * sizeof(wchar_t);
                *value = calloc(1, bytes_value);
                void *remote_ptr = VirtualAllocEx(data->hprocess, 0, bytes_value, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (hchild && remote_ptr && *value)
                {
                    size_t bytes_written = 0;
                    bool w = WriteProcessMemory(data->hprocess, remote_ptr, *value, bytes_value, &bytes_written);
                    if (w && SendMessage(hchild, WM_COMMAND, IDM_TITLE_PLUGIN, (LPARAM)remote_ptr))
                    {
                        ReadProcessMemory(data->hprocess, remote_ptr, *value, bytes_value, &bytes_written);
                        ret = NP_NO_ERROR;
                    }
                }
                if (remote_ptr)
                {
                    VirtualFreeEx(data->hprocess, remote_ptr, 0, MEM_RELEASE);
                }
            }
            else if (variable == NV_TMPNAME)
            {
                size_t bytes_value = MAX_PATH * sizeof(wchar_t);
                *value = calloc(1, bytes_value);
                if (*value)
                {
                    wcsncpy((wchar_t *)(*value), data->filepath, MAX_PATH - 1);
                    ret = NP_NO_ERROR;
                }
            }
            else if (variable == NV_STREAM && ((*value) = (npn_stream *)calloc(1, sizeof(npn_stream))))
            {
                if (pdf_openfile(data->filepath, (npn_stream *)(*value)))
                {
                    ret = NP_NO_ERROR;
                }
            }
        }
    }
    return ret;
}

static int
pdf_setvalue(NPP instance, npp_variable variable, void *value)
{
    int ret = NP_GENERIC_ERROR;
    if (instance)
    {
        (void *)value;
        instance_data *data = (instance_data *)instance->pdata;
        if (data && data->npwin && data->hprocess)
        {
            HWND hwnd = (HWND)data->npwin->window;
            HWND hchild = hwnd ? FindWindowEx(hwnd, NULL, NULL, NULL) : NULL;
            if (variable == NV_THEME && hchild && pdf_get_theme())
            {             
                instance_theme theme = {0};
                size_t bytes_value = sizeof(instance_theme);
                theme.fg = pdf_get_theme()->item.text.color;
                theme.bg = pdf_get_theme()->item.text.bgcolor;
                void *remote_ptr = VirtualAllocEx(data->hprocess, 0, bytes_value, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (remote_ptr)
                {
                    size_t bytes_written = 0;
                    bool w = WriteProcessMemory(data->hprocess, remote_ptr, (void *)&theme, bytes_value, &bytes_written);
                    if (w)
                    {
                        SendMessage(hchild, WM_COMMAND, IDM_THEME_PLUGIN, (LPARAM)remote_ptr);
                        ret = NP_NO_ERROR;
                    }
                    VirtualFreeEx(data->hprocess, remote_ptr, 0, MEM_RELEASE);
                }
            }
        }
    }
    return ret;
}

// NPP回调函数准备
NPP_EXPORT int
npp_entry(npp_funcs *pfunc)
{
    if (pfunc != NULL)
    {
        pfunc->size = (uint16_t)sizeof(npp_funcs);
        pfunc->newp = pdf_new;
        pfunc->destroy = pdf_destroy;
        pfunc->setwindow = pdf_setwindow;
        pfunc->newstream = pdf_newstream;
        pfunc->destroystream = pdf_destroystream;
        pfunc->asfile = pdf_stream2file;
        pfunc->writeready = pdf_writeready;
        pfunc->write = pdf_write;
        pfunc->savefile = pdf_savefile;
        pfunc->savefileas = pdf_savefileas;
        pfunc->getvalue = pdf_getvalue;
        pfunc->setvalue = pdf_setvalue;
        pfunc->print = pdf_print;
        pfunc->event = pdf_event;
        return NP_NO_ERROR;
    }
    return NP_INVALID_FUNCTABLE;
}

// 不使用NPN系函数, 我们获取euapi模块地址
// 直接调用动态库里的函数就好了
NPP_EXPORT int
npp_init(const NMM hmod)
{
    if (!dll_instance)
    {
        dll_instance = (HINSTANCE)hmod;
    }
    return NP_NO_ERROR;
}


// 确认是否支持的文件类型
// 与插件关联的进程是否存在
NPP_EXPORT bool
npp_mime_type(const wchar_t *ext_name)
{
    wchar_t path[MAX_PATH] = {0};
    const wchar_t *mime_types[] = {L".cbr",
                                   L".cbz",
                                   L".chm",
                                   L".pdf",
                                   L".pdb",
                                   L".xps",
                                   L".djvu",
                                   L".epub",
                                   L".mobi",
                                   NULL
                                  };
    if (!pdf_exe_path(path, MAX_PATH) || !pdf_exist_file(path))
    {
        return false;
    }
    for (int i = 0; mime_types[i]; ++i)
    {
        if (_wcsicmp(mime_types[i], ext_name) == 0)
        {
            return true;
        }
    }
    return false;
}

NPP_EXPORT void 
npp_shutdown(void)
{
    dll_instance = NULL;
}
