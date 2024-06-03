#include "../inc/pre-asm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../utilities/generic-dynamic-array/inc/gda.h"
#include <ctype.h>
#define MAX_LINE_LEN 80
#define SPACES "\n \r\t\f\v"
#define SKIP_SPACE(ptr) while(isspace(*ptr)) ptr++
/*
 * The macro structure represents a macro definition,
 * containing the macro's name and the lines it is composed of.
 */
struct macro {
    char *macro_name;
    gda lines;
};

/**
 * @brief Create a new line object by deep copying the contents of the given line object.
 *
 * @param candidate A pointer to the line object to copy.
 * @return A pointer to the newly created line object.
 */
static void line_dtor(void * candidate) {
    free(candidate);
}
/**
 * @brief Create a new line object by deep copying the contents of the given line object.
 *
 * @param candidate A pointer to the line object to copy.
 * @return A pointer to the newly created line object.
 */
static void *line_ctor(const void *candidate) {
    void *ret;
    size_t str_len = strlen((const char *)candidate);
    ret = calloc(str_len + 1, sizeof(char));
    if (!ret)
        return NULL;
    strcpy((char *)ret, (const char *)candidate);
    return ret;
}


/**
 * @brief Create a new macro object by deep copying the contents of the given macro object.
 *
 * @param candidate A pointer to the macro object to copy.
 * @return A pointer to the newly created macro object.
 */
static void * macro_ctor(const void *candidate) {
    const struct macro * c = candidate;
    struct macro * ret = malloc(sizeof(struct macro));
    if(ret ==NULL)
        return NULL;
    ret->macro_name = malloc(strlen(c->macro_name) + 1);
    if(ret->macro_name == NULL) {
        free(ret);
        return NULL;
    }
    strcpy(ret->macro_name,c->macro_name);
    ret->lines = gda_create(line_ctor,line_dtor,NULL);
    return ret;
}

/**
 * @brief Destroy the given macro object and free its associated memory.
 *
 * @param candidate A pointer to the macro object to destroy.
 */
static void macro_dtor(void *candidate) {
    struct macro * c = candidate;
    free(c->macro_name);
    gda_destroy(c->lines);
}

/**
 * @brief Compare two macro objects based on their macro names.
 *
 * @param c1 A pointer to the first macro object.
 * @param c2 A pointer to the second macro object.
 * @return An integer representing the comparison result.
 */
static int macro_cmpr(const void *c1,const void *c2) {
    const struct macro * mc1 = c1;
    const struct macro * mc2 = c2;
    return strncmp(mc1->macro_name,mc2->macro_name, strlen(mc1->macro_name));
}



/* Enum with all the Line Types we can have */
enum line_type {
    macro_def,
    macro_end_def,
    macro_call,
    macro_any_line
};
/**
 * @brief Analyze a given line and determine its type, extracting the macro name if applicable, assuming no syntax error.
 *
 * @param line The input line to analyze.
 * @param macro_name A pointer to a char pointer that will hold the macro name (if applicable).
 * @return The line_type of the analyzed line.
 */
static enum line_type determine_line_type(char *line, char **macro_name,gda macro_table) {
    enum line_type type = macro_any_line;
    struct macro * find;
    struct macro temp = {0};
    char *temp1;
    char line_buffer[MAX_LINE_LEN] = {0};
    temp1 = strstr(line,"endmcr");
    if(temp1) {
        type = macro_end_def;
    }else if((temp1 = strstr(line,"mcr") )) {
        temp1+=3;
        SKIP_SPACE(temp1);
        line = temp1;
        temp1 = strpbrk(line,SPACES);
        if(temp1) *temp1 = '\0';
        (*macro_name) = line;
        type = macro_def;
    }else {
        SKIP_SPACE(line);
        strcpy(line_buffer,line);
        if(*line !='\0') {
            temp1 = strpbrk(line_buffer,SPACES);
            if(temp1) {
                *temp1 = '\0';
                temp1++;
                SKIP_SPACE(temp1);
                if(*temp1 !='\0') {
                    return type;
                }
            }

        }
        temp.macro_name = line;
        find = gda_search(macro_table,&temp);
        if(find) {
            *macro_name = line;
            type = macro_call;
        }
    }
    return type;
}

/**
 * @brief Process the input assembly code and replace macro calls with their definitions, writing the result to an output file.
 *
 * @param base_name The base name of the input assembly file.
 * @return A pointer to the string containing the output file name.
 */
const char * asm_pre_asm(const char *base_name) {
    gda macro_table;

    struct macro *macro_context = NULL;
    struct macro *sm  = NULL;
    char *as_name = NULL, *am_name = NULL;
    struct macro local_macro = {0};
    FILE *as_file = NULL, *am_file = NULL;
    size_t len;

    char line_buffer[MAX_LINE_LEN] = {0};
    void *const *begin;
    void *const *end;
    len = strlen(base_name) + 3;
    as_name = malloc(len + 1);
    am_name = malloc(len + 1);

    if (as_name == NULL || am_name == NULL)
        return NULL;

    strcat(strcpy(as_name, base_name), ".as");
    strcat(strcpy(am_name, base_name), ".am");


    as_file = fopen(as_name, "r");
    am_file = fopen(am_name, "w");
    if (as_file == NULL || am_file == NULL) {
        /* error printing...*/
        free(as_name);
        free(am_name);
        return NULL;
    }

    macro_table = gda_create(macro_ctor, macro_dtor, macro_cmpr);
    while (fgets(line_buffer, MAX_LINE_LEN, as_file)) {
        switch (determine_line_type(line_buffer, &local_macro.macro_name,macro_table)) {
            case macro_def:
                /* assuming no nested macro defs are given....*/
                /*local_macro.lines = gda_create(line_ctor, line_dtor, (int (*)(const void *, const void *)) strcmp); */
                macro_context = gda_insert(macro_table, &local_macro);
                break;

            case macro_end_def:
                if (macro_context == NULL) {
                    /* print error..*/
                } else {
                    macro_context = NULL;
                }
                break;

            case macro_call:
                sm = gda_search(macro_table, &local_macro);
                if (sm == NULL) {
                    /* no such macro... error.*/
                } else {
                    gda_for_each(sm->lines, begin, end) {
                        if (*begin) {
                            fprintf(am_file, "%s", (char *)(*begin));
                        }
                    }
                }
                break;

            case macro_any_line:
                if (macro_context == NULL) {
                        fprintf(am_file, "%s", line_buffer);
                } else {
                    gda_insert(macro_context->lines, line_buffer);
                }
                break;
        }
        memset(line_buffer,0,sizeof(line_buffer));
    }
    fclose(as_file);
    fclose(am_file);
    free(as_name);
    gda_destroy(macro_table);
    return am_name;
}