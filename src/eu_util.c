/******************************************************************************
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

typedef const char *(__cdecl *pwine_get_version)(void);
typedef void (*ptr_do_enc)(FILE *f, TCHAR *out, int out_len);
typedef unsigned long (*ptr_compress_bound)(unsigned long source_len);
typedef int (*ptr_compress)(uint8_t *, unsigned long *, const uint8_t *, unsigned long, int);
typedef int (*ptr_uncompress)(uint8_t *, unsigned long *, const uint8_t *, unsigned long *);

#define BUFF_64K 0x10000
#define BUFF_200M 0xc800000
#define AES_IV_MATERIAL "copyright by skylark team"
#define CONFIG_KEY_MATERIAL_SKYLARK    "EU_SKYLARK"

static pwine_get_version fn_wine_get_version;
static volatile long gth_locked = 0;

static void 
util_thread_lock(void)
{
    size_t spin_count = 0;
    // Wait until the flag is false.
    while (_InterlockedCompareExchange(&gth_locked, 1, 0) != 0)
    {
        // Prevent the loop from being too busy.
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

static inline void 
util_thread_unlock(void)
{
    _InterlockedExchange(&gth_locked, 0);
}

bool
util_under_wine(void)
{
    HMODULE hntdll = NULL;
    if (fn_wine_get_version)
    {
        return true;
    }
    if(!(hntdll = GetModuleHandle(_T("ntdll.dll"))))
    {
        return false;
    }
    if((fn_wine_get_version = (pwine_get_version)GetProcAddress(hntdll, "wine_get_version")))
    {
        printf("Running on Wine... %s\n", fn_wine_get_version());
        return true;
    }
    return false;
}

static int 
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
    eu_sci_call(pnode, SCI_SETCURSOR, (WPARAM) SC_CURSORWAIT, 0);
}

void
util_restore_cursor(eu_tabpage *pnode)
{
    POINT pt;
    eu_sci_call(pnode, SCI_SETCURSOR, (WPARAM) SC_CURSORNORMAL, 0);
    GetCursorPos(&pt);
    SetCursorPos(pt.x, pt.y);
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
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);
    if (hssl && pfunc[0])
    {
        ((eu_crypto_sha224)pfunc[0])((unsigned char *) CONFIG_KEY_MATERIAL, sizeof(CONFIG_KEY_MATERIAL) - 2, sha224);
        memcpy(aes_key, sha224 + (SHA224_DIGEST_LENGTH - AES_BLOCK_SIZE), AES_BLOCK_SIZE);
        memset(sha224, 0, sizeof(sha224));
        ((eu_crypto_sha224)pfunc[0])((unsigned char *) AES_IV_MATERIAL, sizeof(AES_IV_MATERIAL) - 2, sha224);
        memcpy(aes_iv, sha224 + (SHA224_DIGEST_LENGTH - AES_BLOCK_SIZE), AES_BLOCK_SIZE);     
        eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
    eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
    eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
            eu_ssl_close_symbol(&hssl);
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
    eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
            eu_ssl_close_symbol(&hssl);
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
    eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
            eu_ssl_close_symbol(&hssl);
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
    eu_ssl_close_symbol(&hssl);
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
    if (!eu_ssl_open_symbol(fn_name, 2, pfunc))
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
            eu_ssl_close_symbol(&hssl);
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
    eu_ssl_close_symbol(&hssl);
    return SKYLARK_OK;
}

static void 
do_fp_md5(FILE *f, TCHAR *out, int out_len)
{
    MD5_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[MD5_DIGEST_LENGTH+1] = {0};
    char text[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"MD5_Init", "MD5_Update", "MD5_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 3, pfunc);
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
        MultiByteToWideChar(CP_UTF8, 0, text, -1, out, out_len);
        eu_ssl_close_symbol(&hssl);
    }
}

static void 
do_fp_sha1(FILE *f, TCHAR *out, int out_len)
{
    SHA_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[SHA_DIGEST_LENGTH+1] = {0};
    char text[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"SHA1_Init", "SHA1_Update", "SHA1_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 3, pfunc);
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
        MultiByteToWideChar(CP_UTF8, 0, text, -1, out, out_len);
        eu_ssl_close_symbol(&hssl);
    }
}

static void 
do_fp_sha256(FILE *f, TCHAR *out, int out_len)
{
    SHA256_CTX c;
    uint8_t buf[BUFF_64K];
    uint8_t md[SHA256_DIGEST_LENGTH+1] = {0};
    char text[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    char *fn_name[3] = {"SHA256_Init", "SHA256_Update", "SHA256_Final"};
    uintptr_t pfunc[3] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 3, pfunc);
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
        MultiByteToWideChar(CP_UTF8, 0, text, -1, out, out_len);
        eu_ssl_close_symbol(&hssl);
    }
}

static int
do_file_enc(const TCHAR *path, ptr_do_enc fn_do_enc, TCHAR *out, int out_len)
{
    FILE *fp;
    out[0] = 0;
    if ((fp = _tfopen(path, _T("rb"))) == NULL)
    {
        return EUE_OPEN_FILE_ERR;
    }
    fn_do_enc(fp, out, out_len);
    fclose(fp);
    if (!out[0])
    {
        return EUE_OPENSSL_ENC_ERR;
    }
    return SKYLARK_OK;
}

int
util_file_md5(const TCHAR *path, TCHAR *out, int out_len)
{
    return do_file_enc(path, do_fp_md5, out, out_len);
}

int
util_file_sha1(const TCHAR *path, TCHAR *out, int out_len)
{
    return do_file_enc(path, do_fp_sha1, out, out_len);
}

int
util_file_sha256(const TCHAR *path, TCHAR *out, int out_len)
{
    return do_file_enc(path, do_fp_sha256, out, out_len);
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
    int i, j = 0, k = 1, r = 0;
    char c;
    char uc;
    if (((int) strlen(asc_buf) < asc_len) || (strlen(asc_buf) == 0))
    {
        return -1;
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
        else
        {
            r = -1;
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
    if (path[0])
    {
        struct _stat buf = {0};
        _tstat(path, &buf);
        return buf.st_mtime;
    }
    return SKYLARK_OK;
}

int
util_set_title(const TCHAR *filename)
{
    TCHAR title[100 + MAX_PATH];
    LOAD_APP_RESSTR(IDS_APP_TITLE, app_title);
    if (filename && filename[0])
    {
        _sntprintf(title, _countof(title) - 1, _T("%s - %s"), app_title, filename);
    }
    else
    {
        _sntprintf(title, _countof(title) - 1, _T("%s"), app_title);
    }
    SetWindowText(eu_module_hwnd(), title);
    return SKYLARK_OK;
}

int
util_set_working_dir(const TCHAR *path)
{
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

char *
util_strdup_line(eu_tabpage *pnode, size_t *plen)
{
    sptr_t line;
    sptr_t text_len;
    sptr_t cur_pos;
    sptr_t buf_len = 0;
    char *ptext = NULL;
    if (!pnode)
    {
        return NULL;
    }
    cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, cur_pos, 0);
    text_len = eu_sci_call(pnode, SCI_LINELENGTH, line, 0);
    ptext = text_len > 0 ? malloc(text_len+1) : NULL;
    buf_len = eu_sci_call(pnode, SCI_GETLINE, line, (sptr_t) ptext);
    if (ptext)
    {
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
util_trim_left_white(const char *s, int32_t *length)
{
    int32_t start = 0;
    int32_t limit = 0;
    if (!length)
    {
        limit = (int32_t)strlen(s);
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

void
util_replace_newline(char *str)
{
    char *p = NULL;
    while ((p = strstr(str, "\\n")))
    {
        *(p) = '\n';
        memmove(p + 1, p + 2, strlen(p + 2) + 1);
    }
}

TCHAR *
util_wchr_replace(TCHAR *path)
{
    TCHAR *lp = NULL;
    intptr_t pos;
    do
    {
        lp = _tcschr(path, _T('/'));
        if (lp)
        {
            pos = lp - path;
            path[pos] = _T('\\');
        }
    } while (lp != NULL);
    return path;
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
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, len);
    return utf16;
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
    HMODULE lib_symbol = NULL;
    TCHAR curl_path[MAX_PATH+1] = {0};
    _sntprintf(curl_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("libcurl.dll"));
    if ((lib_symbol = LoadLibrary(curl_path)) != NULL)
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

unsigned long 
util_compress_bound(unsigned long source_len)
{
    return source_len + (source_len >> 12) + (source_len >> 14) + (source_len >> 25) + 13;
}

int
util_compress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len, int level)
{
    int ret = -2;      // STREAM_ERROR
    HMODULE curl_symbol = NULL;
    TCHAR curl_path[MAX_PATH+1] = {0};
    _sntprintf(curl_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("libcurl.dll"));
    if ((curl_symbol = LoadLibrary(curl_path)) != NULL)
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
    HMODULE curl_symbol = NULL;
    TCHAR curl_path[MAX_PATH+1] = {0};
    _sntprintf(curl_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("libcurl.dll"));
    if ((curl_symbol = LoadLibrary(curl_path)) != NULL)
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
        return NULL;
    }
    if (!GetTempFileName(temp_path, _T("lua"), 0, file_path))
    {
        printf("GetTempFileName return false\n");
        return NULL;
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
    WCHAR old_folder[MAX_PATH+1] = {0};
    if (NULL == path || *path == '\0' || *path == ' ')
    {
        return NULL;
    }
    if (!GetCurrentDirectory(MAX_PATH, old_folder))
    {
        return NULL;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, lpfile, MAX_PATH))
    {
        return NULL;
    }
    util_wchr_replace(lpfile);
    // 进程当前目录为基准, 得到绝对路径
    SetCurrentDirectory(eu_module_path);
    if (lpfile[0] == _T('%'))
    {
        int n = 1;
        int len = (int)wcslen(lpfile);
        WCHAR env[MAX_PATH + 1] = {0};
        WCHAR buf[FILESIZE + 1] = {0};
        while (lpfile[n] != 0)
        {
            if (lpfile[n++] == _T('%'))
            {
                break;
            }
        }
        if (n < len && n < FILESIZE)
        {
            _sntprintf(buf, n, _T("%s"), &lpfile);
        }
        if (wcslen(buf) > 1 && ExpandEnvironmentStringsW(buf, env, MAX_PATH) > 0)
        {
            if (lpfile[n] != 0 && lpfile[n] == _T('\\'))
            {
                ++n;
            }
            wcsncat(env, &lpfile[n], MAX_PATH);
            pret = _wfullpath(NULL, env, MAX_PATH);
        }
    }  // 使用unix路径.代表当前目录
    else if (lpfile[0] == _T('.'))
    {
        pret = _wfullpath(NULL, lpfile, MAX_PATH);
    }
    else
    {
        pret = _wcsdup(lpfile);
    }
    SetCurrentDirectory(old_folder);
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
        return false;
    }
    return true;
}

static void
util_close_stream_by_free(pt_stream pstream)
{
    if (pstream && pstream->base)
    {
        free((void *)pstream->base);
        pstream->base = 0;
        pstream->size = 0;
    }
}

static void
util_close_stream_by_munmap(pt_stream pstream)
{
    if (pstream && pstream->base)
    {
        UnmapViewOfFile((LPCVOID)pstream->base);
        pstream->base = 0;
        pstream->size = 0;
    }
}

bool
util_open_file(LPCTSTR path, pt_stream pstream)
{
    bool ret = false;
    HANDLE hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE != hfile)
    {
    	if (!pstream->size)
    	{
    		if (!util_file_size(hfile, &pstream->size))
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
        else if ((pstream->base = (uintptr_t)calloc(1, pstream->size)) != 0)
        {
            uint32_t bytesread = 0;
            if (ReadFile(hfile, (LPVOID)pstream->base, (uint32_t)pstream->size, &bytesread, NULL))
            {
                pstream->close = util_close_stream_by_free;
                pstream->size = (size_t)bytesread;
                ret = true;
                printf("we open file use ReadFile API\n");
            }
        }
        CloseHandle(hfile);
    }
    return ret;
}

void
util_setforce_eol(eu_tabpage *pnode)
{
	size_t len = 0;
	char *pdata = util_strdup_content(pnode, &len);
	if (pdata)
	{
		pnode->eol = on_encoding_line_mode(pdata, len);
		eu_sci_call(pnode, SCI_SETEOLMODE, pnode->eol, 0);
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
