#include <stdlib.h>
#include "../inc/gda.h"
#include <string.h>
/* The gda structure representing a generic dynamic array. */
struct gda {
    void **pointer_array; /* A pointer to an array of void pointers. */
    size_t  pointers_count; /* The total number of pointers in the array. */
    size_t  elem_count; /* The count of non-null elements in the array. */
    void *(*ctor)(const void *candidate); /* A constructor function pointer for deep copying elements. */
    void (*dtor)(void * candidate); /* A destructor function pointer for cleaning up elements. */
    int (*compar)(const void *candidate1,const void * candidate2); /* A comparison function pointer for searching elements. */
    
};

/**
 * @brief Creates a new gda.
 * 
 * @param ctor Function pointer for creating a new element
 * @param dtor Function pointer for destroying an element
 * @param compar Function pointer for comparing two elements
 * @return gda The created gda
 */
gda gda_create(void *(*ctor)(const void *candidate),
            void (*dtor)(void * candidate),
            int (*compar)(const void *candidate1,const void * candidate2)) {

            gda new_gda = calloc(1,sizeof(struct gda));
            if(!new_gda)
                return NULL;
            new_gda->compar         = compar;
            new_gda->ctor           = ctor;
            new_gda->dtor           = dtor;
            new_gda->pointers_count = 2;
            new_gda->pointer_array  = calloc(new_gda->pointers_count,sizeof(void *));
            if(!new_gda->pointer_array) {
                free(new_gda);
                return NULL;
            }
            return new_gda;
}
/**
 * @brief Searches for an element in the gda.
 * 
 * @param gda 
 * @param candidate Element to search for
 * @return void* Found element or NULL if not found
 */
void * gda_search(gda gda,const void *candidate) {
    void **runner;
    for(runner = gda->pointer_array;runner < gda->pointer_array + gda->pointers_count;runner++) {
        if(*runner) {
            if (gda->compar(*runner,candidate) == 0)
                return *runner;
        }
    }
    return NULL;
}
/**
 * @brief Inserts an element into the gda.
 * 
 * @param gda 
 * @param candidate Element to insert
 * @return void* Inserted element or NULL if insertion failed
 */
void * gda_insert(gda gda, const void *candidate) {
    size_t i;
    void * ret;
    void *realloc_ret;
    for(i=0;i<gda->pointers_count;i++) {
        if(gda->pointer_array[i] == NULL) {
            ret = gda->ctor(candidate);
            if(!ret)
                return NULL;
            gda->pointer_array[i] = ret;
            gda->elem_count++;
            return ret;
        }
    }
    gda->pointers_count *=2;
    realloc_ret = realloc(gda->pointer_array,gda->pointers_count * sizeof(void *));
    if(!realloc_ret) {
        gda->pointers_count /=2;
        return NULL;
    }
    ret = gda->ctor(candidate);
    if(!ret)
        return NULL;
    gda->pointer_array = realloc_ret;
    gda->pointer_array[gda->elem_count] = ret;
    memset(&gda->pointer_array[gda->elem_count+1],0,(gda->pointers_count - (gda->elem_count+1) ) * sizeof(void *));
    gda->elem_count++;
    return ret;
}
/**
 * @brief Deletes an element from the gda.
 * 
 * @param gda 
 * @param candidate Element to delete
 */
void gda_delete(gda gda, const void *candidate) {
    void **runner;
    for(runner = gda->pointer_array;runner < gda->pointer_array + gda->pointers_count;runner++) {
        if(*runner) {
            if (gda->compar(*runner,candidate) == 0){
                if(gda->dtor) 
                    gda->dtor(*runner);
                *runner = NULL;
                gda->elem_count--;
            }
        }
    }
}
/**
 * @brief Destroys the gda.
 * 
 * @param gda 
 */
void gda_destroy(gda gda) {
    size_t i;
    for(i=0;i<gda->pointers_count;i++) {
        if(gda->pointer_array[i] !=NULL) {
            gda->dtor(gda->pointer_array[i]);
        }
    }
    free(gda->pointer_array);
    free(gda);
}


/**
 * @brief Gets a pointer to the beginning of the gda.
 * 
 * @param gda 
 * @return void *const* Pointer to the beginning
 */
void *const * gda_get_begin_ptr(gda gda) {
    return gda->pointer_array;
}

/**
 * @brief Gets a pointer to the end of the gda.
 * 
 * @param gda 
 * @return void *const* Pointer to the end
 */
void *const * gda_get_end_ptr(gda gda) {
    return (gda->pointer_array + gda->pointers_count) -1;
}

/**
 * @brief Returns the size of the gda.
 * 
 * @param gda 
 * @return size_t Size of the gda
 */
size_t gda_size(gda gda) {
    return gda->elem_count;
}