/******************************************************************************
 * This file is part of c-vector project
 * From https://github.com/eteran/c-vector
 * The MIT License (MIT)
 * Copyright (c) 2015 Evan Teran
 *
 * Permission is hereby granted, free of charge, to_ any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to_ deal
 * in the Software without restriction, including without limitation the rights
 * to_ use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to_ permit persons to_ whom the Software is
 * furnished to_ do so, subject to_ the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#ifndef CVECTOR_H_
#define CVECTOR_H_

#include <assert.h> /* for assert */
#include <stdlib.h> /* for malloc/realloc/free */
#include <string.h> /* for memcpy/memmove */

/* cvector heap implemented using C library malloc() */

/* in case C library malloc() needs extra protection,
 * allow these defines to_ be overridden.
 */
#ifndef cvector_clib_free
#define cvector_clib_free free
#endif
#ifndef cvector_clib_malloc
#define cvector_clib_malloc malloc
#endif
#ifndef cvector_clib_calloc
#define cvector_clib_calloc calloc
#endif
#ifndef cvector_clib_realloc
#define cvector_clib_realloc realloc
#endif

/**
 * @brief cvector_vector_type - The vector type used in this library
 */
#define cvector_vector_type(type) type *

/**
 * @brief cvector_capacity - gets the current capacity of the vector
 * @param_ vec_ - the vector
 * @return the capacity as a size_t
 */
#define cvector_capacity(vec_) ((vec_) ? ((size_t *) (vec_))[-1] : (size_t) 0)

/**
 * @brief cvector_size - gets the current size_ of the vector
 * @param_ vec_ - the vector
 * @return the size_ as a size_t
 */
#define cvector_size(vec_) ((vec_) ? ((size_t *) (vec_))[-2] : (size_t) 0)

/**
 * @brief cvector_empty - returns non-zero if the vector is empty
 * @param_ vec_ - the vector
 * @return non-zero if empty, zero if non-empty
 */
#define cvector_empty(vec_) (cvector_size(vec_) == 0)

/**
 * @brief cvector_reserve - Requests that the vector capacity be at least enough
 * to_ contain n elements. If n is greater than the current vector capacity, the
 * function causes the container to_ reallocate its storage increasing its
 * capacity to_ n (or greater).
 * @param_ vec_ - the vector
 * @param_ n - Minimum capacity for the vector.
 * @return void
 */
#define cvector_reserve(vec_, capacity)           \
    do                                            \
    {                                             \
        size_t cv_cap__ = cvector_capacity(vec_); \
        if (cv_cap__ < (capacity))                \
        {                                         \
            cvector_grow((vec_), (capacity));     \
        }                                         \
    } while (0)

/**
 * @brief cvector_erase - removes the element at index i from_ the vector
 * @param_ vec_ - the vector
 * @param_ i - index of element to_ remove
 * @return void
 */
#define cvector_erase(vec_, i_)                                                                    \
    do                                                                                             \
    {                                                                                              \
        if ((vec_))                                                                                \
        {                                                                                          \
            const size_t cv_sz__ = cvector_size(vec_);                                             \
            if ((i_) < cv_sz__)                                                                    \
            {                                                                                      \
                cvector_set_size((vec_), cv_sz__ - 1);                                             \
                memmove((vec_) + (i_), (vec_) + (i_) + 1, sizeof(*(vec_)) * (cv_sz__ - 1 - (i_))); \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/**
 * @brief cvector_free - frees all memory associated with the vector
 * @param_ vec_ - the vector
 * @return void
 */
#define cvector_free(vec_)                          \
    do                                              \
    {                                               \
        if ((vec_))                                 \
        {                                           \
            size_t *p1_ = &((size_t *) (vec_))[-2]; \
            cvector_clib_free((p1_));               \
        }                                           \
    } while (0)

/**
 * @brief cvector_freep - frees all memory and assign null to_ the pointer
 * @param_ pvec_ - the vector pointer
 * @return void
 */
#define cvector_freep(pvec_)                                   \
    do                                                         \
    {                                                          \
        if ((pvec_ && *pvec_))                                 \
        {                                                      \
            void *val__;                                       \
            memcpy(&val__, (pvec_), sizeof(val__));            \
            memcpy((pvec_), &(void *){ NULL }, sizeof(val__)); \
            cvector_free(val__);                               \
        }                                                      \
    } while (0)

/**
 * @brief cvector_begin - returns an iterator to_ first element of the vector
 * @param_ vec_ - the vector
 * @return a pointer to_ the first element (or NULL)
 */
#define cvector_begin(vec_) (vec_)

/**
 * @brief cvector_end - returns an iterator to_ one past the last element of the vector
 * @param_ vec_ - the vector
 * @return a pointer to_ one past the last element (or NULL)
 */
#define cvector_end(vec_) ((vec_) ? &((vec_)[cvector_size(vec_)]) : NULL)

/* user request to_ use logarithmic growth algorithm */
#ifdef CVECTOR_LOGARITHMIC_GROWTH

/**
 * @brief cvector_compute_next_grow - returns an the computed size_ in next vector grow
 * size_ is increased by multiplication of 2
 * @param_ size_ - current size_
 * @return size_ after next vector grow
 */
#define cvector_compute_next_grow(size_) ((size_) ? ((size_) << 1) : 1)

#else

/**
 * @brief cvector_compute_next_grow - returns an the computed size_ in next vector grow
 * size_ is increased by 1
 * @param_ size_ - current size_
 * @return size_ after next vector grow
 */
#define cvector_compute_next_grow(size_) ((size_) + 1)

#endif /* CVECTOR_LOGARITHMIC_GROWTH */

/**
 * @brief cvector_push_back - adds an element to_ the end of the vector
 * @param_ vec_ - the vector
 * @param_ value_ - the value_ to_ add
 * @return void
 */
#define cvector_push_back(vec_, value_)                                \
    do                                                                 \
    {                                                                  \
        size_t cv_cap__ = cvector_capacity(vec_);                      \
        if (cv_cap__ <= cvector_size(vec_))                            \
        {                                                              \
            cvector_grow((vec_), cvector_compute_next_grow(cv_cap__)); \
        }                                                              \
        (vec_)[cvector_size(vec_)] = (value_);                         \
        cvector_set_size((vec_), cvector_size(vec_) + 1);              \
    } while (0)

/**
 * @brief cvector_insert - insert element at position pos_ to_ the vector
 * @param_ vec_ - the vector
 * @param_ pos_ - position in the vector where the new elements are inserted.
 * @param_ val_ - value_ to_ be copied (or moved) to_ the inserted elements.
 * @return void
 */
#define cvector_insert(vec_, pos_, val_)                                                                      \
    do                                                                                                        \
    {                                                                                                         \
        size_t cv_cap__ = cvector_capacity(vec_);                                                             \
        if (cv_cap__ <= cvector_size(vec_))                                                                   \
        {                                                                                                     \
            cvector_grow((vec_), cvector_compute_next_grow(cv_cap__));                                        \
        }                                                                                                     \
        if ((pos_) < cvector_size(vec_))                                                                      \
        {                                                                                                     \
            memmove((vec_) + (pos_) + 1, (vec_) + (pos_), sizeof(*(vec_)) * ((cvector_size(vec_)) - (pos_))); \
        }                                                                                                     \
        (vec_)[(pos_)] = (val_);                                                                              \
        cvector_set_size((vec_), cvector_size(vec_) + 1);                                                     \
    } while (0)

/**
 * @brief cvector_put - put element at position pos_ to_ the vector
 * @param_ vec_ - the vector
 * @param_ pos_ - position in the vector where the new elements are put
 * @param_ val_ - value_ to_ be put to_ the inserted elements.
 * @return void
 */
#define cvector_put(vec_, pos_, val_)         \
    do                                        \
    {                                         \
        if ((pos_) >= cvector_size(vec_) + 1) \
        {                                     \
            cvector_grow((vec_), (pos_) + 1); \
            cvector_set_size((vec_), (pos_)); \
        }                                     \
        (vec_)[(pos_)] = (val_);              \
    } while (0)

/**
 * @brief cvector_pop_back - removes the last element from_ the vector
 * @param_ vec_ - the vector
 * @return void
 */
#define cvector_pop_back(vec_)                            \
    do                                                    \
    {                                                     \
        cvector_set_size((vec_), cvector_size(vec_) - 1); \
    } while (0)

/**
 * @brief cvector_clear - empty the array, but not free memory
 * @param_ vec_ - the vector
 * @return void
 */
#define cvector_clear(vec_)          \
    do                               \
    {                                \
        cvector_set_size((vec_), 0); \
    } while (0)

/**
 * @brief cvector_copy - copy a vector
 * @param_ from_ - the original vector
 * @param_ to_ - destination to_ which the function copy to_
 * @return void
 */
#define cvector_copy(from_, to_)                                            \
    do                                                                      \
    {                                                                       \
        if ((from_))                                                        \
        {                                                                   \
            cvector_grow(to_, cvector_size(from_));                         \
            cvector_set_size(to_, cvector_size(from_));                     \
            memcpy((to_), (from_), cvector_size(from_) * sizeof(*(from_))); \
        }                                                                   \
    } while (0)

/**
 * @brief cvector_set_capacity - For internal use, sets the capacity variable of the vector
 * @param_ vec_ - the vector
 * @param_ size_ - the new capacity to_ set
 * @return void
 */
#define cvector_set_capacity(vec_, size_)      \
    do                                         \
    {                                          \
        if ((vec_))                            \
        {                                      \
            ((size_t *) (vec_))[-1] = (size_); \
        }                                      \
    } while (0)

/**
 * @brief cvector_set_size - For internal use, sets the size_ variable of the vector
 * @param_ vec_ - the vector
 * @param_ size_ - the new capacity to_ set
 * @return void
 */
#define cvector_set_size(vec_, size_)          \
    do                                         \
    {                                          \
        if ((vec_))                            \
        {                                      \
            ((size_t *) (vec_))[-2] = (size_); \
        }                                      \
    } while (0)

/**
 * @brief cvector_grow - For internal use, ensures that the vector is at least <count_> elements big
 * @param_ vec_ - the vector
 * @param_ count_ - the new capacity to_ set
 * @return void
 */
#define cvector_grow(vec_, count_)                                                \
    do                                                                            \
    {                                                                             \
        const size_t cv_sz__ = (count_) * sizeof(*(vec_)) + (sizeof(size_t) * 2); \
        if ((vec_))                                                               \
        {                                                                         \
            size_t *cv_p1__ = &((size_t *) (vec_))[-2];                           \
            size_t *cv_p2__ = cvector_clib_realloc(cv_p1__, (cv_sz__));           \
            assert(cv_p2__);                                                      \
            (vec_) = (void *) (&cv_p2__[2]);                                      \
            cvector_set_capacity((vec_), (count_));                               \
        }                                                                         \
        else                                                                      \
        {                                                                         \
            size_t *cv_p__ = cvector_clib_malloc(cv_sz__);                        \
            assert(cv_p__);                                                       \
            (vec_) = (void *) (&cv_p__[2]);                                       \
            cvector_set_capacity((vec_), (count_));                               \
            cvector_set_size((vec_), 0);                                          \
        }                                                                         \
    } while (0)

/*
 * @brief cvector_init - Initialize a vector.  The vector must be NULL for this to_ do anything.
 * @param_ vec_ - the vector
 * @param_ capacity - vector capacity to_ reserve
 * @return void
 */
#define cvector_init(vec_, count_)                                                \
    do                                                                            \
    {                                                                             \
        const size_t cv_sz__ = (count_) * sizeof(*(vec_)) + (sizeof(size_t) * 2); \
        if (!(vec_))                                                              \
        {                                                                         \
            size_t *cv_p__ = cvector_clib_calloc(cv_sz__);                        \
            assert(cv_p__);                                                       \
            (vec_) = (void *) (&cv_p__[2]);                                       \
            cvector_set_capacity((vec_), (count_) + 1);                           \
            cvector_set_size((vec_), ((count_)));                                 \
        }                                                                         \
    } while (0)

/**
 * @brief cvector_for_each - call function func_ on each element of the vector
 * @param_ vec_ - the vector
 * @param_ func_ - function to_ be called on each element that takes each element as argument
 * @return void
 */
#define cvector_for_each(vec_, func_)                          \
    do                                                         \
    {                                                          \
        if ((vec_) && (func_) != NULL)                         \
        {                                                      \
            for (size_t i_ = 0; i_ < cvector_size(vec_); ++i_) \
            {                                                  \
                func_((vec_)[i_]);                             \
            }                                                  \
        }                                                      \
    } while (0)

/**
 * @brief cvector_for_each - call function with param_ on each element of the vector
 * @param_ vec_ - the vector
 * @param_ func_ - function to_ be called on each element that takes each element as argument
 * @return void
 */
#define cvector_for_each_and_do(vec_, func_, param_)        \
    do                                                      \
    {                                                       \
        if ((vec_) && (func_) != NULL)                      \
        {                                                   \
            for (size_t i = 0; i < cvector_size(vec_); ++i) \
            {                                               \
                func_(&(vec_)[i], (param_));                \
            }                                               \
        }                                                   \
    } while (0)

/**
 * @brief cvector_for_each_and_cmp - call function func_ on each element of the vector
 * @param_ vec_ - the vector
 * @param_ func_ - function to_ be called on each element that takes each element as argument
 * @param_ param_ - function param_
 * @param_ it_ - the vector pointer
 * @return void
 */
#define cvector_for_each_and_cmp(vec_, func_, param_, it_) \
    do                                                     \
    {                                                      \
        if ((vec_) && (func_) && (it_))                    \
        {                                                  \
            size_t i_ = 0;                                 \
            size_t len_ = cvector_size(vec_);              \
            for (; i_ < len_; ++i_)                        \
            {                                              \
                if (!func_(&(vec_)[i_], param_))           \
                {                                          \
                    break;                                 \
                }                                          \
            }                                              \
            if (i_ >= 0 && i_ < len_)                      \
            {                                              \
                *(it_) = &(vec_)[i_];                      \
            }                                              \
        }                                                  \
    } while (0)

/**
 * @brief cvector_free_each_and_free - calls `free_func` on each element
 * contained in the vector and then destroys the vector itself
 * @param_ vec_ - the vector
 * @param_ free_func - function used to_ free each element in the vector with
 * one parameter which is the element to_ be freed)
 * @return void
 */
#define cvector_free_each_and_free(vec_, free_func) \
    do                                              \
    {                                               \
        cvector_for_each((vec_), (free_func));      \
        cvector_free(vec_);                         \
    } while (0)

#endif /* CVECTOR_H_ */
