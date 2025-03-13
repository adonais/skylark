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

#ifndef _EU_POOL_H_
#define _EU_POOL_H_

typedef struct _TASK_ARG
{
    int           attach;
    char         *block;    // 动态数据, 用户分配内存, 由线程池销毁
    intptr_t      pdata;    // 用户数据, 可作为指针或整数
    volatile long xcode;    // 线程运行状态. 1, 线程运行中
    volatile long cancel;   // 回调函数检测这个变量, 可安全退出线程
} TASK_ARG, *TASK_T;

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

bool   eu_threadpool_init(void);
bool   eu_threadpool_add(PTP_WAIT_CALLBACK wait_back, TASK_T parg);
bool   eu_threadpool_cancel(PTP_WAIT_CALLBACK wait_back);
bool   eu_threadpool_check(PTP_WAIT_CALLBACK wait_back);
void   eu_threadpool_join(void);
void   eu_threadpool_destroy(void);
HANDLE eu_threadpool_handle(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_POOL_H_
