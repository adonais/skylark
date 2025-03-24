#include "framework.h"

static LIST_HEAD(list_trace);
static int max_nav_count = 0;

static bool
on_navigate_node_exist(eu_tabpage *pnode)
{
    if (!list_empty(&list_trace))
    {
        struct navigate_trace *curr = NULL;
        list_for_each_entry(curr, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr && curr->pnode == pnode)
            {
                return true;
            }
        }
    }
    return false;
}

static bool
on_navigate_pos_exist(int64_t *vec, int64_t postion)
{
    for (size_t i = 0; i < cvector_size(vec); ++i)
    {
        if (vec[i] == postion)
        {
            return true;
        }
    }
    return false;
}

int
on_navigate_list_add(eu_tabpage *pnode)
{
    struct navigate_trace *curr = NULL;
    if (!pnode || pnode->pmod || max_nav_count + 1 > MAX_TRACE_COUNT)
    {
        return EUE_TAB_NULL;
    }
    if (on_navigate_node_exist(pnode))
    {
        eu_logmsg("Navigate: node exist ...\n");
        return SKYLARK_OK;
    }
    if ((curr = (struct navigate_trace *)calloc(1, sizeof(struct navigate_trace))) != NULL)
    {
        curr->pnode = pnode;
        cvector_push_back(curr->pos, (int64_t)pnode->nc_pos);
        list_add_tail(&(curr->ng_node), &list_trace);
        ++max_nav_count;
        return SKYLARK_OK;
    }
    eu_logmsg("Navigate: %s, memory allocation failed\n", __FUNCTION__);
    return EUE_OUT_OF_MEMORY;
}

void
on_navigate_list_update(eu_tabpage *pnode, int64_t pos)
{
    if (pnode && pos >= 0)
    {
        struct navigate_trace *curr = NULL;
        list_for_each_entry(curr, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr && curr->pnode == pnode)
            {
                if (cvector_size(curr->pos) < MAX_TRACE_COUNT && !on_navigate_pos_exist(curr->pos, pos))
                {
                    cvector_push_back(curr->pos, pos);
                }
            }
        }
    }
}

bool
on_navigate_back_this(const eu_tabpage *pnode)
{
    struct navigate_trace *curr = NULL;
    if (pnode && !list_empty(&list_trace))
    {
        list_for_each_entry(curr, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr && curr->pnode == pnode && cvector_size(curr->pos) > 0)
            {
                int64_t *back = cvector_back(curr->pos);
                int64_t textlen = (int64_t)eu_sci_call((eu_tabpage *)pnode, SCI_GETLENGTH, 0, 0);
                int64_t postion = (int64_t)eu_sci_call((eu_tabpage *)pnode, SCI_GETCURRENTPOS, 0, 0);
                if (postion == *back)
                {
                    cvector_pop_back(curr->pos);
                    back = cvector_back(curr->pos);
                }
                if (back)
                {
                    sptr_t go = *back > textlen - 1 ? textlen - 1 : (sptr_t)(*back >= 0 ? *back : 0);
                    cvector_pop_back(curr->pos);
                    eu_sci_call((eu_tabpage *)pnode, SCI_GOTOPOS, go, 0);
                    return true;
                }
            }
        }
    }
    return false;
}

void
on_navigate_back_all(void)
{
    eu_tabpage *p = on_tabpage_focus_at();
    if (!on_navigate_back_this(p))
    {
        struct navigate_trace *curr = NULL;
        struct navigate_trace *prev = NULL;
        list_for_each_entry_safe_reverse(curr, prev, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr->pnode == p)
            {
                continue;
            }
            if (cvector_size(curr->pos) > 0)
            {
                on_tabpage_selection(curr->pnode);
                int64_t *back = cvector_back(curr->pos);
                int64_t textlen = (int64_t)eu_sci_call(curr->pnode, SCI_GETLENGTH, 0, 0);
                sptr_t go = *back > textlen - 1 ? textlen - 1 : (sptr_t)(*back >= 0 ? *back : 0);
                cvector_pop_back(curr->pos);
                eu_sci_call(curr->pnode, SCI_GOTOPOS, go, 0);
            }
        }
    }
}

void
on_navigate_clean_this(eu_tabpage *pnode)
{
    if (pnode)
    {
        struct navigate_trace *curr = NULL;
        struct navigate_trace *next = NULL;
        list_for_each_entry_safe(curr, next, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr->pnode == pnode)
            {
                list_del(&(curr->ng_node));
                cvector_freep(&curr->pos);
                free(curr);
                --max_nav_count;
            }
        }
    }
}
