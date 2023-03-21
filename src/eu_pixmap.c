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

#include <float.h>
// for nanosvg
#ifndef NANOSVG_IMPLEMENTATION
#define NANOSVG_IMPLEMENTATION
#endif
#ifndef NANOSVGRAST_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#endif
#include "framework.h"

typedef struct _eu_pixmap
{
    int w, h;       // width and height
    int n;          // number of components in total (colors + spots + alpha)
    int stride;     // number of bytes per row
    uint8_t* img;   // buffer
}eu_pixmap;

static eu_pixmap*
on_pixmap_alloc(const size_t size)
{
    eu_pixmap *px = (eu_pixmap *)calloc(1, sizeof(eu_pixmap));
    if (px)
    {
        if (!(px->img = (uint8_t*)malloc(size)))
        {
            free(px);
            px = NULL;
        }
    }
    return px;
}

static eu_pixmap*
on_pixmap_new(const int w, const int h, const int num)
{
    eu_pixmap *dst = NULL;
    if (w > 0 && h > 0 && num > 0 && (dst = on_pixmap_alloc(w * h * 4 * num)) != NULL && dst->img)
    {
        dst->w = w * num;
        dst->h = h;
        dst->n = 4;
        dst->stride = w * 4 * num;
    }
    return dst;
}

static void
on_pixmap_destory(eu_pixmap **p)
{
    if (p && *p)
    {
        eu_safe_free((*p)->img);
        eu_safe_free(*p);
    }
}

static bool
on_pixmap_make_svg(const char *buf, char **p, const int index, const char *color)
{
    bool ret = false;
    if (p && index >= 0)
    {
        eue_toolbar* ptool = !buf ? eu_get_toolbar() : NULL;
        const int len = (const int)((ptool ? strlen(ptool[index].isvg) : strlen(buf)) + 64);
        *p = len < VALUE_LEN ? (char *)calloc(1, len) : NULL;
        if (*p)
        {
            strncpy(*p, ptool ? ptool[index].isvg : buf, len - 1);
            ret = true;
            if (STR_NOT_NUL(color))
            {
                eu_str_replace(*p, len - 1, "#455f91", color);
            }
        }
    }
    return ret;
}

static eu_pixmap*
on_pixmap_build(const char *buf, const int index, int w, int h, const char *color)
{
    NSVGimage *image = NULL;
    NSVGrasterizer *rast = NULL;
    eu_pixmap *pbuf = NULL;
    char *filename = NULL;
    float scale = 1.0f;
    if (on_pixmap_make_svg(buf, &filename, index, color) && filename)
    {
        image = nsvgParse(filename, "px", 96.0f);
        if (image == NULL) {
            printf("Could not open SVG image.\n");
            goto error;
        }
        if (image->width == 0 && image->height == 0 && !image->shapes)
        {
            printf("The data is not SVG at all.\n");
            goto error;
        }
        if (w != (int)image->width)
        {
            scale = (float)w/image->width;
        }
        rast = nsvgCreateRasterizer();
        if (rast == NULL) {
            printf("Could not init rasterizer.\n");
            goto error;
        }
        pbuf = on_pixmap_alloc(w*h*4);
        if (pbuf == NULL)
        {
            printf("Could not alloc image buffer.\n");
            goto error;
        }
        nsvgRasterize(rast, image, 0, 0, scale, pbuf->img, w, h, w*4);
        pbuf->w = w;
        pbuf->h = h;
        pbuf->n = 4;
        pbuf->stride = w * 4;
    }
error:
    if (rast)
    {
        nsvgDeleteRasterizer(rast);
    }
    if (image)
    {
        nsvgDelete(image);
    }
    if (filename)
    {
        free(filename);
    }
    return pbuf;
}

static HBITMAP
on_pixmap_create_bitmap(const eu_pixmap *pixmap)
{
    int w = pixmap->w;
    int h = pixmap->h;
    int n = 4;
    int img_size = w * h *n;
    int bits_count = n * 8;
    int line_stride = -((w * n + 3) & ~3);
    void* data = NULL;
    HBITMAP hbmp = NULL;
    BITMAPV4HEADER bitmapInfo = {0};
    bitmapInfo.bV4Size     = sizeof (BITMAPV4HEADER);
    bitmapInfo.bV4Width    = w;
    bitmapInfo.bV4Height   = -h;
    bitmapInfo.bV4Planes   = 1;
    bitmapInfo.bV4CSType   = 1;
    bitmapInfo.bV4BitCount = bits_count;
    bitmapInfo.bV4AlphaMask      = 0xff000000;
    bitmapInfo.bV4RedMask        = 0xff0000;
    bitmapInfo.bV4GreenMask      = 0xff00;
    bitmapInfo.bV4BlueMask       = 0xff;
    bitmapInfo.bV4V4Compression  = BI_BITFIELDS;
    if ((hbmp = CreateDIBSection(NULL, (BITMAPINFO*) &(bitmapInfo), DIB_RGB_COLORS, &data, NULL, 0)) && data)
    {
        unsigned int *dp = (unsigned int*)pixmap->img;
        for(int i = 0, len = w * h; i < len; ++i)
        {   //rgba to bgra
            dp[i] = (dp[i] & 0xff00ff00) | ((dp[i]>>16) & 0xFF) | ((dp[i] << 16) & 0xFF0000);
        }
        memcpy(data, pixmap->img, (size_t) abs (h * line_stride));
    }
    return hbmp;
}

static void
on_pixmap_blit(const eu_pixmap* dst, const eu_pixmap* src, int dstx, int dsty)
{
    int dx = src->w;
    int dy = src->h;
    int srcn = src->n;
    int dstn = dst->n;
    int src_stride = src->stride;
    int dst_stride = dst->stride;
    for (size_t y = 0; y < (size_t)dy; y++)
    {
        uint8_t* s = src->img + (src_stride * (size_t)y);
        size_t at_y = y + (size_t)dsty;
        uint8_t* d = dst->img + (dst_stride * at_y) + ((size_t)dstx * dstn);
        for (int x = 0; x < dx; x++)
        {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = s[3];
            d += dstn;
            s += srcn;
        }
    }
}

const int
on_pixmap_svg_count(void)
{
    int count = 0;
    for (eue_toolbar* p = eu_get_toolbar(); p && p->imsg != -1; ++p)
    {
        if (p->isvg[0])
        {
            ++count;
        }
    }
    return (const int)count;
}

const int
on_pixmap_vec_count(void)
{
    int count = 0;
    for (eue_toolbar* p = eu_get_toolbar(); p && p->imsg != -1; ++p)
    {
        ++count;
    }
    return (const int)count;
}

HBITMAP
on_pixmap_icons(const int w, const int h, const char *color, int *pout)
{
    HBITMAP bmp = NULL;
    eu_pixmap* pixmap = NULL;
    eue_toolbar* p = eu_get_toolbar();
    int count = 0;
    const int index = on_pixmap_vec_count();
    eu_pixmap* dst = on_pixmap_new(w, h, on_pixmap_svg_count());
    if (!dst)
    {
        return NULL;
    }
    for (int i = 0, j = 0; i < index - 1; ++i)
    {
        if (p[i].isvg[0] && (pixmap = on_pixmap_build(NULL, i, w, h, color)) != NULL)
        {
            on_pixmap_blit(dst, pixmap, count * w, 0);
            on_pixmap_destory(&pixmap);
            ++count;
        }
    }
    bmp = on_pixmap_create_bitmap(dst);
    if (bmp && pout)
    {
        *pout = count;
    }
    on_pixmap_destory(&dst);
    return bmp;
}

HBITMAP
on_pixmap_from_svg(const char *buf, const int w, const int h, const char *color)
{
    HBITMAP bmp = NULL;
    eu_pixmap* pixmap = on_pixmap_build(buf, 0, w, h, color);
    if (pixmap)
    {
        bmp = on_pixmap_create_bitmap(pixmap);
        on_pixmap_destory(&pixmap);
    }
    return bmp;
}
