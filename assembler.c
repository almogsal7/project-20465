#include "../inc/assembler.h"
#include "../../pre-asm/inc/pre-asm.h"
#include "../../lang-engine/inc/lang-engine.h"
#include "../inc/translation_unit.h"
#include "../../out/inc/out.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


#define TERMINAL_RED     "\x1b[31m"
#define TERMINAL_YELLOW  "\x1b[33m"
#define TERMINAL_RESET   "\x1b[0m"

#define PROG_BASE_ADDR 100

/* String representations of symbol types. */
static const char *sym_type_str[6] = {
    "data symbol",
    "code symbol",
    "external symbol",
    "entry symbol",
    "code entry symbol",
    "data entry symbol"
};

/**
 * @brief Constructs a new symbol table entry by copying an existing one.
 * @param copy Pointer to the existing symbol table entry.
 * @return Pointer to the newly created symbol table entry.
 */
static void *symbol_table_ctor(const void * copy) {
    return memcpy(malloc(sizeof(struct symbol)),copy,sizeof(struct symbol));
}
/**
 * @brief Destroys a symbol table entry.
 * @param copy Pointer to the symbol table entry to be destroyed.
 */
static void symbol_table_dtor(void * copy) {
    free(copy);
}
/**
 * @brief Compares two symbol table entries based on their symbol names.
 * @param a Pointer to the first symbol table entry.
 * @param b Pointer to the second symbol table entry.
 * @return An integer representing the comparison result.
 */
static int symbol_table_compar(const void *a , const void * b) {
    const struct symbol *ap = a;
    const struct symbol *bp = b;
    return strcmp(ap->symbol_name,bp->symbol_name);
}
/**
 * @brief Constructs a new binary machine code entry by copying an existing one.
 * @param copy Pointer to the existing binary machine code entry.
 * @return Pointer to the newly created binary machine code entry.
 */
static void * bmc_ctor(const void * copy) {
    return memcpy(malloc(sizeof(unsigned short)),copy,sizeof(unsigned short));
}
/**
 * @brief Destroys an extern call entry.
 * @param copy Pointer to the extern call entry to be destroyed.
 */
static void bmc_dtor(void * copy) {
    free(copy);
}
/**
 * @brief Constructs a new extern call entry by copying an existing one.
 * @param copy Pointer to the existing extern call entry.
 * @return Pointer to the newly created extern call entry.
 */
static void * extern_call_ctor(const void * copy) {
    struct extern_call * e_call =  malloc(sizeof(struct extern_call));
    strcpy(e_call->symbol_name,((struct extern_call *)copy)->symbol_name);
    e_call->addresses = ((struct extern_call *)copy)->addresses;
    return e_call;
}
/**
 * @brief Destroys an extern call entry.
 * @param copy Pointer to the extern call entry to be destroyed.
 */
static void extern_call_dtor(void * copy) {
    const struct extern_call * e_call = copy;
    gda_destroy(e_call->addresses);
    free(copy);
}
/**
 * @brief Compares two extern call entries based on their symbol names.
 * @param a Pointer to the first extern call entry.
 * @param b Pointer to the second extern call entry.
 * @return An integer representing the comparison result.
 */
static int extern_call_compar(const void *a, const void * b) {
    const struct extern_call * e_call1 = a;
    const struct extern_call * e_call2 = b;
    return strcmp(e_call1->symbol_name,e_call2->symbol_name);
}
/**
 * @brief Creates a new translation unit with initialized data structures.
 * @return A new translation_unit structure.
 */
static struct translation_unit assembler_create_new_translation_unit() {
    struct translation_unit t_unit = {0};
    t_unit.bmc_code = gda_create(bmc_ctor,bmc_dtor,NULL);
    t_unit.bmc_data = gda_create(bmc_ctor,bmc_dtor,NULL);
    t_unit.symbol_table = gda_create(symbol_table_ctor,symbol_table_dtor,symbol_table_compar);
    t_unit.extern_usage = gda_create(extern_call_ctor,extern_call_dtor,extern_call_compar);
   return t_unit;
}
/**
 * @brief Destroys a translation unit and frees its resources.
 * @param t_unit Pointer to the translation unit to be destroyed.
 */
static void assembler_destroy_translation_unit(struct translation_unit * t_unit) {
    gda_destroy(t_unit->bmc_code);
    gda_destroy(t_unit->bmc_data);    
    gda_destroy(t_unit->extern_usage);
    gda_destroy(t_unit->symbol_table);
} 
/**
 * @brief Prints an error message with file name, line number, and a custom message.
 * @param file_name The name of the file where the error occurred.
 * @param line The line number where the error occurred.
 * @param fmt A format string for the custom error message.
 * @param ... Variable arguments for the format string.
 */
static void asm_error_printer(const char *file_name,int line, const char * fmt,... ) {
    va_list arg;
    printf("%s:%d: ",file_name,line);
    printf(TERMINAL_RED "error: " TERMINAL_RESET);
    va_start (arg, fmt);
    
    vprintf(fmt, arg);
    va_end(arg);
}
/**
 * @brief Prints a warning message with file name, line number, and a custom message.
 * @param file_name The name of the file where the warning occurred.
 * @param line The line number where the warning occurred.
 * @param fmt A format string for the custom warning message.
 * @param ... Variable arguments for the format string.
 */
static void asm_warning_printer(const char *file_name,int line, const char * fmt,... ) {
    va_list arg;
    printf("%s:%d: ",file_name,line);
    printf(TERMINAL_YELLOW "warning: " TERMINAL_RESET);
    va_start (arg, fmt);
    
    vprintf(fmt, arg);
    va_end(arg);
}
/**
 * @brief Performs the first pass of the assembler to populate the symbol table.
 * @param symbol_table The gda symbol table to be populated.
 * @param am_file The input assembly file to be processed.
 * @param file_name The name of the input assembly file for error and warning messages.
 * @return Returns 0 if successful, -1 if a syntax error is found, or 1 if other errors are found.
 */
static int assembler_first_pass_symbol_table(gda symbol_table, FILE * am_file,const char * file_name) {
    char buffer[max_line_size + 1] = {0};
    struct syntax_struct s_struct;
    struct symbol * in_table = NULL;
    struct symbol dummy;
    int line_count = 1;
    int error =0;
    int IC = PROG_BASE_ADDR,DC = 0;
    void *const* sym_table_it_begin;
    void *const* sym_table_it_end;
    /* Read lines from the input assembly file */
    while(fgets(buffer,max_line_size,am_file)) {
        /* Create a syntax_struct from the logical line */
        s_struct = lang_engine_create_ss_from_logical_line(buffer);
        /* Process the syntax_struct based on its tag (instruction, directive, or error) */
        switch (s_struct.dir_or_inst_tag)
        {
        case tag_syntax_error:
        /* Return an error if a syntax error is found */
            asm_error_printer(file_name,line_count,"syntax: %s\n",s_struct.syntax_error_buffer);
            error =1;
            break;
        case tag_inst:
            /* Check if the symbol is not empty */
            if(s_struct.symbol[0] !='\0') {
                /* Copy the symbol to the dummy symbol */
                strcpy(dummy.symbol_name,s_struct.symbol);
                /* Search for the symbol in the symbol table */
                in_table = gda_search(symbol_table,&dummy);
                /* If the symbol is found in the table, process it accordingly */
                if(in_table) {
                    switch (in_table->sym_type)
                    {
                        /* Update the symbol type and address */
                    case sym_type_entry:
                        in_table->addr      = IC;
                        in_table->sym_type  = sym_type_code_entry;
                        break;
                    
                    default: /* all other cases are of course errors....*/
                        asm_error_printer(file_name,line_count,"symbol is being defined as '%s' but was defined before as '%s' in line %d.\n",sym_type_str[sym_type_code],sym_type_str[in_table->sym_type],in_table->line_def);
                        error =1;
                        break;
                    }
                }else {
                    /* If the symbol is not found in the table, insert it */
                    dummy.sym_type = sym_type_code;
                    dummy.addr     = IC;
                    dummy.line_def = line_count;
                    gda_insert(symbol_table,&dummy);
                }
            }
             /* Increment the instruction counter (IC) */
            IC++;
            /* Check if instruction belongs to group A */
            if(is_i_tag_groupA(s_struct.asm_directive_and_cpu_inst.cpu_inst.i_tag)) {
                if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[0] == tag_arg_tag_register && 
                    s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[1] == tag_arg_tag_register)
                        IC++;/* two registers*/
                else {
                    IC+=2;
                }
                /* Check if instruction belongs to group B */
            }else if (is_i_tag_groupB(s_struct.asm_directive_and_cpu_inst.cpu_inst.i_tag)) {
                if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.arg_options == tag_arg_2_args_with_symbol) {
                        if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[0] == tag_arg_tag_register &&
                            s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[1] == tag_arg_tag_register ) {
                                IC+=2;
                            }
                            else {
                                IC+=3;
                            }
                }else {
                    IC+=1;
                }
            }
            break;
        case tag_dir:
        /* If the syntax_struct is a directive */
            /* Check the directive tag (extern, entry, string, data) */
            if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_extern || s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_entry ) {
                /* Copy the directive symbol to the dummy symbol */
                strcpy(dummy.symbol_name,s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.symbol);
                /*Search for the symbol in the symbol table */
                in_table = gda_search(symbol_table,&dummy);
            }
            /* If the directive is an extern directive */
            if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_extern) {
                /* If the symbol is found in the table, process it accordingly */
                if(in_table) {
                    switch (in_table->sym_type)
                    {
                    case sym_type_extern:
                        /* warning redefinition as extern...*/
                        asm_warning_printer(file_name,line_count,"symbol:'%s' was already defined as '%s' in line %d.\n",in_table->symbol_name,sym_type_str[sym_type_extern],in_table->line_def);
                        break;

                    default:
                        /* error for the rest of the cases DUHHH*/
                        asm_error_printer(file_name,line_count,"symbol:'%s' was defined in line %d as '%s' and now is being defined as '%s'.\n",in_table->symbol_name,in_table->line_def,sym_type_str[in_table->sym_type],sym_type_str[sym_type_extern]);
                        error =1;
                        break;
                    }
                }else {
                     /* If the symbol is not found in the table, insert it */
                    dummy.sym_type = sym_type_extern;
                    dummy.line_def = line_count;
                    gda_insert(symbol_table,&dummy);
                }
                /* If the directive is an entry directive */
            }else if (s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_entry) {
                if(in_table) {
                    switch (in_table->sym_type)
                    {
                    case sym_type_data:
                        in_table->sym_type= sym_type_data_entry;
                        break;
                    case sym_type_code:
                        in_table->sym_type= sym_type_code_entry;
                        break;
                    case sym_type_extern:
                        asm_error_printer(file_name,line_count,"symbol:'%s' was defined as '%s' in line %d but now it's being redefined as '%s'\n",in_table->symbol_name,sym_type_str[in_table->sym_type],in_table->line_def,sym_type_str[sym_type_extern]);
                        error = 1;
                        /* error what the fuck ? cant be extern and entry !*/
                        break;
                    case sym_type_entry: case sym_type_code_entry: case sym_type_data_entry:
                        /* warning you are trying to redfine this symbol as entry*/
                        asm_warning_printer(file_name,line_count,"symbol:'%s' was already defined as '%s' in line %d.\n",in_table->symbol_name,sym_type_str[sym_type_entry],in_table->line_def);
                        break;
                    default:
                        break;
                    }
                }else {
                    dummy.sym_type = sym_type_entry;
                    dummy.line_def = line_count;
                    gda_insert(symbol_table,&dummy);
                }
            }
            else if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_string || s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_data) {
                if(s_struct.symbol[0] == '\0') {
                    /* warning inserting data or string without a pointing symbol..... how you gonna use it ?*/
                    asm_warning_printer(file_name,line_count,"data or string directive without a pointing symbol.\n");
                }else {
                    strcpy(dummy.symbol_name,s_struct.symbol);
                    in_table = gda_search(symbol_table,&dummy);
                    if(in_table) {
                        switch (in_table->sym_type)
                        {
                            case sym_type_entry:
                            
                            in_table->sym_type= sym_type_data_entry;
                            in_table->addr = DC;
                            if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_string)
                                DC += strlen(s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.string) + 1;
                            else 
                                DC += s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_count;
                            break;
                        default: 
                            /* error redefinition now it's ...*/
                            asm_error_printer(file_name,line_count,"symbol :'%s' was defined as '%s' in line  %d and now it's being redefined as '%s'.\n",in_table->symbol_name,sym_type_str[in_table->sym_type],in_table->line_def,sym_type_str[sym_type_data_entry]);
                            error =1;
                            break;
                        }
                    }else {
                        /* If the symbol is not found in the table, insert it */
                        dummy.addr = DC;
                        dummy.sym_type = sym_type_data;
                        dummy.line_def = line_count;
                        if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_string)
                            DC += strlen(s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.string) + 1;
                        else 
                            DC += s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_count;
                        
        
                        gda_insert(symbol_table,&dummy);
                    }
                }
            }
            break;
        default:
            break;
        }
        line_count++;
    }
    gda_for_each(symbol_table,sym_table_it_begin,sym_table_it_end) {
       if(*sym_table_it_begin) {
        in_table = *sym_table_it_begin;
        if(in_table->sym_type == sym_type_data || in_table->sym_type == sym_type_data_entry )
            in_table->addr +=IC;
       } else if (in_table->sym_type == sym_type_entry) {
        /* error , it was declared as entry but was never defined in this file....!*/
        asm_error_printer(file_name,line_count,"symbol : '%s' was declared as '%s' in line %d but was never defined.\n",in_table->symbol_name,sym_type_str[in_table->sym_type],in_table->line_def);
        error = 1;
       }
    }
    return error;
}
/**
@brief Performs the second pass of the assembler to generate the binary machine code.
@param t_unit The translation unit containing the gda symbol table, bmc_code, and bmc_data.
@param am_file The input assembly file to be processed.
@param file_name The name of the input assembly file for error and warning messages.
@return Returns 0 if successful, -1 if a syntax error is found, or 1 if other errors are found.
*/
static int assembler_second_pass(struct translation_unit * t_unit, FILE * am_file,const char * file_name) {
    /* Initialize local variables */
    char buffer[max_line_size + 1] = {0};
    struct syntax_struct s_struct;
     struct extern_call e_call_dummy = {0};
    struct extern_call *e_call_find = NULL;
    unsigned short temp;
    struct symbol * f_sym;
    unsigned short bmc_code_i = 0;
    int line_counter = 1;
    char *it;
    int i, error =0;
    /*Iterate through each line of the input assembly file*/
    while(fgets(buffer,max_line_size,am_file)) {
        /* Parse the current line and store the result in a syntax_struct */
        s_struct = lang_engine_create_ss_from_logical_line(buffer);
        /* Process the line based on the tag (directive, instruction, or syntax error/null line) */
        switch (s_struct.dir_or_inst_tag) {
            /*Ignore syntax errors or null lines*/
            case tag_syntax_error: case tag_line_null:
            break;
            /* Handle an assembly instruction */
            case tag_inst:
                bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.i_tag << 6;
                /* Check if the instruction belongs to group A */
                if(is_i_tag_groupA(s_struct.asm_directive_and_cpu_inst.cpu_inst.i_tag)) {
                    bmc_code_i |= s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[0] << 4;
                    bmc_code_i |= s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[1] << 2;
                    gda_insert(t_unit->bmc_code,&bmc_code_i);
                    if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[0] == tag_arg_tag_register && 
                        s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[1] == tag_arg_tag_register) {
                            bmc_code_i = (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[0].register_number << 8) |
                             (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[1].register_number << 2);
                             gda_insert(t_unit->bmc_code,&bmc_code_i);
                        }else {
                            for(i=0;i<2;i++) {
                                switch (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[i])
                                {
                                case tag_arg_tag_register:
                                    bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].register_number << (8 - (i * 6));
                                    break;
                                case tag_arg_tag_constant:
                                    bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].constant_number << 2;
                                    break;
                                case tag_arg_tag_symbol:
                                    f_sym = gda_search(t_unit->symbol_table,s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].symbol);
                                    if(f_sym == NULL) {
                                        /* error couldn't find the symbol in the sym table...*/
                                        asm_error_printer(file_name,line_counter,"undefined symbol: '%s'.",s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].symbol);
                                        error =1;
                                    }else {
                                        if(f_sym->sym_type == sym_type_extern) {
                                            bmc_code_i = 1;
                                            strcpy(e_call_dummy.symbol_name,f_sym->symbol_name);
                                            e_call_find = gda_search(t_unit->extern_usage,&e_call_dummy);
                                            temp = gda_size(t_unit->bmc_code) + PROG_BASE_ADDR;
                                            if(e_call_find) {
                                                gda_insert(e_call_find->addresses,&temp);
                                            }else {
                                                e_call_dummy.addresses = gda_create(bmc_ctor,bmc_dtor,NULL);
                                                gda_insert(e_call_dummy.addresses,&temp);
                                                gda_insert(t_unit->extern_usage,&e_call_dummy);
                                            }
                                        }else {
                                            bmc_code_i = (f_sym->addr << 2) | 2;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                                gda_insert(t_unit->bmc_code,&bmc_code_i);
                            }
                            
                        }
                }
                 /* Process group B instructions */
                else if(is_i_tag_groupB(s_struct.asm_directive_and_cpu_inst.cpu_inst.i_tag)) {
                    if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.arg_options == tag_arg_2_args_with_symbol) {
                        bmc_code_i |= (2 << 2);
                        bmc_code_i |= s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[0] << 12;
                        bmc_code_i |= s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[1] << 10;
                        gda_insert(t_unit->bmc_code,&bmc_code_i);
                        f_sym = gda_search(t_unit->symbol_table,s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.symbol);
                        if(f_sym) {
                            if(f_sym->sym_type == sym_type_extern) {
                                bmc_code_i = 1;
                                            strcpy(e_call_dummy.symbol_name,f_sym->symbol_name);
                                            e_call_find = gda_search(t_unit->extern_usage,&e_call_dummy);
                                            temp = gda_size(t_unit->bmc_code) + PROG_BASE_ADDR;
                                            if(e_call_find) {
                                                gda_insert(e_call_find->addresses,&temp);
                                            }else {
                                                e_call_dummy.addresses = gda_create(bmc_ctor,bmc_dtor,NULL);
                                                gda_insert(e_call_dummy.addresses,&temp);
                                                gda_insert(t_unit->extern_usage,&e_call_dummy);
                                            }
                            }else {
                                bmc_code_i = (f_sym->addr <<2) | 2;
                            }
                            gda_insert(t_unit->bmc_code,&bmc_code_i);
                        }else {
                            /* error couldn't find the symbol in the sym table...*/
                            asm_error_printer(file_name,line_counter,"undefined symbol: '%s'.\n",s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.symbol);
                            error = 1;
                        }
                        if(s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[0] == tag_arg_tag_register &&
                            s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[1] == tag_arg_tag_register) {
                                bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[0].register_number << 8 |
                                            s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[1].register_number << 2;
                                gda_insert(t_unit->bmc_code,&bmc_code_i);
                            }
                        else {
                            for(i=0;i<2;i++) {
                                switch (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[i])
                                {
                                case tag_arg_tag_register:
                                    bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].register_number<< (8 - (i * 6));
                                    break;
                                case tag_arg_tag_constant:
                                    bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].constant_number << 2;
                                    break;
                                case tag_arg_tag_symbol:
                                    f_sym = gda_search(t_unit->symbol_table,s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].symbol);
                                    if(f_sym) {
                                        if(f_sym->sym_type == sym_type_extern) {
                                            bmc_code_i = 1;
                                            strcpy(e_call_dummy.symbol_name,f_sym->symbol_name);
                                            e_call_find = gda_search(t_unit->extern_usage,&e_call_dummy);
                                            temp = gda_size(t_unit->bmc_code) + PROG_BASE_ADDR;
                                            if(e_call_find) {
                                                gda_insert(e_call_find->addresses,&temp);
                                            }else {
                                                e_call_dummy.addresses = gda_create(bmc_ctor,bmc_dtor,NULL);
                                                gda_insert(e_call_dummy.addresses,&temp);
                                                gda_insert(t_unit->extern_usage,&e_call_dummy);
                                            }
                                        }else {
                                            bmc_code_i = (f_sym->addr << 2) | 2;
                                        }

                                        
                                    }else {
                                        /* error couldn't find the symbol in the sym table...*/
                                        asm_error_printer(file_name,line_counter,"undefined symbol: '%s'.\n",s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].symbol);
                                        error = 1;
                                    }
                                    break;
                                default:
                                    break;
                                }
                                gda_insert(t_unit->bmc_code,&bmc_code_i);
                            }
                        }
                    }else {
                        bmc_code_i |= (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_opt << 2);
                        gda_insert(t_unit->bmc_code,&bmc_code_i);
                        switch (s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_opt)
                        {
                        case tag_arg_tag_register:
                             bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.register_number << (8 - (i * 6));
                            break;
                        case tag_arg_tag_constant:
                            bmc_code_i = s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.constant_number << 2;
                            break;
                        case tag_arg_tag_symbol:
                         f_sym = gda_search(t_unit->symbol_table,s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.symbol);
                            if(f_sym == NULL) {
                                        /* error couldn't find the symbol in the sym table...*/
                                asm_error_printer(file_name,line_counter,"undefined symbol: '%s'.\n",s_struct.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.symbol);
                                error = 1;
                            }else {
                                if(f_sym->sym_type == sym_type_extern) {
                                bmc_code_i = 1;
                                            strcpy(e_call_dummy.symbol_name,f_sym->symbol_name);
                                            e_call_find = gda_search(t_unit->extern_usage,&e_call_dummy);
                                            temp = gda_size(t_unit->bmc_code) + PROG_BASE_ADDR;
                                            if(e_call_find) {
                                                gda_insert(e_call_find->addresses,&temp);
                                            }else {
                                                e_call_dummy.addresses = gda_create(bmc_ctor,bmc_dtor,NULL);
                                                gda_insert(e_call_dummy.addresses,&temp);
                                                gda_insert(t_unit->extern_usage,&e_call_dummy);
                                            }
                                }else {
                                    bmc_code_i = (f_sym->addr << 2) | 2;
                                }
                                
                            }
                            break;
                     }
                     gda_insert(t_unit->bmc_code,&bmc_code_i);
                    }
                }else {
                    gda_insert(t_unit->bmc_code,&bmc_code_i);
                }
                break;
            case tag_dir:
            /* Handle an assembly directive */
                if(s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_string) {
                    for(it =s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.string;*it;it++) {
                        bmc_code_i = *it;
                        gda_insert(t_unit->bmc_data,&bmc_code_i);
                    }
                    bmc_code_i = 0;
                    gda_insert(t_unit->bmc_data,&bmc_code_i);
                }else if (s_struct.asm_directive_and_cpu_inst.asm_directive.d_tag == tag_data){
                    /* Process data directive */
                    for(i=0;i<s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_count;i++) {
                        bmc_code_i =s_struct.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_array[i];
                        gda_insert(t_unit->bmc_data,&bmc_code_i);
                    }
                }
                break;
        }
        /*Increment the line counter */
        line_counter++;
    }
    /*Return any errors encountered during the second pass*/
    return error;
}
int assemble( char **files,int file_count) {
    int i;
    const char *am_file_name;
    struct translation_unit t_unit;
    FILE * am_file;
    for(i=0;i<file_count;i++) {
        am_file_name = asm_pre_asm(files[i]);
        if(!am_file_name ) /* failed to macro parsed .. do what ever...*/ {

        }else {
            am_file = fopen(am_file_name,"r");
            if(!am_file) {

            }else {
                t_unit = assembler_create_new_translation_unit();
                if(assembler_first_pass_symbol_table(t_unit.symbol_table,am_file,am_file_name) == 0 ) {
                    rewind(am_file);
                    if(assembler_second_pass(&t_unit,am_file,am_file_name) == 0) {
                        if(out_print_translation_unit(&t_unit,files[i])) {

                        }
                    }
                }
                fclose(am_file);
                assembler_destroy_translation_unit(&t_unit);
                free((void*)am_file_name);
            }
            
        }
    }
    return 0;
}