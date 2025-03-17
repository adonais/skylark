#include "framework.h"

typedef struct _eu_threadpool
{
    TASK_T            task;
    HANDLE            event;
} eu_threadpool;

static HANDLE g_threadpool_sem = NULL;
static cvector(eu_threadpool) g_threadpool_task = NULL;
static cvector(TASK_ARG) g_timer_task = NULL;

#ifdef EU_POOL_TESTING
static CRITICAL_SECTION       g_threadpool_cs;
static CRITICAL_SECTION_DEBUG critsect_debug_cs = {
    0,
    0,
    &g_threadpool_cs,
    {&critsect_debug_cs.ProcessLocksList, &critsect_debug_cs.ProcessLocksList},
    0,
    0,
    (DWORD_PTR)0x1,
};
static CRITICAL_SECTION g_threadpool_cs = {&critsect_debug_cs, -1, 0, 0, 0, 0};
static int              gcs_test = 0;

int get_context(TASK_T hv)
{
    return hv ? hv->attach : 0;
}

/* 回调函数，处理事件触发后的任务 */
void CALLBACK WaitCallback(PTP_CALLBACK_INSTANCE inst, PVOID *lp, PTP_WAIT wait, TP_WAIT_RESULT result)
{
    TASK_T hv = (TASK_T)lp;
    while (gcs_test < 0xffff)
    {
        printf("Thread_id = %u, task %d value. wait result: %s\n",
               GetCurrentThreadId(), get_context(hv),
               (result == WAIT_OBJECT_0) ? "Signaled" : "Timeout");
        if (WaitForSingleObject(g_threadpool_sem, 0) != WAIT_TIMEOUT)
        {
            printf("recv g_threadpool_sem, thread %u exit ...\n", GetCurrentThreadId());
            break;
        }
        if (hv && _InterlockedCompareExchange(&hv->cancel, 0, 1))
        {
            printf("recv cancel, thread %u exit ...\n", GetCurrentThreadId());
            break;
        }
        EnterCriticalSection(&(g_threadpool_cs));
        ++gcs_test;
        LeaveCriticalSection(&(g_threadpool_cs));
        Sleep(300);
    }
}
#endif

HANDLE
eu_threadpool_handle(void)
{
    return g_threadpool_sem;
}

bool
eu_threadpool_init(void)
{
    return ((g_threadpool_sem = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL);
}

bool
eu_threadpool_add(PTP_WAIT_CALLBACK wait_back, TASK_T parg)
{
    bool          ret = false;
    eu_threadpool pool = {0};
    if (wait_back && parg && g_threadpool_sem)
    {
        do
        {
            if ((pool.task = (TASK_T)malloc(sizeof(TASK_ARG))) == NULL)
            {
                eu_logmsg("malloc failed for task\n");
                break;
            }
            memcpy(pool.task, parg, sizeof(TASK_ARG));
            if ((pool.event = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
            {
                eu_logmsg("CreateEvent failed for task\n");
                break;
            }
            if ((pool.task->pwait = (intptr_t)CreateThreadpoolWait(wait_back, (PVOID)pool.task, NULL)) == 0)
            {
                eu_logmsg("CreateThreadpoolWait failed for task\n");
                break;
            }
            if (true)
            {
                SetThreadpoolWait((PTP_WAIT)pool.task->pwait, pool.event, NULL);
                pool.task->pcall = (intptr_t)wait_back;
                cvector_push_back(g_threadpool_task, pool);
            }
            if (!SetEvent(pool.event))
            {
                cvector_pop_back(g_threadpool_task);
                eu_logmsg("SetEvent failed for task\n");
                break;
            }
            ret = true;
        } while (0);
    }
    if (!ret)
    {
        if (pool.task)
        {
            if (pool.task->block)
            {
                free(pool.task->block);
            }
            if (pool.task->pwait)
            {
                CloseThreadpoolWait((PTP_WAIT)pool.task->pwait);
            }
            free(pool.task);
            pool.task = NULL;
        }
        if (pool.event)
        {
            CloseHandle(pool.event);
        }
    }
    return ret;
}

bool
eu_threadpool_check(PTP_WAIT_CALLBACK wait_back, const intptr_t indent)
{
    if (g_threadpool_sem)
    {
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].task)
            {
                if (indent > 0)
                {
                    if (indent == g_threadpool_task[i].task->pdata && g_threadpool_task[i].task->xcode == 1L)
                    {
                        return true;
                    }
                }
                else if (wait_back)
                {
                    if (g_threadpool_task[i].task->pcall == (intptr_t)wait_back && g_threadpool_task[i].task->xcode == 1L)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool
eu_threadpool_cancel(PTP_WAIT_CALLBACK wait_back, const intptr_t indent)
{
    bool res = false;
    if (g_threadpool_sem)
    {
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].task)
            {
                if (indent > 0)
                {
                    if (indent == g_threadpool_task[i].task->pdata)
                    {
                        _InterlockedExchange(&g_threadpool_task[i].task->cancel, 1);
                        return true;
                    }
                }
                else if (wait_back && g_threadpool_task[i].task->pcall == (intptr_t)wait_back)
                {
                    _InterlockedExchange(&g_threadpool_task[i].task->cancel, 1);
                    return true;
                }
            }
        }
    }
    return false;
}

void
eu_threadpool_join(void)
{
    if (g_threadpool_sem)
    {
        SetEvent(g_threadpool_sem);
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].task && g_threadpool_task[i].task->pwait)
            {
                WaitForThreadpoolWaitCallbacks((PTP_WAIT)g_threadpool_task[i].task->pwait, TRUE);
            }
        }
    }
}

void
eu_threadpool_destroy(void)
{
    if (g_threadpool_sem)
    {
        ResetEvent(g_threadpool_sem);
        CloseHandle(g_threadpool_sem);
        g_threadpool_sem = NULL;
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].task)
            {
                if (g_threadpool_task[i].task->block)
                {
                    free(g_threadpool_task[i].task->block);
                }
                if (g_threadpool_task[i].task->pwait)
                {
                    CloseThreadpoolWait((PTP_WAIT)g_threadpool_task[i].task->pwait);
                }
                free(g_threadpool_task[i].task);
                g_threadpool_task[i].task = NULL;
            }
            if (g_threadpool_task[i].event)
            {
                CloseHandle(g_threadpool_task[i].event);
                g_threadpool_task[i].event = NULL;
            }
        }
        cvector_freep(&g_threadpool_task);
    }
}

void
eu_timer_join(void)
{
    if (g_timer_task)
    {
        for (size_t i = 0; i < cvector_size(g_timer_task); ++i)
        {
            _InterlockedExchange(&g_timer_task[i].cancel, 1);
            if (g_timer_task[i].pwait > 0)
            {
                WaitForThreadpoolTimerCallbacks((PTP_TIMER)g_timer_task[i].pwait, TRUE);
            }
        }
    }
}

bool
eu_timer_create(PTP_TIMER_CALLBACK call_back, TASK_T pv)
{
    PTP_TIMER timer = NULL;
    if (call_back && pv)
    {
        TASK_ARG hv;
        memcpy(&hv, pv, sizeof(TASK_ARG));
        FILETIME ft;
        ULARGE_INTEGER ul;
        size_t i = 0;
        // 20毫米后启动定时器
        ul.QuadPart = (LONGLONG) - (200000);
        ft.dwHighDateTime = ul.HighPart;
        ft.dwLowDateTime = ul.LowPart;
        hv.pcall = (intptr_t)call_back;
        cvector_push_back(g_timer_task, hv);
        if ((i = cvector_size(g_timer_task)) > 0 && (timer = CreateThreadpoolTimer(call_back, (PVOID)&g_timer_task[i - 1], NULL)) != NULL)
        {   // 保存PTP_TIMER句柄到结构数组
            g_timer_task[i - 1].pwait = (intptr_t)timer;
            // 间隔时间内回调函数仅运行一次
            SetThreadpoolTimer(timer, &ft, 0, 0);
        }
    }
    return (timer != NULL);
}

void
eu_timer_destroy(void)
{
    if (g_timer_task)
    {
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_timer_task[i].block)
            {
                free(g_timer_task[i].block);
            }
            if (g_timer_task[i].pwait)
            {
                CloseThreadpoolTimer((PTP_TIMER)g_timer_task[i].pwait);
            }
        }
        cvector_freep(&g_timer_task);
    }
}

bool
eu_timer_check(PTP_TIMER_CALLBACK wait_back, const intptr_t indent)
{
    
    for (size_t i = 0; i < cvector_size(g_timer_task); ++i)
    {
        if (indent > 0)
        {
            if (indent == g_timer_task[i].pdata && g_timer_task[i].xcode == 1L)
            {
                return true;
            }
        }
        else if (wait_back && g_timer_task[i].pcall == (intptr_t)wait_back && g_timer_task[i].xcode == 1L)
        {
            return true;
        }
    }
    return false;
}

uint32_t
eu_timer_id(PTP_TIMER_CALLBACK call_back, const intptr_t indent)
{
    for (size_t i = 0; i < cvector_size(g_timer_task); ++i)
    {
        if (indent > 0)
        {
            if (indent == g_timer_task[i].pdata && g_timer_task[i].pthid > 0u)
            {
                return g_timer_task[i].pthid;
            }
        }
        else if (call_back && g_timer_task[i].pcall == (intptr_t)call_back && g_timer_task[i].pthid > 0u)
        {
            return g_timer_task[i].pthid;
        }
    }
    return 0;
}

bool
eu_timer_cancel(PTP_TIMER_CALLBACK call_back, const intptr_t indent)
{
    for (size_t i = 0; i < cvector_size(g_timer_task); ++i)
    {
        if ((indent > 0 && indent == g_timer_task[i].pdata) || (call_back && g_timer_task[i].pcall == (intptr_t)call_back))
        {
            _InterlockedExchange(&g_timer_task[i].cancel, 1);
            WaitForThreadpoolTimerCallbacks((PTP_TIMER)g_timer_task[i].pwait, TRUE);
            if (g_timer_task[i].block)
            {
                free(g_timer_task[i].block);
            }
            if (g_timer_task[i].pwait)
            {
                CloseThreadpoolTimer((PTP_TIMER)g_timer_task[i].pwait);
            }
            cvector_erase(g_timer_task, i);
            return true;
        }
    }
    return false;
}
