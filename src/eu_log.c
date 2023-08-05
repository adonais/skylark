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

static TCHAR logfile_path[MAX_BUFFER];

bool __cdecl
eu_init_logs(const bool turn)
{
    if (eu_get_config())
    {
        if (turn)
        {
            eu_get_config()->m_logging ^= true;
        }
        if (*logfile_path == 0)
        {
            _sntprintf(logfile_path, MAX_BUFFER - 1, _T("%s\\skylark.log"), eu_config_path);
        }
        if (eu_exist_file(logfile_path))
        {
            util_delete_file(logfile_path);
        }
        return (*logfile_path != 0);
    }
    return false;
}

#if !APP_DEBUG
void __cdecl
eu_logmsg(const char *format, ...)
{
    va_list args = NULL;
    char buffer[MAX_BUFFER + 1] = {0};
    if (eu_get_config() && eu_get_config()->m_logging && logfile_path[0])
    {
        FILE *pfile = NULL;
        va_start(args, format);
        int len = vsnprintf(buffer, MAX_BUFFER, format, args);
        if (len > 0 && len < MAX_BUFFER)
        {
            buffer[len] = '\0';
            if ((pfile = _tfopen(logfile_path, _T("a+b"))) != NULL)
            {
                fwrite(buffer, strlen(buffer), 1, pfile);
                fclose(pfile);
            }
        }
    }
    if (args)
    {
        va_end(args);
    }
}
#endif
