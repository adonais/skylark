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

#if APP_DEBUG

#include "framework.h"

#define LOG_FILE _T("skylark.log")

static TCHAR file_buf[MAX_PATH + 1];

void __stdcall
eu_init_logs(void)
{
    if (*file_buf == 0 && GetEnvironmentVariable(_T("APPDATA"), file_buf, MAX_PATH) > 0)
    {
        _tcsncat(file_buf, _T("\\"), MAX_PATH);
        _tcsncat(file_buf, LOG_FILE, MAX_PATH);
        if (eu_exist_file(file_buf))
        {
            DeleteFile(file_buf);
        }
    }
}

void __cdecl 
eu_logmsg(const char *format, ...)
{
    va_list args;
    char buffer[MAX_BUFFER] = {0};
    va_start(args, format);
    if (_tcslen(file_buf) > 0)
    {
        FILE *pfile = NULL;
        int len = vsnprintf(buffer, MAX_BUFFER - 1, format, args);
        if (len > 0 && len < MAX_BUFFER)
        {
            buffer[len] = '\0';
            if ((pfile = _tfopen(file_buf, _T("a+"))) != NULL)
            {
                fwrite(buffer, strlen(buffer), 1, pfile);
                fclose(pfile);
            }
        }
    }
    va_end(args);
    return;
}
#endif
