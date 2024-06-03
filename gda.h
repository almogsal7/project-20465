#ifndef maman14_gda_h
#define maman14_gda_h

#include <stddef.h>
/* opaque struct */
struct gda;
typedef struct gda * gda;

/**
 * @brief 
 * 
 * @param ctor 
 * @param dtor 
 * @param compar 
 * @return gda 
 */
gda gda_create(void *(*ctor)(const void *candidate),
            void (*dtor)(void * candidate),
            int (*compar)(const void *candidate1,const void * candidate2));


/**
 * @brief 
 * 
 * @param gda 
 * @param candidate 
 * @return void* if not exists returns NULL. 
 */
void *gda_search(gda gda,const void *candidate);

/**
 * @brief inserts element into gda, changes sort mode to zero (obviously).
 * 
 * @param gda 
 * @param candidate 
 * @return returns a pointer to the inserted element, returns NULL otherwise.
 */
void * gda_insert(gda gda, const void *candidate);

/**
 * @brief deletes element from gda, changes sort mode to zero (obviouisly).
 * 
 * @param gda 
 * @param candidate 
 */
void gda_delete(gda gda, const void *candidate);

/**
 * @brief destorys the gda.
 * 
 * @param gda 
 */
void gda_destroy(gda gda);

/**
 * @brief 
 * 
 * @param gda 
 * @return void* const* 
 */
void *const * gda_get_begin_ptr(gda gda);

/**
 * @brief 
 * 
 * @param gda 
 * @return void* const* 
 */
void *const * gda_get_end_ptr(gda gda);

/**
 * @brief 
 * 
 * @param gda 
 * @return size_t returns number of elements inside the gda.
 */
size_t gda_size(gda gda);

#define gda_for_each(gda,begin_ptr,end_ptr) for(begin_ptr = gda_get_begin_ptr(gda),end_ptr = gda_get_end_ptr(gda);begin_ptr<=end_ptr;begin_ptr++)
#endif
