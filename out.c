
#include "../inc/out.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/**
 * @brief called only if there are externs for the program.
 * 
 * @param externs_list cannot be empty.. obviously..
 * @param ext_file cannot be null and must be valid obviously..
 */
static void out_print_externs(gda externs_list,FILE * const ext_file) {
    void *const* begin;
    void *const* begin_addr;
    void *const* end;
    void *const* end_addr;
    const struct extern_call * ec;
    gda_for_each(externs_list,begin,end) {
        if(*begin !=NULL) {
            ec = *begin;
            gda_for_each(ec->addresses,begin_addr,end_addr) {
                if(*begin_addr)
                    fprintf(ext_file,"%s\t%hu\n",ec->symbol_name,*(unsigned short *)(*begin_addr));
            }
        }
    }
}

/**
 * @brief Prints the entry symbols and their addresses to a file with the extension .ent.
 * 
 * @param symbol_table the symbol table
 * @param base_name the base name of the output files
 * @return int 0 if successful, -1 otherwise 
 */

static int out_print_entry(gda symbol_table, const char *base_name) {
    void *const* begin;
    void *const* end;
    const struct symbol *symbol;
    FILE * ent_file = NULL;
    char * ent_file_name = NULL;
    gda_for_each(symbol_table, begin, end) {
        if (*begin != NULL) {
            symbol = *begin;
            if (symbol->sym_type == sym_type_code_entry || symbol->sym_type == sym_type_data_entry) {
                if(ent_file == NULL) {
                    ent_file_name = malloc(strlen(base_name) + 5);
                    strcat(strcpy(ent_file_name,base_name),".ent");
                    ent_file = fopen(ent_file_name,"w");
                }
                fprintf(ent_file, "%s\t%u\n", symbol->symbol_name, symbol->addr);
            }
        }
    }
    if(ent_file)
        free(ent_file_name);
    if(ent_file)
        fclose(ent_file);
    return 0;
}

/**
 * @brief Prints to the object file, for the given binary machine code.
 * 
 * @param bmc_code Binary machine code for the program code. 
 * @param bmc_data Binary machine code for the program data. 
 * @param ob_file ob_file Pointer to the output file for the object file.
 * @return int 
 */
static int out_print_ob(gda bmc_code, gda bmc_data, FILE * const ob_file) {
   void *const* bmc_it_begin;
    void *const* bmc_it_end;
    gda it;
    int i;
    unsigned short code;
    fprintf(ob_file,"%d\t%d\n",gda_size(bmc_code),gda_size(bmc_data));
    for(it = bmc_code ;1;it=bmc_data){
        gda_for_each(it, bmc_it_begin, bmc_it_end) {
            if (*bmc_it_begin != NULL) {
                code = *(unsigned short *)*bmc_it_begin;
                for(i=0;i<14;i++, code <<=1) {
                    fprintf(ob_file,code & 0x2000 ? "/" : ".");
                }
                fprintf(ob_file,"\n");
            }
        }
        fprintf(ob_file,"\n");
        if(it == bmc_data)
            break;
    }
    fclose(ob_file);
    return 0;
}
 /**
  * @brief Prints the output files (.ext, .ent, .ob) for a given translation unit

@param tu Pointer to the translation unit to print output files for

@param base_name Base name for the output files

@return int Returns 0 upon successful completion, non-zero otherwise
 */
int out_print_translation_unit(const struct translation_unit * tu,const char *base_name) {
    FILE * out;
    char * out_file_name;
    if(gda_size(tu->extern_usage) > 0) {
        out_file_name = malloc(strlen(base_name) + 5);
        strcat(strcpy(out_file_name,base_name),".ext");
        out = fopen(out_file_name,"w");
        if(out) {
            out_print_externs(tu->extern_usage,out);
        }else {

        }
        free(out_file_name);
        fclose(out);
    }
    out_print_entry(tu->symbol_table,base_name);
    out_file_name = malloc(strlen(base_name) + 4);
    strcat(strcpy(out_file_name,base_name),".ob");
    out_print_ob(tu->bmc_code,tu->bmc_data,fopen(out_file_name,"w"));
    free(out_file_name);
    return 0;
}