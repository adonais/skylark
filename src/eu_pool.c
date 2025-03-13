#include "framework.h"

typedef struct _eu_threadpool
{
    TASK_T            task;
    PTP_WAIT          wait;
    HANDLE            event;
    PTP_WAIT_CALLBACK pcall;
} eu_threadpool;

static HANDLE g_threadpool_sem = NULL;
static cvector(eu_threadpool) g_threadpool_task = NULL;

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
void CALLBACK WaitCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT WaitResult)
{
    TASK_T hv = (TASK_T)Context;
    while (1)
    {
        printf("Thread_id = %u, Task1 %d value. Wait result: %s\n",
               GetCurrentThreadId(), get_context(hv),
               (WaitResult == WAIT_OBJECT_0) ? "Signaled" : "Timeout");
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
    if (wait_back && g_threadpool_sem)
    {
        do
        {
            if (parg)
            {
                if ((pool.task = (TASK_T)malloc(sizeof(TASK_ARG))) == NULL)
                {
                    eu_logmsg("malloc failed for task\n");
                    break;
                }
                memcpy(pool.task, parg, sizeof(TASK_ARG));
            }
            if ((pool.event = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
            {
                eu_logmsg("CreateEvent failed for task\n");
                break;
            }
            if ((pool.wait = CreateThreadpoolWait(wait_back, (PVOID)pool.task,
                                                  NULL)) == NULL)
            {
                eu_logmsg("CreateThreadpoolWait failed for task\n");
                break;
            }
            if (true)
            {
                SetThreadpoolWait(pool.wait, pool.event, NULL);
                pool.pcall = wait_back;
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
            free(pool.task);
        }
        if (pool.wait)
        {
            CloseThreadpoolWait(pool.wait);
        }
        if (pool.event)
        {
            CloseHandle(pool.event);
        }
    }
    return ret;
}

bool
eu_threadpool_check(PTP_WAIT_CALLBACK wait_back)
{
    bool res = false;
    if (g_threadpool_sem)
    {
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].pcall == wait_back && g_threadpool_task[i].task->xcode == 1L)
            {
                res = true;
                break;
            }
        }
    }
    return res;
}

bool
eu_threadpool_cancel(PTP_WAIT_CALLBACK wait_back)
{
    bool res = false;
    if (g_threadpool_sem)
    {
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            if (g_threadpool_task[i].pcall == wait_back)
            {
                res = _InterlockedExchange(&g_threadpool_task[i].task->cancel, 1) == 0;
                break;
            }
        }
    }
    eu_logmsg("eu_threadpool_cancel return %s\n", (res ? "true" : "false"));
    return res;
}

void
eu_threadpool_join(void)
{
    if (g_threadpool_sem)
    {
        SetEvent(g_threadpool_sem);
        for (size_t i = 0; i < cvector_size(g_threadpool_task); ++i)
        {
            WaitForThreadpoolWaitCallbacks(g_threadpool_task[i].wait, FALSE);
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
                free(g_threadpool_task[i].task);
            }
            if (g_threadpool_task[i].wait)
            {
                CloseThreadpoolWait(g_threadpool_task[i].wait);
            }
            if (g_threadpool_task[i].event)
            {
                CloseHandle(g_threadpool_task[i].event);
            }
        }
        cvector_freep(&g_threadpool_task);
    }
}
