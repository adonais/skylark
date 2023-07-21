/******************************************************************************
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
#include <shlobj_core.h>

typedef const char *(__cdecl *pwine_get_version)(void);
typedef char *(__cdecl *pwine_get_unix_file_name)(LPCWSTR dos);
typedef wchar_t *(__cdecl *pwine_get_nt_file_name)(LPCSTR str);
typedef void (*ptr_file_enc)(FILE *f, void **pout);
typedef unsigned long (*ptr_compress_bound)(unsigned long source_len);
typedef int (*ptr_compress)(uint8_t *, unsigned long *, const uint8_t *, unsigned long, int);
typedef int (*ptr_uncompress)(uint8_t *, unsigned long *, const uint8_t *, unsigned long *);

typedef DWORD(WINAPI *PFNGFVSW)(LPCWSTR, LPDWORD);
typedef DWORD(WINAPI *PFNGFVIW)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef bool(WINAPI *PFNVQVW)(LPCVOID, LPCWSTR, LPVOID, PUINT);

typedef struct _LANGANDCODEPAGE
{
    uint16_t wLanguage;
    uint16_t wCodePage;
} LANGANDCODEPAGE;

static PFNGFVSW pfnGetFileVersionInfoSizeW;
static PFNGFVIW pfnGetFileVersionInfoW;
static PFNVQVW pfnVerQueryValueW;

#define AES_IV_MATERIAL "copyright by skylark team"
#define CONFIG_KEY_MATERIAL_SKYLARK    "EU_SKYLARK"

// For win32 clock_gettime
#define MS_PER_SEC      1000ULL     // MS = milliseconds
#define US_PER_MS       1000ULL     // US = microseconds
#define HNS_PER_US      10ULL       // HNS = hundred-nanoseconds (e.g., 1 hns = 100 ns)
#define NS_PER_US       1000ULL

#define HNS_PER_SEC     (MS_PER_SEC * US_PER_MS * HNS_PER_US)
#define NS_PER_HNS      (100ULL)    // NS = nanoseconds
#define NS_PER_SEC      (MS_PER_SEC * US_PER_MS * NS_PER_US)

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME  0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

static pwine_get_version fn_wine_get_version;

static int
clock_gettime_monotonic(struct timespec *tv)
{
    static LARGE_INTEGER ticksPerSec;
    LARGE_INTEGER ticks;
    double seconds;

    if (!ticksPerSec.QuadPart)
    {
        QueryPerformanceFrequency(&ticksPerSec);
        if (!ticksPerSec.QuadPart)
        {
            errno = ENOTSUP;
            return -1;
        }
    }

    QueryPerformanceCounter(&ticks);

    seconds = (double) ticks.QuadPart / (double) ticksPerSec.QuadPart;
    tv->tv_sec = (time_t) seconds;
    tv->tv_nsec = (long) ((ULONGLONG) (seconds * NS_PER_SEC) % NS_PER_SEC);

    return 0;
}

static int
clock_gettime_realtime(struct timespec *tv)
{
    FILETIME ft;
    ULARGE_INTEGER hnsTime;

    GetSystemTimeAsFileTime(&ft);

    hnsTime.LowPart = ft.dwLowDateTime;
    hnsTime.HighPart = ft.dwHighDateTime;

    // To get POSIX Epoch as baseline, subtract the number of hns intervals from Jan 1, 1601 to Jan 1, 1970.
    hnsTime.QuadPart -= (11644473600ULL * HNS_PER_SEC);

    // modulus by hns intervals per second first, then convert to ns, as not to lose resolution
    tv->tv_nsec = (long) ((hnsTime.QuadPart % HNS_PER_SEC) * NS_PER_HNS);
    tv->tv_sec = (long) (hnsTime.QuadPart / HNS_PER_SEC);
    return 0;
}

int
util_clock_gettime(int type, struct timespec *tp)
{
    if (type == CLOCK_MONOTONIC)
    {
        return clock_gettime_monotonic(tp);
    }
    else if (type == CLOCK_REALTIME)
    {
        return clock_gettime_realtime(tp);
    }
    errno = ENOTSUP;
    return -1;
}

uint64_t
util_gen_tstamp(void)
{
    uint64_t ns = 0;
    struct timespec ts;
    if (!clock_gettime_realtime(&ts))
    {   // 1s = le9 ns, but we can only accurately reach 100 ns
        ns = (uint64_t)((ts.tv_sec * (time_t)1e9 + ts.tv_nsec)/100);
    }
    return ns;
}

void
util_lock(volatile long *gcs)
{
    size_t spin_count = 0;
    // Wait until the flag is false.
    while (_InterlockedCompareExchange(gcs, 1, 0) != 0)
    {   // Prevent the loop from being too busy.
        if (spin_count < 32)
        {
            Sleep(0);
        }
        else
        {
            Sleep(1);
        }
        ++spin_count;
    }
}

void
util_unlock(volatile long *gcs)
{
    _InterlockedExchange(gcs, 0);
}

HWND
util_create_tips(HWND hwnd_stc, HWND hwnd, TCHAR* ptext)
{
    if (!(hwnd_stc && hwnd && ptext))
    {
        return NULL;
    }
    // Create the tooltip. g_hInst is the global instance handle.
    HWND htip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, eu_module_handle(), NULL);

    if (!htip)
    {
        return NULL;
    }
    // Associate the tooltip with the tool.
    TOOLINFO toolinfo = {0};
    toolinfo.cbSize = sizeof(TOOLINFO);
    toolinfo.hwnd = hwnd;
    toolinfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolinfo.uId = (LONG_PTR)hwnd_stc;
    toolinfo.lpszText = ptext;
    if (!SendMessage(htip, TTM_ADDTOOL, 0, (LPARAM)&toolinfo))
    {
        DestroyWindow(htip);
        return NULL;
    }
    SendMessage(htip, TTM_ACTIVATE, TRUE, 0);
    SendMessage(htip, TTM_SETMAXTIPWIDTH, 0, 200);
    // Make tip stay 15 seconds
    SendMessage(htip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((15000), (0)));
    return htip;
}

bool
util_under_wine(void)
{
    HMODULE hntdll = NULL;
    if (fn_wine_get_version)
    {
        return true;
    }
    if (!(hntdll = GetModuleHandle(_T("ntdll.dll"))))
    {
        return false;
    }
    if ((fn_wine_get_version = (pwine_get_version)GetProcAddress(hntdll, "wine_get_version")))
    {
        printf("Running on Wine... %s\n", fn_wine_get_version());
        return true;
    }
    return false;
}

bool
util_get_unix_file_name(LPCWSTR path, wchar_t *out, const int len)
{
    HMODULE kernel32 = NULL;
    pwine_get_unix_file_name fn_wine_get_unix_file_name = NULL;
    if (!(kernel32 = GetModuleHandle(_T("kernel32.dll"))))
    {
        return false;
    }
    if (!(fn_wine_get_unix_file_name = (pwine_get_unix_file_name)GetProcAddress(kernel32, "wine_get_unix_file_name")))
    {
        return false;
    }
    if (fn_wine_get_unix_file_name)
    {
        char *unix_path = fn_wine_get_unix_file_name(path);
        if (unix_path)
        {
            util_make_u16(unix_path, out, len);
            util_free(unix_path);
            const wchar_t *p = wcsstr(out, L":");
            if (p && wcslen(p) > 1 && wcsncmp(&p[1], L"/users/", 7) != 0)
            {
                memmove(out, &p[1], (wcslen(p) + 1) * sizeof(wchar_t));
            }
            return !!out[0];
        }
    }
    return false;
}

wchar_t*
util_get_nt_file_name(LPCWSTR path)
{
    wchar_t *nt_path = NULL;
    HMODULE kernel32 = NULL;
    pwine_get_nt_file_name fn_wine_get_nt_file_name = NULL;
    if (!(kernel32 = GetModuleHandle(_T("kernel32.dll"))))
    {
        return NULL;
    }
    if (!(fn_wine_get_nt_file_name = (pwine_get_nt_file_name)GetProcAddress(kernel32, "wine_get_dos_file_name")))
    {
        return NULL;
    }
    if (fn_wine_get_nt_file_name)
    {
        char *unix_path = eu_utf16_utf8(path, NULL);
        if (unix_path)
        {
            nt_path = fn_wine_get_nt_file_name(unix_path);
            free(unix_path);
        }
    }
    return nt_path;
}

int
util_get_hex_byte(const char *p)
{
    int val;
    if (*p >= '0' && *p <= '9')
    {
        val = *p - '0';
    }
    else if (*p >= 'A' && *p <= 'Z')
    {
        val = *p - 'A' + 10;
    }
    else if (*p >= 'a' && *p <= 'z')
    {
        val = *p - 'a' + 10;
    }
    else
    {
        return -1;
    }
    val <<= 4;
    p++;
    if (*p >= '0' && *p <= '9')
    {
        val += *p - '0';
    }
    else if (*p >= 'A' && *p <= 'Z')
    {
        val += *p - 'A' + 10;
    }
    else if (*p >= 'a' && *p <= 'z')
    {
        val += *p - 'a' + 10;
    }
    else
    {
        return -1;
    }
    return val;
}

char *
util_struct_to_string(void *buf, size_t bufsize)
{
    uint8_t *binbuf;
    char *outstring, *p;
    uint32_t sum = 0;
    const char hex[16] = "0123456789ABCDEF";
    /* allocate string buffer for hex chars + checksum hex char + '\0' */
    outstring = (char *)calloc(1, (bufsize*2 + 2 + 1));
    p = outstring;
    for (binbuf = (uint8_t *)buf; binbuf < (uint8_t *)buf+bufsize; binbuf++)
    {
        *p++ = hex[*binbuf >> 4];
        *p++ = hex[*binbuf & 0xf];
        sum += *binbuf;
    }
    /* checksum is sum & 0xff */
    *p++ = hex[(sum & 0xf0) >> 4];
    *p++ = hex[sum & 0xf];
    *p++ = '\0';
    return outstring;
}

bool
util_string_to_struct(const char *buffer, void *buf, size_t len)
{
    char *p;
    int val;
    bool ret = false;
    uint8_t chksum = 0;
    uint8_t *data = buf;
    for (p = (char *)buffer; len; p += 2, len--)
    {
        if ((val = util_get_hex_byte(p)) == -1)
        {
            goto done;
        }
        *data++ = val;
        chksum += val;
    }
    /* retrieve stored checksum value */
    if ((val = util_get_hex_byte(p)) == -1)
    {
        goto done;
    }
    ret = ((uint8_t)val == chksum);
done:
    return ret;
}

void
util_wait_cursor(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_SETCURSOR, (WPARAM) SC_CURSORWAIT, 0);
    }
}

void
util_restore_cursor(eu_tabpage *pnode)
{
    POINT pt;
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_SETCURSOR, (WPARAM) SC_CURSORNORMAL, 0);
        GetCursorPos(&pt);
        SetCursorPos(pt.x, pt.y);
    }
}

HMODULE
util_ssl_open_symbol(char *s[], int n, uintptr_t *pointer)
{
    HMODULE ssl = NULL;
    const TCHAR *ssl_path =
#ifdef _WIN64
    _T("libcrypto-1_1-x64.dll");
#else
    _T("libcrypto-1_1.dll");
#endif
    if ((ssl = np_load_plugin_library(ssl_path)) != NULL)
    {
        for (int i = 0; i < n && s[i][0]; ++i)
        {
            pointer[i] = (uintptr_t)GetProcAddress(ssl, s[i]);
            if (!pointer[i])
            {
                FreeLibrary(ssl);
                ssl = NULL;
            }
        }
    }
    return ssl;
}

void
util_ssl_close_symbol(HMODULE *pssl)
{
    if (pssl && *pssl)
    {
        FreeLibrary(*pssl);
        *pssl = NULL;
    }
}

static bool
util_genarate_key(unsigned char *aes_key, unsigned char *aes_iv)
{
#define CONFIG_KEY_MATERIAL \
    CONFIG_KEY_MATERIAL_SKYLARK CONFIG_KEY_MATERIAL_FILETREE CONFIG_KEY_MATERIAL_TABPAGES CONFIG_KEY_MATERIAL_LEXER
    bool res = false;
    unsigned char sha224[SHA224_DIGEST_LENGTH + 1] = {0};
    char *fn_name[1] = {"SHA224"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = util_ssl_open_symbol(fn_name, 1, pfunc);
    if (hssl && pfunc[0])
    {
        ((eu_crypto_sha224)pfunc[0])((unsigned char *) CONFIG_KEY_MATERIAL, sizeof(CONFIG_KEY_MATERIAL) - 2, sha224);
        memcpy(aes_key, sha224 + (SHA224_DIGEST_LENGTH - AES_BLOCK_SIZE), AES_BLOCK_SIZE);
        memset(sha224, 0, sizeof(sha224));
        ((eu_crypto_sha224)pfunc[0])((unsigned char *) AES_IV_MATERIAL, sizeof(AES_IV_MATERIAL) - 2, sha224);
        memcpy(aes_iv, sha224 + (SHA224_DIGEST_LENGTH - AES_BLOCK_SIZE), AES_BLOCK_SIZE);
        util_ssl_close_symbol(&hssl);
        res = true;
    }
    return res;
}

int
util_aes_enc(unsigned char *dec, unsigned char *enc, int len)
{
    int l;
    AES_KEY k;
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char aes_key[AES_BLOCK_SIZE] = {0};
    unsigned char aes_iv[AES_BLOCK_SIZE] = {0};
    char *fn_name[2] = {"AES_set_encrypt_key", "AES_cbc_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (!util_genarate_key(aes_key, aes_iv))
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    if (((eu_aes_set_encrypt_key)pfunc[0])((const unsigned char *) aes_key, 128, &k) < 0)
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    if (len % AES_BLOCK_SIZE)
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    memcpy(iv, aes_iv, sizeof(iv));
    for (l = 0; l < len; l += AES_BLOCK_SIZE)
    {
        ((eu_aes_cbc_encrypt)pfunc[1])(dec + l, enc + l, AES_BLOCK_SIZE, &k, iv, AES_ENCRYPT);
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

int
util_aes_dec(unsigned char *enc, unsigned char *dec, int len)
{
    int l;
    AES_KEY k;
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char aes_key[AES_BLOCK_SIZE] = {0};
    unsigned char aes_iv[AES_BLOCK_SIZE] = {0};
    char *fn_name[2] = {"AES_set_decrypt_key", "AES_cbc_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (!util_genarate_key(aes_key, aes_iv))
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    if (((eu_aes_set_encrypt_key)pfunc[0])((const unsigned char *) aes_key, 128, &k) < 0)
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    if (len % AES_BLOCK_SIZE)
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    memcpy(iv, aes_iv, sizeof(iv));
    for (l = 0; l < len; l += AES_BLOCK_SIZE)
    {
        ((eu_aes_cbc_encrypt)pfunc[1])(enc + l, dec + l, AES_BLOCK_SIZE, &k, iv, AES_DECRYPT);
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

int
util_enc_des_ecb_192(unsigned char *key_192bits, unsigned char *decrypt, long decrypt_len, unsigned char *encrypt, long *encrypt_len)
{
    unsigned char key[8 + 1];
    DES_key_schedule ks1;
    DES_key_schedule ks2;
    DES_key_schedule ks3;

    unsigned char *decrypt_ptr = NULL;
    long offset;
    unsigned char *encrypt_ptr = NULL;
    unsigned char in[8 + 1];
    unsigned char out[8 + 1];
    char *fn_name[2] = {"DES_set_key_unchecked", "DES_ecb3_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (decrypt_len <= 0 || !encrypt_len)
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    memcpy(key, key_192bits, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks1);
    memcpy(key, key_192bits + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks2);
    memcpy(key, key_192bits + 8 + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks3);
    if (encrypt_len)
    {
        if ((*encrypt_len) < ((decrypt_len - 1) / 8 + 1) * 8)
        {
            util_ssl_close_symbol(&hssl);
            return EUE_OPENSSL_ENC_ERR;
        }
    }
    (*encrypt_len) = 0;
    decrypt_ptr = decrypt;
    encrypt_ptr = encrypt;
    for (offset = 0; offset < decrypt_len; offset += 8)
    {
        memset(in, 0, sizeof(in));
        if (offset + 8 > decrypt_len)
        {
            memcpy(in, decrypt_ptr, decrypt_len - offset);
        }
        else
        {
            memcpy(in, decrypt_ptr, 8);
        }
        memset(out, 0, sizeof(out));
        ((eu_des_ecb3_encrypt)pfunc[1])((const_DES_cblock *) in, (const_DES_cblock *) out, &ks1, &ks2, &ks3, DES_ENCRYPT);
        memcpy(encrypt_ptr, out, 8);

        if (encrypt_len)
        {
            (*encrypt_len) += 8;
        }
        decrypt_ptr += 8;
        encrypt_ptr += 8;
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

int
util_dec_des_ecb_192(unsigned char *key_192bits, unsigned char *encrypt, long encrypt_len, unsigned char *decrypt, long *decrypt_len)
{
    unsigned char key[8 + 1];
    DES_key_schedule ks1;
    DES_key_schedule ks2;
    DES_key_schedule ks3;
    unsigned char *encrypt_ptr = NULL;
    unsigned char *decrypt_ptr = NULL;
    long offset;
    unsigned char in[8 + 1];
    unsigned char out[8 + 1];
    char *fn_name[2] = {"DES_set_key_unchecked", "DES_ecb3_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (encrypt_len <= 0 || !decrypt_len)
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    memcpy(key, key_192bits, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks1);
    memcpy(key, key_192bits + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks2);
    memcpy(key, key_192bits + 8 + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks3);
    if (decrypt_len)
    {
        if ((*decrypt_len) < ((encrypt_len - 1) / 8 + 1) * 8)
        {
            util_ssl_close_symbol(&hssl);
            return EUE_OPENSSL_DEC_ERR;
        }
    }
    (*decrypt_len) = 0;
    encrypt_ptr = encrypt;
    decrypt_ptr = decrypt;
    for (offset = 0; offset < encrypt_len; offset += 8)
    {
        memset(in, 0, sizeof(in));
        if (offset + 8 > encrypt_len)
        {
            memcpy(in, encrypt_ptr, encrypt_len - offset);
        }
        else
        {
            memcpy(in, encrypt_ptr, 8);
        }
        memset(out, 0, sizeof(out));
        ((eu_des_ecb3_encrypt)pfunc[1])((const_DES_cblock *) in, (const_DES_cblock *) out, &ks1, &ks2, &ks3, DES_DECRYPT);
        memcpy(decrypt_ptr, out, 8);
        if (decrypt_len)
        {
            (*decrypt_len) += 8;
        }
        encrypt_ptr += 8;
        decrypt_ptr += 8;
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

int
util_enc_des_cbc_192(unsigned char *key_192bits, unsigned char *decrypt, long decrypt_len, unsigned char *encrypt, long *encrypt_len, unsigned char *init_vector)
{
    unsigned char key[8 + 1];
    DES_key_schedule ks1;
    DES_key_schedule ks2;
    DES_key_schedule ks3;
    unsigned char ivec[24 + 1];
    char *fn_name[2] = {"DES_set_key_unchecked", "DES_ede3_cbc_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (decrypt_len <= 0 || !encrypt_len)
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    memcpy(key, key_192bits, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks1);
    memcpy(key, key_192bits + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks2);
    memcpy(key, key_192bits + 8 + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks3);
    if (encrypt_len)
    {
        if ((*encrypt_len) < ((decrypt_len - 1) / 8 + 1) * 8)
        {
            util_ssl_close_symbol(&hssl);
            return EUE_OPENSSL_ENC_ERR;
        }
    }
    (*encrypt_len) = 0;
    if (init_vector == NULL)
    {
        memset(ivec, 0, sizeof(ivec));
    }
    else
    {
        memcpy(ivec, init_vector, sizeof(ivec) - 1);
    }
    ((eu_des_ede3_cbc_encrypt)pfunc[1])(decrypt, encrypt, decrypt_len, &ks1, &ks2, &ks3, (DES_cblock *) ivec, DES_ENCRYPT);
    if (encrypt_len)
    {
        (*encrypt_len) = decrypt_len;
    }
    if (init_vector)
    {
        memcpy(init_vector, ivec, sizeof(ivec) - 1);
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

int
util_dec_des_cbc_192(unsigned char *key_192bits, unsigned char *encrypt, long encrypt_len, unsigned char *decrypt, long *decrypt_len, unsigned char *init_vector)
{
    unsigned char key[8 + 1];
    DES_key_schedule ks1;
    DES_key_schedule ks2;
    DES_key_schedule ks3;
    unsigned char ivec[24 + 1];
    char *fn_name[2] = {"DES_set_key_unchecked", "DES_ede3_cbc_encrypt"};
    uintptr_t pfunc[2] = {0};
    HMODULE hssl = NULL;
    if (encrypt_len <= 0 || !decrypt_len)
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    if (!util_ssl_open_symbol(fn_name, 2, pfunc))
    {
        return EUE_OPENSSL_DEC_ERR;
    }
    memcpy(key, key_192bits, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks1);
    memcpy(key, key_192bits + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks2);
    memcpy(key, key_192bits + 8 + 8, 8);
    ((eu_des_set_key_unchecked)pfunc[0])((const_DES_cblock *) key, &ks3);
    if (decrypt_len)
    {
        if ((*decrypt_len) < ((encrypt_len - 1) / 8 + 1) * 8)
        {
            util_ssl_close_symbol(&hssl);
            return EUE_OPENSSL_DEC_ERR;
        }
    }
    (*decrypt_len) = 0;
    if (init_vector == NULL)
    {
        memset(ivec, 0, sizeof(ivec));
    }
    else
    {
        memcpy(ivec, init_vector, sizeof(ivec) - 1);
    }
    ((eu_des_ede3_cbc_encrypt)pfunc[1])(encrypt, decrypt, encrypt_len, &ks1, &ks2, &ks3, (DES_cblock *) ivec, DES_DECRYPT);
    if (decrypt_len)
    {
        (*decrypt_len) = encrypt_len;
    }
    if (init_vector)
    {
        memcpy(init_vector, ivec, sizeof(ivec) - 1);
    }
    util_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

static void
do_fp_md5(FILE *f, void **pout)
{
    MD5_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[MD5_DIGEST_LENGTH+1] = {0};
    char text[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"MD5_Init", "MD5_Update", "MD5_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = util_ssl_open_symbol(fn_name, 3, pfunc);
    if (hssl)
    {
        ((eu_md5_init)pfunc[0])(&c);
        for (;;)
        {
            size_t i = fread(buf, 1, BUFF_64K, f);
            if (i <= 0)
            {
                break;
            }
            ((eu_md5_update)pfunc[1])(&c, buf, (unsigned long)i);
        }
        ((eu_md5_final)pfunc[2])(&(md[0]), &c);
        util_hex_expand((char *)md, MD5_DIGEST_LENGTH, text);
        *pout = eu_utf8_utf16(text, NULL);
        util_ssl_close_symbol(&hssl);
    }
}

static void
do_fp_sha1(FILE *f, void **pout)
{
    SHA_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[SHA_DIGEST_LENGTH+1] = {0};
    char text[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"SHA1_Init", "SHA1_Update", "SHA1_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = util_ssl_open_symbol(fn_name, 3, pfunc);
    if (hssl)
    {
        ((eu_sha1_init)pfunc[0])(&c);
        for (;;)
        {
            size_t i = fread(buf, 1, BUFF_64K, f);
            if (i <= 0)
            {
                break;
            }
            ((eu_sha1_update)pfunc[1])(&c, buf, (unsigned long)i);
        }
        ((eu_sha1_final)pfunc[2])(&(md[0]), &c);
        util_hex_expand((char *)md, SHA_DIGEST_LENGTH, text);
        *pout = eu_utf8_utf16(text, NULL);
        util_ssl_close_symbol(&hssl);
    }
}

static void
do_fp_sha256(FILE *f, void **pout)
{
    SHA256_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[SHA256_DIGEST_LENGTH+1] = {0};
    char text[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"SHA256_Init", "SHA256_Update", "SHA256_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = util_ssl_open_symbol(fn_name, 3, pfunc);
    if (hssl)
    {
        ((eu_sha256_init)pfunc[0])(&c);
        for (;;)
        {
            size_t i = fread(buf, 1, BUFF_64K, f);
            if (i <= 0)
            {
                break;
            }
            ((eu_sha256_update)pfunc[1])(&c, buf, (unsigned long)i);
        }
        ((eu_sha256_final)pfunc[2])(&(md[0]), &c);
        util_hex_expand((char *)md, SHA256_DIGEST_LENGTH, text);
        *pout = eu_utf8_utf16(text, NULL);
        util_ssl_close_symbol(&hssl);
    }
}

static void
do_fp_base64(FILE *f, void **pout)
{
    uintptr_t pfunc[10] = {0};
    char *fn_name[10] = {"BIO_new", "BIO_f_base64", "BIO_s_mem","BIO_set_flags", "BIO_push",
                         "BIO_write", "BIO_test_flags", "BIO_ctrl", "BIO_pop", "BIO_free_all"};
    HMODULE hssl = util_ssl_open_symbol(fn_name, 10, pfunc);
    if (hssl)
    {
        int len = 0;
        FILE *fp = f;
        unsigned char *data[64*64] = {0};
        BIO *b64 = ((eu_bio_new)pfunc[0])(((eu_bio_f_base64)pfunc[1])());
        ((eu_bio_set_flags)pfunc[3])(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO *mem = ((eu_bio_new)pfunc[0])(((eu_bio_s_mem)pfunc[2])());
        ((eu_bio_push)pfunc[4])(b64, mem);
        while ((len = (int)fread(data, 1, sizeof data, fp)) > 0)
        {   // write data
            bool done = false;
            int res = 0;
            while(!done)
            {
                res = ((eu_bio_write)pfunc[5])(b64, data, len);
                if(res <= 0) // if failed
                {
                    if(((eu_bio_test_flags)pfunc[6])(b64, BIO_FLAGS_SHOULD_RETRY))
                    {
                        continue;
                    }
                    else
                    {   // encoding failed, Handle Error!
                        len = -2;
                        break;
                    }
                }
                else
                {   // success!
                    done = true;
                }
            }
            if (len == -2)
            {
                break;
            }
        }
        ((eu_bio_ctrl)pfunc[7])(b64, BIO_CTRL_FLUSH, 0, NULL);
        if (len > -2 && *data)
        {
            int out_len = ((eu_bio_ctrl)pfunc[7])(mem, BIO_CTRL_INFO, 0, (char *)(pout));
        }
        ((eu_bio_pop)pfunc[8])(b64);
        ((eu_bio_free_all)pfunc[9])(b64);
        util_ssl_close_symbol(&hssl);
    }
}

static int
do_file_enc(const TCHAR *path, ptr_file_enc fn_do_enc, void **pout)
{
    FILE *fp;
    if ((fp = _tfopen(path, _T("rb"))) == NULL)
    {
        return EUE_OPEN_FILE_ERR;
    }
    fn_do_enc(fp, pout);
    fclose(fp);
    if (STR_IS_NUL(pout))
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    return SKYLARK_OK;
}

int
util_file_md5(const TCHAR *path, TCHAR **pout)
{
    return do_file_enc(path, do_fp_md5, (void **)pout);
}

int
util_file_sha1(const TCHAR *path, TCHAR **pout)
{
    return do_file_enc(path, do_fp_sha1, (void **)pout);
}

int
util_file_sha256(const TCHAR *path, TCHAR **pout)
{
    return do_file_enc(path, do_fp_sha256, (void **)pout);
}

int
util_file_base64(const TCHAR *path, char **pout)
{
    return do_file_enc(path, do_fp_base64, (void **)pout);
}

int
util_hex_expand(char *hex_buf, int hex_len, char *asc_buf)
{
    int i, j = 0;
    char c;
    for (i = 0; i < hex_len; i++)
    {
        c = (hex_buf[i] >> 4) & 0x0f;
        if (c <= 9)
        {
            asc_buf[j++] = c + '0';
        }
        else
        {
            asc_buf[j++] = c + 'A' - 10;
        }
        c = hex_buf[i] & 0x0f;
        if (c <= 9)
        {
            asc_buf[j++] = c + '0';
        }
        else
        {
            asc_buf[j++] = c + 'A' - 10;
        }
    }
    asc_buf[j] = 0;
    return (0);
}

int
util_hex_fold(char *asc_buf, int asc_len, char *hex_buf)
{
    char c;
    char uc;
    int i, j = 0, k = 1, r = 0;
    int txt_len = eu_int_cast(strlen(asc_buf));
    if (txt_len < asc_len || txt_len <= 0)
    {
        return 1;
    }
    uc = 0;
    for (i = 0; i < asc_len; i++)
    {
        if ((asc_buf[i] >= '0') && (asc_buf[i] <= '9'))
        {
            c = (asc_buf[i] - '0') & 0x0f;
        }
        else if ((asc_buf[i] >= 'A') && (asc_buf[i] <= 'F'))
        {
            c = (asc_buf[i] - 'A' + 10) & 0x0f;
        }
        else if ((asc_buf[i] >= 'a') && (asc_buf[i] <= 'f'))
        {
            c = (asc_buf[i] - 'a' + 10) & 0x0f;
        }
        else if (asc_buf[i] == ' ')
        {
            continue;
        }
        else
        {
            r = 1;
            break;
        }
        uc |= (unsigned char) (c << (k * 4));
        k--;
        if (k < 0)
        {
            hex_buf[j] = uc;
            uc = 0;
            k = 1;
            j++;
        }
    }
    if (k == 0)
    {
        hex_buf[j] = uc;
    }
    return (r);
}

time_t
util_last_time(const TCHAR *path)
{
    if (STR_NOT_NUL(path))
    {
        struct _stat buf = {0};
        _tstat(path, &buf);
        return buf.st_mtime;
    }
    return 0;
}

int
util_set_title(const eu_tabpage *pnode)
{
    if (pnode && pnode->pathfile[0])
    {
        size_t len = 0;
        TCHAR *title = NULL;
        const TCHAR *filename = pnode->pathfile;
        bool admin = !util_under_wine() && on_reg_admin();
        if ((len = _tcslen(pnode->pathfile)) > FILESIZE && pnode->filename[0])
        {
            filename = pnode->filename;
        }
        len = _tcslen(filename) + FILESIZE;
        if ((title = (TCHAR *)calloc(sizeof(TCHAR), len + 1)))
        {
            LOAD_APP_RESSTR(IDS_APP_TITLE, app_title);
            if (!filename[0])
            {
                _sntprintf(title, len, admin ? _T("%s [Administrator]") : _T("%s"), app_title);
            }
            else
            {
                _sntprintf(title, len, admin ? _T("%s [Administrator] - %s") : _T("%s - %s"), app_title, filename);
            }
        }
        SetWindowText(eu_module_hwnd(), title);
    }
    return SKYLARK_OK;
}

int
util_set_working_dir(const TCHAR *path, TCHAR **pold)
{
    if (pold)
    {
        uint32_t len = GetCurrentDirectory(0, NULL);
        *pold = len > 0 ? (TCHAR *)calloc(1, (len + 1) * sizeof(TCHAR)) : NULL;
        if (*pold)
        {
            GetCurrentDirectory(len, *pold);
        }
    }
    if (path && path[0] && eu_exist_dir(path))
    {
        SetCurrentDirectory(path);
    }
    else
    {
        TCHAR home_path[MAX_PATH+1] = {0};
        uint32_t len = GetEnvironmentVariable(_T("USERPROFILE"), home_path, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            SetCurrentDirectory(home_path);
        }
    }
    return SKYLARK_OK;
}

bool
util_creater_window(HWND hwnd, HWND hwnd_parent)
{
    RECT rc_wnd, rc_parent;
    POINT pt_new;
    int width;
    int height;
    int parent_width;
    int parent_height;
    if (!IsWindow(hwnd))
    {
        return false;
    }
    if (!IsWindow(hwnd_parent) || 0 == hwnd_parent)
    {
        hwnd_parent = GetDesktopWindow();
    }
    GetWindowRect(hwnd, &rc_wnd);
    GetWindowRect(hwnd_parent, &rc_parent);
    width = rc_wnd.right - rc_wnd.left;
    height = rc_wnd.bottom - rc_wnd.top;
    parent_width = rc_parent.right - rc_parent.left;
    parent_height = rc_parent.bottom - rc_parent.top;
    pt_new.x = rc_parent.left + (parent_width - width) / 2;
    pt_new.y = rc_parent.top + (parent_height - height) / 2;
    return MoveWindow(hwnd, pt_new.x, pt_new.y, width, height, false);
}

/***********************************************************
 * 获得系统剪贴板数据, 调用后需要释放指针
 ***********************************************************/
bool
util_get_clipboard(char **ppstr)
{
    bool ret = false;
    if (!ppstr)
    {
        return ret;
    }
    if (OpenClipboard(NULL))
    {
        HGLOBAL hmem = GetClipboardData(CF_UNICODETEXT);
        if (NULL != hmem)
        {
            wchar_t *lpstr = (wchar_t *) GlobalLock(hmem);
            if (NULL != lpstr)
            {
                if ((*ppstr = eu_utf16_utf8(lpstr, NULL)) != NULL)
                {
                    ret = true;
                }
                GlobalUnlock(hmem);
            }
        }
        CloseClipboard();
    }
    return ret;
}

char *
util_strdup_select(eu_tabpage *pnode, size_t *plen, size_t multiple)
{
    sptr_t text_len;
    sptr_t buf_len;
    char *ptext = NULL;
    if (!pnode)
    {
        return NULL;
    }
    text_len = eu_sci_call(pnode, SCI_GETSELTEXT, 0, 0);
    if (text_len > 0)
    {
        if (multiple > 1)
        {
            buf_len = ((sptr_t) (text_len / multiple)) * multiple + ((text_len % multiple) ? multiple : 0);
        }
        else
        {
            buf_len = text_len;
        }
        if ((ptext = (char *) calloc(1, buf_len + 1 + multiple)) == NULL)
        {
            if (plen)
            {
                (*plen) = (size_t)text_len;
            }
            return NULL;
        }
        eu_sci_call(pnode, SCI_GETSELTEXT, text_len, (sptr_t) ptext);
        if (plen)
        {
            (*plen) = (size_t)buf_len;
        }
        return ptext;
    }
    if (plen)
    {
        (*plen) = 0;
    }
    return NULL;
}

sptr_t
util_line_header(eu_tabpage *pnode, const sptr_t start, const sptr_t end, char **pout)
{
    sptr_t len = 0;
    if (pnode && end > start)
    {
        char *txt = NULL;
        for (len = start; len < end; ++len)
        {
            int ch = (int) eu_sci_call(pnode, SCI_GETCHARAT, len, 0);
            if (!isspace(ch))
            {
                break;
            }
        }
        if (len > start && len < end && pout)
        {
            *pout = on_sci_range_text(pnode, start, len);
        }
        len -= start;
    }
    return len;
}

char *
util_strdup_line(eu_tabpage *pnode, const sptr_t line_number, size_t *plen)
{
    sptr_t line;
    sptr_t text_len;
    sptr_t buf_len = 0;
    char *ptext = NULL;
    if (!pnode)
    {
        return NULL;
    }
    if (line_number < 0)
    {
        sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, cur_pos, 0);
    }
    else
    {
        line = (sptr_t)line_number;
    }
    if (line < 0)
    {
        return NULL;
    }
    text_len = eu_sci_call(pnode, SCI_LINELENGTH, line, 0);
    if (!text_len)
    {
        sptr_t row = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line, 0);
        if (row == -1)
        {
            text_len = -1;
        }
    }
    ptext = text_len >= 0 ? malloc(text_len+3) : NULL;
    if (ptext)
    {
        buf_len = eu_sci_call(pnode, SCI_GETLINE, line, (sptr_t) ptext);
        ptext[buf_len] = 0;
        if (plen)
        {
            *plen = (size_t)buf_len;
        }
        return ptext;
    }
    else
    {
        if (plen)
        {
            *plen = 0;
        }
        return NULL;
    }
}

char *
util_strdup_content(eu_tabpage *pnode, size_t *plen)
{
    char *ptext = NULL;
    size_t total_len = 0;
    if (!pnode)
    {
        return NULL;
    }
    total_len = (size_t)eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
    if (total_len > on_file_get_avail_phys())
    {
        MSG_BOX(IDC_MSG_MEM_NOT_AVAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return NULL;
    }
    ptext = total_len > 0 ? (char *) calloc(1, total_len + 1) : NULL;
    if (!ptext)
    {
        plen ? *plen = 0 : (void)0;
        return NULL;
    }
    else
    {
        if (plen)
        {
            *plen = (size_t)total_len;
        }
    }
    eu_sci_call(pnode, SCI_GETTEXT, (sptr_t)(total_len + 1), (sptr_t)ptext);
    return ptext;
}

/**************************************************
 * 验证字符的有效性
 *************************************************/
bool
util_availed_char(int ch)
{
    if (ch && !isspace(ch))
    {
        return true;
    }
    return false;
}

/**************************************************
 * 去除字符串右边的*号字符
 *************************************************/
void
util_trim_right_star(TCHAR *str)
{
    size_t str_len = _tcslen(str);
    if (str_len > 0 && str[str_len - 1] == _T('*'))
    {
        str[str_len - 1]  = 0;
    }
}

/**************************************************
 * 去除前导空白字符
 * 返回非空白位置开始的指针
 * 设置length为新的字符串长度
 *************************************************/
const char *
util_trim_left_white(const char *s, int *length)
{
    int start = 0, limit = 0;
    if (!length)
    {
        limit = eu_int_cast(strlen(s));
    }
    else
    {
        limit = *length;
    }
    if(limit<=0 || !isspace(s[0]))
    {
        return s;
    }
    while (start < limit && isspace(s[start]))
    {
        ++start;
    }
    if (length)
    {
        *length = limit-start;
    }
    return s+start;
}

/**************************************************
 * 比较两个字符串, s1左侧可能带空白字符
 * 如果两个字符串相似, 返回0, 否则返回1
 * plen, 跳过的空白数
 *************************************************/
int
util_strnspace(const char *s1, const char *s2, int *plen)
{
    if (plen)
    {
        *plen = 0;
    }
    if (!(s1 && s2))
    {
        return 1;
    }
    if (s1 == s2)
    {
        return 0;
    }
    if (strncasecmp(s1, s2, strlen(s2)) == 0)
    {
        return 0;
    }
    for (int i = 0; i < eu_int_cast(strlen(s1)); ++i)
    {
        if (isspace(s1[i]))
        {
            continue;
        }
        if (strncasecmp(&s1[i], s2, strlen(s2)) == 0)
        {
            if (plen)
            {
                *plen = i;
            }
            return 0;
        }
        break;
    }
    return 1;
}

void
util_push_text_dlg(eu_tabpage *pnode, HWND hwnd)
{
    char *text = NULL;
    TCHAR *uni_text = NULL;
    if (pnode && (text = util_strdup_select(pnode, NULL, 0)) && (uni_text = eu_utf8_utf16(text, NULL)))
    {
        SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) uni_text);
        free(text);
        free(uni_text);
    }
}

void
util_enable_menu_item(HMENU hmenu, uint32_t m_id, bool enable)
{
    if (hmenu)
    {
        MENUITEMINFO pmii = {sizeof(MENUITEMINFO), MIIM_STATE,};
        if (GetMenuItemInfo(hmenu, m_id, false, &pmii))
        {
            if (enable)
            {
                bool checked = pmii.fState & MFS_CHECKED;
                if (pmii.fState != MFS_ENABLED)
                {
                    pmii.fState = MFS_ENABLED;
                    if (checked)
                    {
                        pmii.fState |= MFS_CHECKED;
                    }
                    SetMenuItemInfo(hmenu, m_id, false, &pmii);
                }
            }
            else if (pmii.fState != MFS_DISABLED)
            {
                pmii.fState = MFS_DISABLED;
                SetMenuItemInfo(hmenu, m_id, false, &pmii);
            }
        }
    }
}

void
util_set_menu_item(HMENU hmenu, uint32_t m_id, bool checked)
{
    if (hmenu)
    {
        if (checked)
        {
            CheckMenuItem(hmenu, m_id, MF_CHECKED);
        }
        else
        {
            CheckMenuItem(hmenu, m_id, MF_UNCHECKED);
        }
    }
}

void
util_switch_menu_group(HMENU hmenu, int pop_id, uint32_t first_id, uint32_t last_id, uint32_t select)
{
    HMENU htab_next = GetSubMenu(hmenu, pop_id);
    if (htab_next)
    {
        for (uint32_t i = first_id; i <= last_id; ++i)
        {
            util_set_menu_item(htab_next, i, i == select);
        }
    }
}

void
util_update_menu_chars(HMENU hmenu, uint32_t m_id, int width)
{
    if (hmenu)
    {
        TCHAR *pstart = NULL;
        TCHAR *pend = NULL;
        TCHAR m_text[MAX_PATH] = {0};
        TCHAR new_text[MAX_PATH] = {0};
        MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = m_text;
        mii.cch = MAX_PATH - 1;
        GetMenuItemInfo(hmenu, m_id, 0, &mii);
        pstart = _tcschr(m_text, _T('['));
        if (pstart)
        {
            pend = _tcschr(pstart + 1, _T(']'));
            if (pend)
            {
                sntprintf(new_text,
                          _countof(new_text) - 1,
                          _T("%.*s%d%.*s"),
                          pstart - m_text + 1,
                          m_text,
                          width,
                          _tcslen(pend) + 1,
                          pend);
            }
            if (_tcslen(new_text) > 0)
            {
                mii.cch = (uint32_t) _tcslen(new_text);
                mii.dwTypeData = new_text;
                SetMenuItemInfo(hmenu, m_id, 0, &mii);
            }
        }
    }
}

TCHAR *
util_path2unix(TCHAR *path, int len)
{
    if (path)
    {
        for (int i = 0; i < len; ++i)
        {
            if (path[i] == L'\\')
            {
                path[i] = L'/';
            }
        }
        return path;
    }
    return NULL;
}

TCHAR *
util_unix2path(TCHAR *path, int len)
{
    if (path)
    {
        for (int i = 0; i < len; ++i)
        {
            if (path[i] == L'/')
            {
                path[i] = L'\\';
            }
        }
        return path;
    }
    return NULL;
}

char *
util_unix_newline(const char *in, const size_t in_size)
{
    char *res = NULL;
    char *in_ptr = (char *)in;
    size_t resoffset = 0;
    size_t out_size = in_size + MAX_BUFFER;
    char *needle;
    const char *pattern = "\n";
    const char *by = "\r\n";
    if (!(res = (char *)calloc(1, out_size+3)))
    {
        return NULL;
    }
    while ((needle = strstr(in_ptr, pattern)) && resoffset < out_size)
    {
        if (needle - in_ptr > 1 && *(needle-1) == '\r')
        {
            strncpy(res + resoffset, in_ptr, needle - in_ptr + 1);
            resoffset += (needle - in_ptr + 1);
            in_ptr = needle + (int) strlen(pattern);
            continue;
        }
        else
        {
            strncpy(res + resoffset, in_ptr, needle - in_ptr);
            resoffset += needle - in_ptr;
            in_ptr = needle + (int) strlen(pattern);
            strncpy(res + resoffset, by, strlen(by));
            resoffset += (int) strlen(by);
        }
    }
    strcpy(res + resoffset, in_ptr);
    return res;
}

void
util_upper_string(char *str)
{
    char *p = NULL;
    for (p = str; *(p); p++)
    {
        if (islower(*p))
        {
            (*p) = toupper(*p);
        }
    }
}

int
util_query_hostname(char *hostname, char *ip, int len)
{
    WSADATA wsa;
    int err = 0;
    struct in_addr s = {0};
    struct hostent *remote_host = NULL;
    char **ip_addr = NULL;
    if (STR_IS_NUL(hostname))
    {
        return SKYLARK_OK;
    }
    if (inet_pton(AF_INET, hostname, (void *) &s) == 1)
    {   // 正常的ip地址
        strncpy(ip, hostname, len - 1);
        return SKYLARK_OK;
    }
    if ((err = WSAStartup(MAKEWORD(2, 2), &wsa)))
    {
        return EUE_CURL_NETWORK_ERR;
    }
    if (!(remote_host = gethostbyname(hostname)))
    {
        WSACleanup();
        return EUE_CURL_NETWORK_ERR;

    }
    switch (remote_host->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            ip_addr = remote_host->h_addr_list;
            inet_ntop(remote_host->h_addrtype, (*ip_addr), ip, len);
            break;
        default:
            strncpy(ip, hostname, len - 1);
            break;
    }
    WSACleanup();
    return SKYLARK_OK;
}

int
util_effect_line(eu_tabpage *pnode, sptr_t *start_line, sptr_t *end_line)
{
    EU_VERIFY(pnode != NULL);
    sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    sptr_t line_start = eu_sci_call(pnode, SCI_LINEFROMPOSITION, sel_start, 0);
    sptr_t line_end;
    if (sel_end - sel_start > 0)
    {
        line_end = eu_sci_call(pnode, SCI_LINEFROMPOSITION, sel_end, 0);
        if (sel_end == eu_sci_call(pnode, SCI_POSITIONFROMLINE, line_end, 0))
        {
            line_end--;
        }
        if (line_end < line_start)
        {
            line_end = line_start;
        }
    }
    else
    {
        line_end = line_start;
    }
    if (start_line)
    {
        (*start_line) = line_start;
    }
    if (end_line)
    {
        (*end_line) = line_end;
    }
    return 0;
}

TCHAR *
util_make_u16(const char *utf8, TCHAR *utf16, int len)
{
    *utf16 = 0;
    int m = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, len);
    if (m > 0 && m <= len)
    {
        utf16[m-1] = 0;
    }
    else if (len > 0)
    {
        utf16[len-1] = 0;
    }
    return utf16;
}

char *
util_make_u8(const TCHAR *utf16, char *utf8, int len)
{
    *utf8 = 0;
    int m = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, len, NULL, NULL);
    if (m > 0 && m <= len)
    {
        utf8[m-1] = 0;
    }
    else if (len > 0)
    {
        utf8[len-1] = 0;
    }
    return utf8;
}

void
util_kill_thread(uint32_t pid)
{
    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, 0, pid);
    if (handle)
    {
        TerminateThread(handle, 255);
        CloseHandle(handle);
    }
}

bool
util_exist_libcurl(void)
{
    bool ret = false;
    HMODULE lib_symbol = np_load_plugin_library(_T("libcurl.dll"));
    if (lib_symbol)
    {
        ptr_compress fn_compress = (ptr_compress)GetProcAddress(lib_symbol,"zlib_compress2");
        if (fn_compress)
        {
            ret = true;
        }
        FreeLibrary(lib_symbol);
    }
    return ret;
}

bool
util_upcheck_exist(void)
{
    TCHAR name[QW_SIZE + 1] = {0};
    TCHAR upcheck_path[MAX_PATH+1] = {0};
    _sntprintf(upcheck_path, MAX_PATH, _T("%s\\plugins\\%s"), eu_module_path, _T("upcheck.exe"));
    if (eu_exist_file(upcheck_path) && util_product_name(upcheck_path, name, QW_SIZE))
    {
        return (_tcscmp(name, _T("upcheck")) == 0);
    }
    return false;
}

unsigned long
util_compress_bound(unsigned long source_len)
{
    return source_len + (source_len >> 12) + (source_len >> 14) + (source_len >> 25) + 13;
}

int
util_compress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len, int level)
{
    int ret = -2;      // STREAM_ERROR
    HMODULE curl_symbol = np_load_plugin_library(_T("libcurl.dll"));
    if (curl_symbol)
    {
        ptr_compress fn_compress = (ptr_compress)GetProcAddress(curl_symbol,"zlib_compress2");
        if (fn_compress)
        {
            ret = fn_compress(dest, dest_len, source, source_len, level);
        }
        FreeLibrary(curl_symbol);
    }
    return ret;
}

int
util_uncompress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long *source_len)
{
    int ret = -2;      // STREAM_ERROR
    HMODULE curl_symbol = np_load_plugin_library(_T("libcurl.dll"));
    if (curl_symbol)
    {
        ptr_uncompress fn_uncompress = (ptr_uncompress)GetProcAddress(curl_symbol,"zlib_uncompress2");
        if (fn_uncompress)
        {
            ret = fn_uncompress(dest, dest_len, source, source_len);
        }
        FreeLibrary(curl_symbol);
    }
    return ret;
}

HANDLE
util_mk_temp(TCHAR *file_path, TCHAR *ext)
{
    TCHAR temp_path[MAX_PATH+1];
    if (!GetTempPath(MAX_PATH, temp_path))
    {
        return INVALID_HANDLE_VALUE;
    }
    if (!GetTempFileName(temp_path, _T("lua"), 0, file_path))
    {
        printf("GetTempFileName return false\n");
        return INVALID_HANDLE_VALUE;
    }
    if (STR_NOT_NUL(ext))
    {
        DeleteFile(file_path);
        _tcsncat(file_path, ext, MAX_PATH);
        return CreateFile(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return CreateFile(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
}

WCHAR *
util_to_abs(const char *path)
{
    WCHAR *pret = NULL;
    WCHAR lpfile[MAX_PATH+1] = {0};
    if (NULL == path || *path == '\0' || *path == ' ')
    {
        return NULL;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, lpfile, MAX_PATH) || !lpfile[0])
    {
        return NULL;
    }
    util_unix2path(lpfile, eu_int_cast(_tcslen(lpfile)));
    // 如果路径有引号, 去除
    util_wstr_unquote(lpfile, sizeof(lpfile));
    if (lpfile[0] == L'%')
    {
        int n = 1;
        int len = (int)wcslen(lpfile);
        WCHAR env[MAX_PATH + 1] = {0};
        WCHAR buf[FILESIZE + 1] = {0};
        while (lpfile[n] != 0)
        {
            if (lpfile[n++] == L'%')
            {
                break;
            }
        }
        if (n < len && n < FILESIZE)
        {
            _snwprintf(buf, n, L"%s", &lpfile);
        }
        if (wcslen(buf) > 1 && ExpandEnvironmentStrings(buf, env, MAX_PATH) > 0)
        {
            if (lpfile[n] != 0 && lpfile[n] == L'\\')
            {
                ++n;
            }
            wcsncat(env, &lpfile[n], MAX_PATH);
            pret = _wfullpath(NULL, env, MAX_PATH);
        }
    }
    else if (lpfile[0] == L'.')
    {   // 使用了相对路径, 以进程目录为基准, 转为绝对路径
        pret = _wfullpath(NULL, lpfile, MAX_PATH);
    }
    else if (wcslen(lpfile) > 1 && lpfile[1] != L':')
    {   // 在PATH环境变量里, 转为绝对路径
        pret = util_which(lpfile);
    }
    else
    {
        pret = _wcsdup(lpfile);
    }
    return pret;
}

bool
util_can_selections(eu_tabpage *pnode)
{
    sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    return sel_start != sel_end;
}

bool
util_file_size(HANDLE hfile, uint64_t *psize)
{
    if (!GetFileSizeEx(hfile, (LARGE_INTEGER *) psize))
    {
        *psize = 0;
        printf("GetFileSizeEx fail, case: %lu\n", GetLastError());
        return false;
    }
    return true;
}

static void
util_close_stream_by_free(pf_stream pstream)
{
    if (pstream && pstream->base)
    {
        free((void *)pstream->base);
        pstream->base = 0;
        pstream->size = 0;
    }
}

static void
util_close_stream_by_munmap(pf_stream pstream)
{
    if (pstream && pstream->base)
    {
        UnmapViewOfFile((LPCVOID)pstream->base);
        pstream->base = 0;
        pstream->size = 0;
    }
}

bool
util_open_file(LPCTSTR path, pf_stream pstream)
{
    bool ret = false;
    HANDLE hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE != hfile)
    {
        if (!pstream->size)
        {
            if (!util_file_size(hfile, (uint64_t *)&pstream->size))
            {
                CloseHandle(hfile);
                return false;
            }
        }
        if (pstream->size > BUFF_200M)
        {
            HANDLE hmap = NULL;
            if ((hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
            {
                pstream->base = (uintptr_t)MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
                CloseHandle(hmap);
                if (pstream->base)
                {
                    pstream->close = util_close_stream_by_munmap;
                    ret = true;
                    printf("we open file use MapViewOfFile API\n");
                }
            }
        }
        else if ((pstream->base = (uintptr_t)calloc(1, pstream->size > 0 ? pstream->size : 1)) != 0)
        {
            uint32_t bytesread = 0;
            if (ReadFile(hfile, (LPVOID)pstream->base, (uint32_t)pstream->size, &bytesread, NULL))
            {
                pstream->close = util_close_stream_by_free;
                pstream->size = (size_t)bytesread;
                ret = true;
                printf("we open file use ReadFile API, bytesread = %u\n", bytesread);
            }
        }
        CloseHandle(hfile);
    }
    return ret;
}

void
util_set_undo(eu_tabpage *p)
{
    if (p && !eu_sci_call(p, SCI_GETUNDOCOLLECTION, 0 , 0))
    {
        eu_sci_call(p, SCI_SETUNDOCOLLECTION, 1 , 0);
    }
}

void
util_setforce_eol(eu_tabpage *p)
{
    size_t len = 0;
    char *pdata = util_strdup_content(p, &len);
    if (pdata)
    {
        p->eol = on_encoding_line_mode(pdata, len);
        eu_sci_call(p, SCI_SETEOLMODE, p->eol, 0);
        free(pdata);
    }
}

void
util_save_placement(HWND hwnd)
{
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    if (GetWindowPlacement(hwnd, &wp))
    {
        char *placement = util_struct_to_string(&wp, sizeof(wp));
        if (placement)
        {
            if (strlen(placement) < MAX_BUFFER)
            {
                sprintf(eu_get_config()->m_placement, "%s", placement);
            }
            free(placement);
        }
    }
}

void
util_restore_placement(HWND hwnd)
{
    WINDOWPLACEMENT wp = {0};
    if (util_string_to_struct(eu_get_config()->m_placement, &wp, sizeof(wp)))
    {
        if (wp.showCmd == SW_HIDE || wp.showCmd == SW_MINIMIZE || wp.showCmd == SW_SHOWMINIMIZED)
        {
            wp.showCmd = SW_SHOWNORMAL;
        }
        SetWindowPlacement(hwnd, &wp);
    }
}

void
util_untransparent(HWND hwnd)
{
    if (hwnd != NULL)
    {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~0x00080000);
    }
}

void
util_transparent(HWND hwnd, int percent)
{
    if (hwnd)
    {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | 0x00080000);
        if (percent > 255)
        {
            percent = 255;
        }
        if (percent < 0)
        {
            percent = 0;
        }
        SetLayeredWindowAttributes(hwnd, 0, percent, 0x00000002);
    }
}

int
util_count_number(size_t number)
{
    int length = 1;
    while (number /= 10)
    {
        ++length;
    }
    return length;
}

sptr_t
util_select_characters(eu_tabpage *pnode, const sptr_t start, const sptr_t end)
{
    sptr_t len = 0;
    char *buffer = (char *)calloc(1, end - start + 2);
    if (buffer)
    {
        wchar_t *pbuf = NULL;
        Sci_TextRangeFull tr = {{start, end}, buffer};
        eu_sci_call(pnode, SCI_GETTEXTRANGEFULL, 0, (sptr_t) &tr);
        if (*buffer && (pbuf = eu_utf8_utf16(buffer, NULL)))
        {
            len = (sptr_t)wcslen(pbuf);
            free(pbuf);
        }
        free(buffer);
    }
    return len;
}

/* 初始化version.dll里面的三个函数 */
static HMODULE
util_init_verinfo(void)
{
    HMODULE h_ver = LoadLibraryW(L"version.dll");
    if (h_ver != NULL)
    {
        pfnGetFileVersionInfoSizeW = (PFNGFVSW) GetProcAddress(h_ver, "GetFileVersionInfoSizeW");
        pfnGetFileVersionInfoW = (PFNGFVIW) GetProcAddress(h_ver, "GetFileVersionInfoW");
        pfnVerQueryValueW = (PFNVQVW) GetProcAddress(h_ver, "VerQueryValueW");
        if (!(pfnGetFileVersionInfoSizeW && pfnGetFileVersionInfoW && pfnVerQueryValueW))
        {
            FreeLibrary(h_ver);
            h_ver = NULL;
        }
    }
    return h_ver;
}

bool
util_product_name(LPCWSTR filepath, LPWSTR out_string, size_t len)
{
    HMODULE h_ver = NULL;
    bool ret = false;
    DWORD dw_handle = 0;
    DWORD dw_size = 0;
    uint32_t cb_translate = 0;
    LPWSTR pbuffer = NULL;
    PVOID ptmp = NULL;
    WCHAR dw_block[FILESIZE + 1] = { 0 };
    LANGANDCODEPAGE *lptranslate = NULL;
    do
    {
        if ((h_ver = util_init_verinfo()) == NULL)
        {
            break;
        }
        if ((dw_size = pfnGetFileVersionInfoSizeW(filepath, &dw_handle)) == 0)
        {
            printf("pfnGetFileVersionInfoSizeW return false\n");
            break;
        }
        if ((pbuffer = (LPWSTR) calloc(1, dw_size * sizeof(WCHAR))) == NULL)
        {
            break;
        }
        if (!pfnGetFileVersionInfoW(filepath, 0, dw_size, (LPVOID) pbuffer))
        {
            printf("pfnpfnGetFileVersionInfoW return false\n");
            break;
        }
        pfnVerQueryValueW((LPCVOID) pbuffer, L"\\VarFileInfo\\Translation", (LPVOID *) &lptranslate, &cb_translate);
        if (NULL == lptranslate)
        {
            break;
        }
        for (uint16_t i = 0; i < (cb_translate / sizeof(LANGANDCODEPAGE)); i++)
        {
            sntprintf(dw_block,
                      FILESIZE,
                      L"\\StringFileInfo\\%04x%04x\\ProductName",
                      lptranslate[i].wLanguage,
                      lptranslate[i].wCodePage);

            ret = pfnVerQueryValueW((LPCVOID) pbuffer, (LPCWSTR) dw_block, (LPVOID *) &ptmp, &cb_translate);
            if (ret)
            {
                out_string[0] = L'\0';
                wcsncpy(out_string, (LPCWSTR) ptmp, len);
                ret = wcslen(out_string) > 1;
                if (ret) break;
            }
        }
    } while (0);
    eu_safe_free(pbuffer);
    eu_close_dll(h_ver);
    return ret;
}

const uint32_t
util_os_version(void)
{
    typedef void (WINAPI *RtlGetNtVersionNumbersPtr)(DWORD*, DWORD*, DWORD*);
    RtlGetNtVersionNumbersPtr fnRtlGetNtVersionNumbers = NULL;
    uint32_t major_ver, minor_ver, build_num;
    uint32_t ver = 0;
    HMODULE nt_dll = GetModuleHandleW(L"ntdll.dll");
    if (nt_dll)
    {
        fnRtlGetNtVersionNumbers = (RtlGetNtVersionNumbersPtr)GetProcAddress(nt_dll, "RtlGetNtVersionNumbers");
    }
    if (fnRtlGetNtVersionNumbers)
    {
    #define VER_NUM 5
        TCHAR pos[VER_NUM] = { 0 };
        fnRtlGetNtVersionNumbers(&major_ver, &minor_ver,&build_num);
        _sntprintf(pos, VER_NUM, _T("%u%d%u"), major_ver, 0, minor_ver);
        ver = _tcstol(pos, NULL, 10);
    #undef VER_NUM
    }
    return ver;
}

static char *
util_stristr(const char *str, const char *pattern)
{
    char *pptr, *sptr, *start;
    uintptr_t slen, plen;

    for (start = (char *) str, pptr = (char *) pattern, slen = strlen(str), plen = strlen(pattern);
         /* while string length not shorter than pattern length */
         slen >= plen;
         start++, slen--)
    {
        /* find start of pattern in string */
        while (toupper(*start) != toupper(*pattern))
        {
            start++;
            slen--;

            /* if pattern longer than string */
            if (slen < plen) return (NULL);
        }

        sptr = start;
        pptr = (char *) pattern;
        while (toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;
            /* if end of pattern then pattern was found */
            if ('\0' == *pptr) return (start);
        }
    }
    return (NULL);
}

static inline char *
util_search_case(const char *str, const char *pattern, bool incase)
{
    return (incase ? util_stristr(str, pattern) : (char *)strstr(str, pattern));
}

static inline bool
util_punct_or_space(int ch)
{
    return isspace(ch) || ispunct(ch);
}

char *
util_string_match(const char *str, const char *pattern, bool incase, bool match_start, bool whole)
{
    const char *psrc = str;
    int slen =0, plen = (int)strlen(pattern);
    char *presult = util_search_case(str, pattern, incase);
    while (match_start && presult && presult - psrc > 0)
    {
        presult = NULL;
        while(*psrc && !isspace(*psrc))
        {
            ++psrc;
        }
        while(*psrc && util_punct_or_space(*psrc))
        {
            ++psrc;
        }
        presult = util_search_case(psrc, pattern, incase);
    }
    psrc = match_start ? presult : str;
    slen = presult ? (int)strlen(presult) : 0;
    while(whole && presult && (presult - psrc > 0 || (slen > plen && !util_punct_or_space(presult[plen]))))
    {
        presult = NULL;
        while(*psrc && !isspace(*psrc))
        {
            ++psrc;
        }
        while(*psrc && util_punct_or_space(*psrc))
        {
            ++psrc;
        }
        presult = util_search_case(psrc, pattern, incase);
        slen = presult ? (int)strlen(presult) : 0;
    }
    return presult;
}

TCHAR *
util_add_double_quotes(const TCHAR *path)
{
    TCHAR *buf = NULL;
    if (path)
    {
        int len = eu_int_cast(_tcslen(path)) + 4;
        if ((buf = (TCHAR *)calloc(sizeof(TCHAR), len + 1)) != NULL)
        {
            _sntprintf(buf, len, _T("\"%s\""), path);
        }
    }
    return buf;
}

wchar_t *
util_wstr_unquote(wchar_t *path, const int size)
{
    if (STR_NOT_NUL(path) && size > 0)
    {
        if ((path[0] == L'"') || path[0] == L'\'')
        {
            memmove(path, &path[1], size - sizeof(wchar_t));
            int len = (int)wcslen(path);
            if (len > 0 && (path[len - 1] == L'"' || path[len - 1] == L'\''))
            {
                path[len - 1] = 0;
            }
        }
    }
    return path;
}

char *
util_str_unquote(const char *path)
{
    char *buf = NULL;
    if (path)
    {
        if ((path[0] == '"' || path[0] == '\''))
        {
            buf = (char *)_strdup(&path[1]);
            int len = buf ? eu_int_cast(strlen(buf)) : 0;
            if (len > 0 && (buf[len - 1] == '"' || buf[len - 1] == '\''))
            {
                buf[len - 1] = 0;
            }
        }
        else
        {
            buf = (char *)_strdup(path);
        }
    }
    return buf;
}

void
util_skip_whitespace(uint8_t **cp, int n, int term)
{
    uint8_t *pstr = *cp;
    while (isspace(*pstr) && *pstr != term && n--)
    {
        pstr++;
    }
    *cp = pstr;
}

static inline bool
util_main_window(HWND handle)
{
    return (GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle));
}

static int CALLBACK
util_enum_callback(HWND handle, LPARAM lparam)
{
    handle_data *pdata = (handle_data *)lparam;
    uint32_t process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (pdata->pid != process_id || !util_main_window(handle))
    {
        return 1;
    }
    pdata->handle = handle;
    return 0;
}

HWND
util_get_hwnd(const uint32_t pid)
{
    handle_data data = {pid, 0};
    EnumWindows(util_enum_callback, (LPARAM)&data);
    return data.handle;
}

TCHAR *
util_which(const TCHAR *name)
{
    bool diff = false;
    bool add_suf = true;
    TCHAR *file = NULL;
    TCHAR *env_path = NULL;
    TCHAR sz_work[MAX_PATH + 1] = {0};
    const TCHAR *delimiter = _T(";");
    TCHAR *path = _tgetenv(_T("PATH"));
    TCHAR *av[] = {_T(".exe"), _T(".com"), _T(".cmd"), _T(".bat"), NULL};
    TCHAR *dot = _tcsrchr(name, _T('.'));
    int len = eu_int_cast(_tcslen(path)) + (3 * MAX_PATH);
    if (!path)
    {
        return NULL;
    }
    GetSystemDirectory(sz_work, MAX_PATH);
    if (!sz_work[0] || !eu_module_path[0])
    {
        return NULL;
    }
    if ((env_path = (TCHAR *)calloc(sizeof(TCHAR), len + 1)) == NULL)
    {
        return NULL;
    }
    diff = _tcscmp(sz_work, eu_module_path) != 0;
    if (dot)
    {
        for (int i = 0; av[i]; ++i)
        {
            if (_tcsicmp(dot, av[i]) == 0)
            {
                add_suf = false;
                break;
            }
        }
    }
    if ((file = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH)) != NULL)
    {
        if (diff)
        {
            _sntprintf(env_path, len, _T("%s;%s;%s\\plugins;%s"), path, eu_module_path, eu_module_path, sz_work);
        }
        else
        {
            _sntprintf(env_path, len, _T("%s;%s;%s\\plugins"), path, eu_module_path, eu_module_path);
        }
        bool quote = false;
        wchar_t *tok = _tcstok(env_path, delimiter);
        while (tok)
        {
            int i = 0;
            int blen = eu_int_cast(wcslen(tok));
            if (blen > 0 && tok[blen - 1] == '"')
            {
                tok[blen - 1] = 0;
                quote = true;
            }
            else
            {
                quote = false;
            }
            _sntprintf(file, MAX_PATH, _T("%s\\%s"), quote? &tok[1] : tok, name);
            do
            {
                struct _stat st;
                if (_tstat(file, &st) != -1 && (st.st_mode & S_IEXEC))
                {
                    eu_safe_free(env_path);
                    return file;
                }
                if (add_suf && av[i])
                {
                    _sntprintf(file, MAX_PATH, _T("%s\\%s%s"), quote? &tok[1] : tok, name, av[i]);
                }
            } while (av[i++]);
            tok = _tcstok(NULL, delimiter);
        }
        eu_safe_free(file);
    }
    eu_safe_free(env_path);
    return NULL;
}

int
eu_prepend_path(const TCHAR *dir)
{
    size_t bufsize;
    TCHAR *path;
    TCHAR *value = NULL;
    int    rc = -1;
    if ((path = _tgetenv(_T("PATH"))) == NULL)
    {
        return (-1);
    }
    bufsize = _tcslen(dir) + _tcslen(path) + 8;
    value = (TCHAR *)calloc(1, bufsize * sizeof(TCHAR));
    if (value && _sntprintf(value, bufsize, _T("PATH=%s;%s"), dir, path) > 0)
    {
        rc = _tputenv(value);
    }
    eu_safe_free(value);
    return rc;
}

bool
eu_gui_app(void)
{
    HMODULE hmodule = GetModuleHandle(NULL);
    if(hmodule == NULL)
    {
        return false;
    }
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER *)hmodule;
    IMAGE_NT_HEADERS* pe_header =(IMAGE_NT_HEADERS *)((uint8_t *)dos_header + dos_header->e_lfanew);
    return pe_header->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;
}

bool
util_delete_file(LPCTSTR filepath)
{
    int ret = 1;
    int path_len = eu_int_cast(_tcslen(filepath));
    TCHAR *psz_from = (TCHAR *)calloc(sizeof(TCHAR), path_len + 2);
    if (psz_from)
    {
        _tcscpy(psz_from, filepath);
        psz_from[path_len] = 0;
        psz_from[++path_len] = 0;
        SHFILEOPSTRUCT st_struct = {0};
        st_struct.wFunc = FO_DELETE;
        st_struct.pFrom = psz_from;
        st_struct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_SILENT | FOF_FILESONLY;
        st_struct.fAnyOperationsAborted = 0;
        ret = SHFileOperation(&st_struct);
        free(psz_from);
    }
    return (ret == 0);
}

bool
util_file_access(LPCTSTR filename, uint32_t *pgranted)
{
    bool ret = false;
    uint32_t length = 0;
    if (!GetFileSecurity(filename, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, NULL, 0, &length) &&
        ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
        PSECURITY_DESCRIPTOR security = (PSECURITY_DESCRIPTOR) malloc(length);
        if (security &&
            GetFileSecurity(filename, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, security, length, &length))
        {
            HANDLE old_token = NULL;
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &old_token))
            {
                HANDLE token = NULL;
                uint32_t access_mask = MAXIMUM_ALLOWED;
                if (DuplicateToken(old_token, SecurityImpersonation, &token))
                {
                    GENERIC_MAPPING mapping = {0xFFFFFFFF};
                    PRIVILEGE_SET privileges = {0};
                    uint32_t granted_access = 0, privileges_length = sizeof(privileges);
                    BOOL result = FALSE;
                    mapping.GenericRead = FILE_GENERIC_READ;
                    mapping.GenericWrite = FILE_GENERIC_WRITE;
                    mapping.GenericExecute = FILE_GENERIC_EXECUTE;
                    mapping.GenericAll = FILE_ALL_ACCESS;
                    MapGenericMask(&access_mask, &mapping);
                    if (AccessCheck(security, token, access_mask, &mapping, &privileges, &privileges_length, &granted_access, &result))
                    {
                        ret = (result == TRUE);
                        *pgranted = granted_access;
                    }
                    CloseHandle(token);
                }
                CloseHandle(old_token);
            }
        }
        eu_safe_free(security);
    }
    return ret;
}

int
util_split(const char *pstr, char (*pout)[MAX_PATH], int ch)
{
    if (STR_NOT_NUL(pstr) && NULL != pout)
    {
        const int len = (int)strlen(pstr);
        if (len < MAX_PATH)
        {
            bool split = false;
            char *tmp = (char *)pstr;
            int i = 0, j = 0, row = 0;
            for (; i < TWO_DISM && j < len; ++j)
            {
                if (ch == tmp[j])
                {
                    if (!split)
                    {
                        *(*(pout + i) + row) = '\0';
                        split = true;
                        row = 0;
                        ++i;
                    }
                }
                else
                {
                    if (!row)
                    {
                        memset(pout[i], 0, MAX_PATH);
                    }
                    *(*(pout + i) + row) = tmp[j];
                    split = false;
                    ++row;
                }
            }
        }
    }
    return 0;
}

int
util_strim_end(char *pstr, int len)
{
    if (pstr && len > 0)
    {
        char *p = NULL;
        if ((p = strstr(pstr, BAD_CHARACTERS)) != NULL)
        {
            len = eu_int_cast(p - pstr);
            pstr[len] = 0;
        }
        while (len > 0 && (p = strchr(END_CHARACTERS, pstr[len - 1])))
        {
            pstr[len - 1] = 0;
            --len;
        }
    }
    return 0;
}

HFONT
util_create_font(const char* name, const int font_size, const bool bold)
{
    LOGFONT lf = {FONT_SIZE_DPI(font_size)};
    lf.lfWidth = 0;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    util_make_u16(name, lf.lfFaceName, _countof(lf.lfFaceName)-1);
    return CreateFontIndirectW(&lf);
}

const TCHAR*
util_path_ext(const TCHAR *path)
{
    if (STR_NOT_NUL(path))
    {
        TCHAR *p = _tcsrchr(path, _T('.'));
        if (p && _tcslen(p) > 1)
        {
            return (const TCHAR *)&p[1];
        }
    }
    return NULL;
}

void
util_postion_xy(eu_tabpage *pnode, sptr_t pos, sptr_t *px, sptr_t *py)
{
    if (pnode)
    {
        if (pos < 0 && (pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0)) < 0)
        {
            pos = eu_sci_call(pnode, SCI_GETANCHOR, 0, 0);
        }
        if (pos >= 0)
        {
            *px = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            *py = eu_sci_call(pnode, SCI_POSITIONFROMLINE, *px, 0);
            (*px) += 1;
            (*py) = pos - (*py) + 1;
        }
    }
}

void
util_explorer_open(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (!util_under_wine())
        {
            LPITEMIDLIST dir = NULL;
            LPITEMIDLIST item = NULL;
            do
            {
                if (!(dir = ILCreateFromPathW(pnode->pathname)))
                {
                    break;
                }
                if (!(item = ILCreateFromPathW(pnode->pathfile)))
                {
                    break;
                }
                LPCITEMIDLIST selection[] = {item};
                uint32_t count = _countof(selection);
                HRESULT hr = SHOpenFolderAndSelectItems(dir, count, selection, 0);
            } while(0);
            if (dir)
            {
                CoTaskMemFree(dir);
            }
            if (item)
            {
                CoTaskMemFree(item);
            }
        }
        else
        {
            wchar_t cmd_exec[MAX_BUFFER] = {0};
            wchar_t plugin[MAX_PATH] = {0};
            wchar_t unix_path[MAX_PATH] = {0};
            if (util_get_unix_file_name(pnode->pathfile, unix_path, MAX_PATH - 1))
            {
                _sntprintf(plugin, MAX_PATH - 1, L"%s\\plugins\\np_winexy.dll", eu_module_path);
                util_path2unix(plugin, eu_int_cast(_tcslen(plugin)));
            #if (defined _M_X64) || (defined __x86_64__)
                _sntprintf(cmd_exec, MAX_BUFFER - 1, L"%s \"%s\" %s \"%s\"", L"/bin/wine64", plugin, L"explorer.exe", unix_path);
            #else
                _sntprintf(cmd_exec, MAX_BUFFER - 1, L"%s \"%s\" %s \"%s\"", L"/bin/wine", plugin, L"explorer.exe", unix_path);
            #endif
                CloseHandle(eu_new_process(cmd_exec, NULL, pnode->pathname, 2, NULL));
            }
        }
    }
}

int
util_num_cores(void)
{
    SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);
    return (int)sysinfo.dwNumberOfProcessors;
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * c风格的字符串替换函数, 字符串长度不限
 * 返回值是替换后的字符串, 失败返回空指针
 ** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
char*
util_str_replace(const char *in, const char *pattern, const char *by)
{
    size_t offset = 0;
    char *needle = NULL;
    char *in_ptr = (char *)in;
    const int diff = (int)strlen(by) - (int)strlen(pattern);
    const size_t in_size = (strlen(in) * (size_t)(diff > 0 ? diff + 1 : 1) + 2);
    char *res = (char *)calloc(1, in_size + 1);
    if (res)
    {
        while ((needle = strstr(in_ptr, pattern)) && offset < in_size)
        {
            strncpy(res + offset, in_ptr, needle - in_ptr);
            offset += needle - in_ptr;
            in_ptr = needle + (int) strlen(pattern);
            strncpy(res + offset, by, in_size - offset);
            offset += (int) strlen(by);
        }
        strncpy(res + offset, in_ptr, in_size - offset);
    }
    return res;
}

char*
util_url_escape(const char *url)
{
    char *result = NULL;
    if (url)
    {
        int j = 0;
        const int m_size = (int)strlen(url);
        if ((result = (char *) malloc(3 * m_size + 1)) == NULL)
        {
            return NULL;
        }
        for (int i = 0; i < m_size; ++i)
        {
            uint8_t ch = (uint8_t)url[i];
            if (ch == ' ' || ch == '#' || ch == '?' || ch == '&' || ch == '=' || ch == '+' || ch == '$' ||
                ch == ',' || ch == '%' || ch == '<' || ch == '>' || ch == '~' || ch == ';')
            {
                _snprintf(result + j, 3 * m_size - j, "%%%02X", ch);
                j += 3;
            }
            else
            {
                result[j++] = ch;
            }
        }
        result[j] = '\0';
    }
    return result;
}

bool
util_try_path(LPCTSTR dir)
{
#define LEN_NAME 6
    HANDLE pfile = INVALID_HANDLE_VALUE;
    TCHAR dist_path[MAX_BUFFER] = {0};
    TCHAR temp[LEN_NAME + 1] =  {0};
    if (eu_exist_dir(dir) || eu_mk_dir(dir))
    {
        _sntprintf(dist_path, MAX_BUFFER, _T("%s\\%s"), dir, eu_rand_str(temp, LEN_NAME));
        pfile = CreateFile(dist_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (pfile == INVALID_HANDLE_VALUE)
        {
            printf("%s, create folder failed\n", __FUNCTION__);
        }
        CloseHandle(pfile);
    }
    return (pfile != INVALID_HANDLE_VALUE);
#undef LEN_NAME
}

bool
util_shell_path(const GUID *folder, TCHAR *path, const int len)
{
    if (path && len > 0)
    {
        int m = 0;
        TCHAR *tmp = NULL;
        uint32_t flags = KF_FLAG_SIMPLE_IDLIST | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_ALIAS;
        if (!SUCCEEDED(SHGetKnownFolderPath(folder, flags, NULL, &tmp)))
        {
            return false;
        }
        m = _sntprintf(path, len, _T("%s"), tmp);
        CoTaskMemFree(tmp);
        return (m > 0 && m < len);
    }
    return false;
}
