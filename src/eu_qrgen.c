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
#include <qrencode.h>

// Prescaler (number of pixels in bmp file for each QRCode pixel, on each dimension)
#define OUT_FILE_PIXEL_PRESCALER     8
// Color of bmp pixels
#define PIXEL_COLOR_R                0
#define PIXEL_COLOR_G                0
#define PIXEL_COLOR_B                0
#define BI_RGB                       0L
#define EXP_WIDTH                    16

typedef QRcode* (*qrcode_string_ptr)(const char *string, int version, QRecLevel level, QRencodeMode hint, int casesensitive);
typedef void (*qrcode_free_ptr)(QRcode *qrcode);

static HWND hwnd_qr;
static HBITMAP hqr_bitmap;

static inline void
set_hwnd_qr(HWND hwnd)
{
    hwnd_qr = hwnd;
}

static HBITMAP 
resize_hbitmap(HWND hwnd, HBITMAP bmp_from, int width, int height)
{
    BITMAP  bm;
    HDC     hdc     = GetDC(hwnd);
    HDC     hdc_from = CreateCompatibleDC(hdc);
    HDC     hdc_to   = CreateCompatibleDC(hdc);
    HBITMAP bmp_to   = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ hob_from = SelectObject(hdc_from, bmp_from);
    HGDIOBJ hob_to   = SelectObject(hdc_to, bmp_to);
    GetObject(bmp_from, sizeof(bm), &bm);
    SetStretchBltMode(hdc_to, STRETCH_DELETESCANS);  // or COLORONCOLOR?
    if (!StretchBlt(hdc_to, 0, 0, width, height, hdc_from, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY))
    {
        DeleteObject(bmp_to);
        bmp_to = NULL;
    }
    SelectObject(hdc_from, hob_from);   
    SelectObject(hdc_to, hob_to);   
    DeleteObject(bmp_from);
    ReleaseDC(hwnd, hdc);
    DeleteDC(hdc_from);  
    DeleteDC(hdc_to);
    hqr_bitmap = bmp_to;
    return hqr_bitmap;
}

static HBITMAP 
get_hbitmap(const char *text)
{
    HBITMAP hbm = NULL;
    BITMAPINFO bi = {sizeof(BITMAPINFOHEADER)};
    unsigned int x, y, l, n, un_width, un_adjust_width, un_bytes;
    uint8_t *rgb_pdata = NULL;
    uint8_t *psource, *pdest;
    QRcode *qrcode;
    if (!text)
    {
        return NULL;
    }
    qrcode = QRcode_encodeString(text, 0, QR_ECLEVEL_H, QR_MODE_8, 1);
    if (!qrcode)
    {
        return NULL;
    }
    un_width = qrcode->width;
    un_adjust_width = un_width * OUT_FILE_PIXEL_PRESCALER * 3;
    if (un_adjust_width % 4)
    {
        un_adjust_width = (un_adjust_width / 4 + 1) * 4;
    }
    un_bytes = un_adjust_width * un_width * OUT_FILE_PIXEL_PRESCALER;
    bi.bmiHeader.biWidth = un_width * OUT_FILE_PIXEL_PRESCALER;
    bi.bmiHeader.biHeight = -((int)un_width * OUT_FILE_PIXEL_PRESCALER);
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;
    hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (VOID **)&rgb_pdata,  NULL, 0);
    if (hbm && rgb_pdata)
    {
        printf("bi.bmiHeader.biWidth = %ld, bi.bmiHeader.biHeight = %ld\n", bi.bmiHeader.biWidth, bi.bmiHeader.biHeight);
        // Preset to white
        memset(rgb_pdata, 0xff, un_bytes);
        // Convert QrCode bits to bmp pixels
        psource = qrcode->data;
        for(y = 0; y < un_width; y++)
        {
            pdest = rgb_pdata + un_adjust_width * y * OUT_FILE_PIXEL_PRESCALER;
            for(x = 0; x < un_width; x++)
            {
                if (*psource & 1)
                {
                    for(l = 0; l < OUT_FILE_PIXEL_PRESCALER; l++)
                    {
                        for(n = 0; n < OUT_FILE_PIXEL_PRESCALER; n++)
                        {
                            *(pdest +     n * 3 + un_adjust_width * l) = PIXEL_COLOR_B;
                            *(pdest + 1 + n * 3 + un_adjust_width * l) = PIXEL_COLOR_G;
                            *(pdest + 2 + n * 3 + un_adjust_width * l) = PIXEL_COLOR_R;
                        }
                    }
                }
                pdest += 3 * OUT_FILE_PIXEL_PRESCALER;
                psource++;
            }
        }
    }
    QRcode_free(qrcode);
    return hbm;
}

static void
on_qrgen_dpi_scale(HWND hdlg)
{
    RECT rc;
    HWND hwnd_sqr = NULL;
    GetWindowRect(hdlg, &rc);
    MoveWindow(hdlg, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, true);
    if ((hwnd_sqr = GetDlgItem(hdlg, IDC_IMG_QR)) != NULL)
    {
        MoveWindow(hwnd_sqr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, true);
        if (resize_hbitmap(hdlg, hqr_bitmap, rc.right - rc.left - EXP_WIDTH, rc.bottom - rc.top - EXP_WIDTH))
        {
            SendMessage(hwnd_sqr, STM_SETIMAGE,(WPARAM)IMAGE_BITMAP, (LPARAM)hqr_bitmap);
        }
    }
}

static INT_PTR CALLBACK 
func_qrgen(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            RECT rc;
            size_t len = 0;
            char *text = NULL;
            HBITMAP hbit = NULL;
            eu_tabpage *pnode = on_tabpage_focus_at();
            if (!pnode)
            {
                EndDialog(hdlg, 0);
                break;
            }
            if (!GetWindowRect(hdlg, &rc))
            {
                EndDialog(hdlg, 0);
                break;
            }
            text = util_strdup_select(pnode, &len, 0);
            if (!text)
            {
                EndDialog(hdlg, 0);
                break;
            }
            if (!(hbit = get_hbitmap(text)))
            {
                free(text);
                EndDialog(hdlg, 0);
                break;
            }
            if (!resize_hbitmap(hdlg, hbit, rc.right - rc.left - EXP_WIDTH, rc.bottom - rc.top - EXP_WIDTH))
            {
                free(text);
                EndDialog(hdlg, 0);
                break;
            }
            if (eu_get_dpi(hdlg) > 96)
            {
                on_qrgen_dpi_scale(hdlg);
            }
            else
            {
                SendDlgItemMessage(hdlg, IDC_IMG_QR, STM_SETIMAGE,(WPARAM)IMAGE_BITMAP, (LPARAM)hqr_bitmap);
            }
            if (text)
            {
                free(text);
            }
            set_hwnd_qr(hdlg);
            return 1;
        }
    case WM_DPICHANGED:
    {
        on_qrgen_dpi_scale(hdlg);
        return 1;
    }        
    case WM_LBUTTONUP:
        EndDialog(hdlg, LOWORD(wParam));
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hdlg, LOWORD(wParam));
            return 1;
        }
        break;
    case WM_DESTROY:
        if (hqr_bitmap)
        {
            printf("DeleteObject(hqr_bitmap)\n");
            DeleteObject(hqr_bitmap);
        }
        set_hwnd_qr(NULL);
        break;
    }
    return 0;
}

HWND
on_qrgen_hwnd(void)
{
    return hwnd_qr;
}

bool
on_qrgen_create_dialog(void)
{
    return DialogBoxParam(eu_module_handle(), MAKEINTRESOURCE(IDD_QRBOX), eu_module_hwnd(), func_qrgen, 0) > 0;
}
