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

#ifndef _EU_LOG_H_
#define _EU_LOG_H_

#ifndef MAX_OUTPUT_BUF
#define MAX_OUTPUT_BUF 1024*64
#endif

#ifndef STRCMP
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef STRICMP
#if ( defined _WIN32 )
#define STRICMP(_a_,_C_,_b_) ( stricmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strnicmp(_a_,_b_,_n_) _C_ 0 )
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
#define STRICMP(_a_,_C_,_b_) ( strcasecmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strncasecmp(_a_,_b_,_n_) _C_ 0 )
#endif
#endif

#ifndef MEMCMP
#define MEMCMP(_a_,_C_,_b_,_n_) ( memcmp(_a_,_b_,_n_) _C_ 0 )
#endif

#if defined(_MSC_VER)

#if !defined(__cplusplus)
#define inline __inline
#endif

#ifndef va_copy
#define va_copy(d,s) ((d) = (s))
#endif

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define fseek _fseeki64

#ifdef _stat
#undef _stat
#define _stat _stat64
#endif

#ifdef _tstat
#undef _tstat
#define _tstat _tstat64
#endif

#ifndef snprintf
#define snprintf c99_snprintf

static inline int
c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);
    return count;
}

static inline int
c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);
    return count;
}
#endif

#ifndef snwprintf
#define snwprintf c99_snwprintf
static inline int
c99_vsnwprintf(wchar_t* str, size_t size, const wchar_t* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnwprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscwprintf(format, ap);
    return count;
}

static inline int
c99_snwprintf(wchar_t* str, size_t size, const wchar_t* format, ...)
{
    int count;
    va_list ap;
    va_start(ap, format);
    count = c99_vsnwprintf(str, size, format, ap);
    va_end(ap);
    return count;
}
#endif

#ifdef _UNICODE
#define sntprintf c99_snwprintf
#else
#define sntprintf c99_snprintf
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#if APP_DEBUG
__declspec(dllexport) void __cdecl eu_init_logs(void);
__declspec(dllexport) void __cdecl eu_logmsg(const char *format, ...);
//#define printf eu_logmsg
#else
#define eu_init_logs(...) (__noop)
#define eu_logmsg(...) (__noop)
#define printf(...) (__noop)
#endif // _DEBUG

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _EU_LOG_H_
