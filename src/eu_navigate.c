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

static bool
on_navigate_diff(eu_tabpage *p1, eu_tabpage *pnode)
{
    if (p1 && pnode && p1 != pnode)
    {
        char sub[FILESIZE] = {0};
        const char *p = NULL;
        const char *ext = (pnode && pnode->doc_ptr) ? pnode->doc_ptr->extname : NULL;
        util_make_u8(p1->extname, sub, FILESIZE);
        if (ext && sub[0] && (p = util_stristr(ext, sub)) != NULL && (p + strlen(sub))[0] == ';')
        {
            return true;
        }
    }
    return false;
}

static int
on_navigate_match_callback(pcre_conainer *pcre_info, void *param)
{
    const char *p = NULL;
    eu_tabpage *pnode = (eu_tabpage *)param;
    if (!pnode || !pcre_info->named_substring || pcre_info->rc < 0)
    {
        eu_logmsg("Navigate: pcre matching error or not match\n");
        return EUE_PCRE_NO_MATCHING;
    }
    for (int i = 1; i < pcre_info->rc; ++i)
    {
        char buf[MAX_PATH+1] = {0};
        const char *substring_start = pcre_info->buffer + pcre_info->ovector[2 * i];
        int substring_length = pcre_info->ovector[2 * i + 1] - pcre_info->ovector[2 * i];
        if (substring_length > 0)
        {
            snprintf(buf, MAX_PATH, "%.*s", substring_length, substring_start);
        }
        if (*buf)
        {
            // 某些语言函数带类名称
            if ((p = util_strrstr(buf, "::")))
            {
                p += strlen("::");
            }
            if (STRCMP(buf, ==, pcre_info->named_substring) || (STR_NOT_NUL(p) && STRCMP(p, ==, pcre_info->named_substring)))
            {
                pnode->nc_pos = (intptr_t)pcre_info->ovector[2 * i];
                return EUE_PCRE_BACK_ABORT;
            }
        }
    }
    return SKYLARK_OK;
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

bool
on_navigate_find_this(eu_tabpage *pnode, const char *name, sptr_t *pos)
{
    bool ret = false;
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->reqular_exp)
    {
        size_t bytes = 0;
        uint8_t *buffer = NULL;
        pcre_conainer *pcre_info = NULL;
        intptr_t psave = pnode->nc_pos;
        const char *regxp = pnode->doc_ptr->reqular_exp;
        if ((buffer = (uint8_t *)util_strdup_content(pnode, &bytes)))
        {
            if ((pcre_info = eu_pcre_init((const char *)buffer, bytes, regxp, name, PCRE_NO_UTF8_CHECK|PCRE_CASELESS)) != NULL)
            {
                pnode->nc_pos = -1;
                eu_pcre_exec_multi(pcre_info, on_navigate_match_callback, pnode);
                ret = (*pos = pnode->nc_pos) >= 0;
                pnode->nc_pos = psave;
                eu_pcre_delete(pcre_info);
            }
            free(buffer);
        }
    }
    return ret;
}

int
on_navigate_jump(eu_tabpage *pnode, sptr_t wp, sptr_t lp)
{
    sptr_t postion = -1;
    eu_tabpage *p = NULL;
    char *text = NULL;
    if ((p = pnode))
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t start_pos = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, pos, true);
        sptr_t end_pos = eu_sci_call(pnode, SCI_WORDENDPOSITION, pos, true);
        text = on_sci_range_text(pnode, start_pos, end_pos);
    }
    if (STR_NOT_NUL(text) && !on_navigate_find_this(pnode, text, &postion))
    {
        for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(index)) != NULL && on_navigate_diff(p, pnode) && on_navigate_find_this(p, text, &postion))
            {
                break;
            }
        }
    }
    if (postion >= 0 && p)
    {
        on_tabpage_selection(p);
        eu_sci_call(p, SCI_GOTOPOS, postion, 0);
        on_navigate_list_update(p, postion);
    }
    eu_safe_free(text);
    return SKYLARK_OK;
}
