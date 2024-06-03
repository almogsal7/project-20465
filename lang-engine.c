#include "../inc/lang-engine.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#define MIN_C_NUMBER -8192
#define MAX_C_NUMBER  8191
#define MAX_REG_NUM     7
#define MIN_REG_NUM     0
#define SKIP_SPACE(ptr) while(isspace(*ptr)) ptr++
#define SPACES "\n \r\t\f\v"
#define END_LINE '\0'
/* Structure representing a single CPU instruction */
struct cpu_instruction {
    const char *inst_name; /* Name of the CPU instruction */
    enum inst_tag i_tag;  /* Associated instruction tag */
};
/* Array of CPU instructions and their corresponding instruction tags */
static const struct cpu_instruction i_map[16] = {
    {"add",tag_add},
    {"bne",tag_bne},
    {"clr",tag_clr},
    {"cmp",tag_cmp},
    {"dec",tag_dec},
    {"inc",tag_inc},
    {"jmp",tag_jmp},
    {"jsr",tag_jsr},
    {"lea",tag_lea},
    {"mov",tag_mov},
    {"not",tag_not},
    {"prn",tag_prn},
    {"red",tag_red},
    {"rts",tag_rts},
    {"stop",tag_stop},
    {"sub",tag_sub}
};
/* Structure representing an assembly directive */
struct asm_directive {
    const char *dir_name; /* Name of the assembly directive */
    enum dir_tag d_tag; /* Associated directive tag */
};
/* Enumeration representing various symbol validation tags */
enum sst_symbol_valid_tag {
    sst_symbol_ok = -1,
    sst_symbol_starts_no_alpha,
    sst_symbol_contains_no_alpha_numeric,
    sst_symbol_too_long
};
/* Enumeration representing various number validation tags */
enum sst_number_valid_tag {
    sst_number_ok = -1,
    sst_number_overflows,
    sst_number_bigger_than_max,
    sst_number_lower_than_min,
    sst_number_invalid_number
};
/* Array of symbol validation error strings */
static const char *sst_symbol_valid_tag_str_error[3] = {
    "symbol starts with non alpha character.",
    "symbol contains a non alpha numeric character.",
    "symbol is too long"
};
/* Array of number validation error strings */
static const char *sst_number_valid_str_error[4] = {
    "out of range (over-flow).",
    "bigger than maximum",
    "lower than minimum",
    "invalid number"
};
/* Array of assembly directives and their corresponding directive tags */
static const struct asm_directive asm_dirs[4] = {
    {".data",tag_data},
    {".entry",tag_entry},
    {".extern",tag_extern},
    {".string",tag_string}
};
/* Comparison function for CPU instructions used by bsearch() */
int cpu_i_compar(const void *a, const void *b) {
    const struct cpu_instruction * pa =a;
    const struct cpu_instruction * pb =b;
    return strcmp(pa->inst_name,pb->inst_name);
}

/* Function that checks if a string is a valid CPU instruction */
static struct cpu_instruction * lang_engine_is_string_a_cpu_inst(const char * string) {
    struct cpu_instruction inst;
    inst.inst_name = string;
    return bsearch(&inst,i_map,16,sizeof(struct cpu_instruction),cpu_i_compar);
}


/* Function that checks if a string is a valid assembly directive */
static struct asm_directive * lang_engine_is_string_a_asm_dir(const char * string) {
    struct asm_directive dir;
    dir.dir_name = string;
    return bsearch(&dir,&asm_dirs[0],4,sizeof(struct asm_directive),cpu_i_compar);
}
/* Function that validates a given symbol */
static enum sst_symbol_valid_tag lang_engine_symbol_validation(const char * string) {
    size_t str_len = 0;
    if(!isalpha(*string))
        return sst_symbol_starts_no_alpha;
    string++;
    while(*string && str_len <= max_symbol_len) {
        if(!isalnum(*string))
            return sst_symbol_contains_no_alpha_numeric;
        string++;
        str_len++;
    }
    if(str_len > max_symbol_len)
        return sst_symbol_too_long;
    return sst_symbol_ok;
}
/* Function that sets the result of the syntax structure as an error */
void static lang_engine_set_result_as_error(struct syntax_struct *ss,const char *format,...) {
    va_list args;
    va_start(args,format);
    vsprintf(ss->syntax_error_buffer,format,args);
    ss->dir_or_inst_tag = tag_syntax_error;
    va_end(args);
}
/* Function that validates a given number */
static enum sst_number_valid_tag lang_engine_number_validation(const char * string, char ** endptr,int * num,const int max, const int min) {
     char * end;
     errno = 0;
    *num = strtol(string,&end,10);
    
    if(end == string) {
        return sst_number_invalid_number;
    }
    if(errno == ERANGE)
        return sst_number_overflows;
    if((*num) > max)
        return sst_number_bigger_than_max;
    if((*num) < min)
        return sst_number_lower_than_min;

    
    *endptr = end;
    return sst_number_ok;
}
/* Structure representing a text parameter */
struct text_param {
    char error_str[syntax_error_buf_len + 1];
    enum argument_option opt;
    union {
        int register_number;
        int constant_number;
        char symbol[max_symbol_len +1];
    }param;
};

/* Function that parses a parameter from the input text */
struct text_param lang_engine_parse_param(const char *param,char **endptr) {
    struct text_param tp_ret = {0};
        enum sst_symbol_valid_tag symbol_valid_temp;
    enum sst_number_valid_tag number_valid_temp;
    SKIP_SPACE(param);

    if(*param == 'r'){
        param++;
        number_valid_temp = lang_engine_number_validation(param,endptr,&tp_ret.param.register_number,MAX_REG_NUM,MIN_REG_NUM);
        if(number_valid_temp != sst_number_ok) {
            sprintf(tp_ret.error_str,"register is %s",sst_number_valid_str_error[number_valid_temp]);
        }
        tp_ret.opt = tag_arg_tag_register;
    }else if(*param == '#') {
        param++;
        number_valid_temp = lang_engine_number_validation(param,endptr,&tp_ret.param.constant_number,MAX_C_NUMBER,MIN_C_NUMBER);
        if(number_valid_temp != sst_number_ok) {
            sprintf(tp_ret.error_str,"constant number is %s",sst_number_valid_str_error[number_valid_temp]);
        }
        tp_ret.opt = tag_arg_tag_constant;
    }
    else { /* must be a label...*/
        tp_ret.opt = tag_arg_tag_symbol;
        symbol_valid_temp = lang_engine_symbol_validation(param);
            if(symbol_valid_temp != sst_symbol_ok)
                sprintf(tp_ret.error_str,"%s is %s.",param,sst_symbol_valid_tag_str_error[symbol_valid_temp]);
            else {
                strcpy(tp_ret.param.symbol,param);
            }
    }
    return tp_ret;
}
/**
 * @brief Creates a syntax_struct from a logical line of assembly code.
 *
 * This function takes a logical line of assembly code and creates a syntax_struct
 * which contains information about any symbols, instructions, or directives present
 * in the line. It also validates the syntax of the line and reports any errors
 * encountered.
 *
 * @param logical_line A string containing a logical line of assembly code.
 *
 * @return A syntax_struct containing information about any symbols, instructions,
 * or directives present in the logical line.
 */
struct syntax_struct lang_engine_create_ss_from_logical_line(char *logical_line) {
    /* Declare variables used in the function */
    struct syntax_struct result = {0};
    char * temp1;
    char * temp2;
    char * temp3;
    char * temp4;
    enum sst_symbol_valid_tag symbol_valid_temp;
    enum sst_number_valid_tag number_valid_temp;
    struct asm_directive * dir;
    struct cpu_instruction * inst;
    struct text_param tp;
    int i;
    /* Remove leading spaces from the logical line */
    SKIP_SPACE(logical_line);
    /* If the line is empty or a comment, return a null tag */
    if(*logical_line == END_LINE || *logical_line == ';') {
        result.dir_or_inst_tag = tag_line_null;
        return result;
    }
     /* Check for a symbol in the line */
    temp1 = strchr(logical_line,':');
    if(temp1) {
        /* Check for multiple colons */
        temp2 = strchr(temp1+1,':');
        if(temp2 != NULL) {
            lang_engine_set_result_as_error(&result,"%s","token ':' appears twice or more.");
            return result;
            /* Extract the symbol from the line */
        }
        /* symbol logic goes here*/
        *temp1 = END_LINE;
        if((symbol_valid_temp = lang_engine_symbol_validation(logical_line) )== sst_symbol_ok) {
            strcpy(result.symbol,logical_line);
        }else {
            lang_engine_set_result_as_error(&result,"'%s' %s",logical_line,sst_symbol_valid_tag_str_error[symbol_valid_temp]);
            return result;
        }
        *temp1 = ':';
        logical_line = temp1+1;
    }
    SKIP_SPACE(logical_line);
    /* handle symbol*/
    temp1 = strpbrk(logical_line,SPACES);
    if(temp1) {
        *temp1 = END_LINE; 
    }
    inst    = lang_engine_is_string_a_cpu_inst(logical_line);
    dir     = lang_engine_is_string_a_asm_dir(logical_line);
    if(!inst && !dir) {
        lang_engine_set_result_as_error(&result,"'%s' unknown key word.",logical_line);
        return result;        
    }
    /* Handle instructions */
    if(inst) {
        result.dir_or_inst_tag = tag_inst;
        result.asm_directive_and_cpu_inst.cpu_inst.i_tag = inst->i_tag;
        if(is_i_tag_groupA(inst->i_tag) || is_i_tag_groupB(inst->i_tag)) {
            if(!temp1) {
                lang_engine_set_result_as_error(&result,"no arguments for instruction: '%s'",inst->inst_name);
                return result;
            }
        }

        if(is_i_tag_groupA(inst->i_tag)) {
            /* Handle group A instructions */
        temp1++; /* Move to the the first character of the instruction name */
        SKIP_SPACE(temp1);
            temp2 = strchr(temp1,','); /* Find the first "," (if any) after the instruction name*/
            if(temp2 == NULL) {
                /* If "," is not found */
                lang_engine_set_result_as_error(&result,"expected separator ',' for cpu instruction: '%s'.",inst->inst_name);
                return result;
            }
            *temp2 = END_LINE;
            temp3 = strpbrk(temp1,SPACES);
                if(temp3) *temp3 = END_LINE;
            temp3 = temp2+1;
            SKIP_SPACE(temp3);
            temp3 = strpbrk(temp3,SPACES);
            if(temp3) {
                *temp3 = END_LINE;
                temp3++;
                SKIP_SPACE(temp3);
                if(*temp3 != END_LINE) {
                    lang_engine_set_result_as_error(&result,"extraneous text for cpu instruction: '%s'.",inst->inst_name);
                    return result;
                }
            }
            for(i =0;i<2;i++,temp1=temp2+1) {
                SKIP_SPACE(temp1);
                /* Parse the parameter using lang_engine_parse_param() */
                tp = lang_engine_parse_param(temp1,&temp3);
                /* Check if there was an error parsing the parameter */
                if(tp.error_str[0] !='\0') {
                    lang_engine_set_result_as_error(&result,"%s",tp.error_str);
                    return result;
                }
                /* Store the parsed parameter in the result struct */
                result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.left_and_right_args[i] = tp.opt;
                switch (tp.opt)
                {
                    /* Depending on the type of argument, store the corresponding value in the result struct */
                case tag_arg_tag_constant:
                    result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].constant_number = tp.param.constant_number;
                    break;
                case tag_arg_tag_register:
                    result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].register_number = tp.param.register_number;
                    break;
                case tag_arg_tag_symbol:
                    strcpy(result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_A.arg_option[i].symbol,tp.param.symbol);
                    break;
                }
            }
        }else if(is_i_tag_groupB(inst->i_tag)) {
            temp1++;
            SKIP_SPACE(temp1);
            temp2 = strchr(temp1,'(');
            temp3 = strchr(temp1+1,')');
            /* Missing closing brackets ')' token */
            if(temp3== NULL && temp2 !=NULL) {
                    lang_engine_set_result_as_error(&result,"missing closing brackets ')' token  for cpu instruction: '%s'.",inst->inst_name);
                    return result;
            }
             /* Missing opening brackets '(' token */
            else if(temp3 !=NULL && temp2 == NULL) {
                    lang_engine_set_result_as_error(&result,"missing opening brackets '(' token  for cpu instruction: '%s'.",inst->inst_name);
                    return result;
            }else if(temp3 != NULL && temp2 != NULL) {
                result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.arg_options = tag_arg_2_args_with_symbol;
                /* Closing brackets ')' appears before opening brackets '(' token */
                if(temp3 < temp2) {
                    lang_engine_set_result_as_error(&result,"closing brackets ')' appears before opening brackets '(' token  for cpu instruction: '%s'.",inst->inst_name);
                    return result;
                }
                *temp2 = END_LINE;
                temp4 = strpbrk(temp1,SPACES);
                    if(temp4) {
                        *temp4 = END_LINE;
                        lang_engine_set_result_as_error(&result,"label '%s' must appear next to opening brackets '(' without spaces for cpu instruction: '%s'.",temp1,inst->inst_name);
                        return result;
                    }
                symbol_valid_temp = lang_engine_symbol_validation(temp1);
                if(symbol_valid_temp != sst_symbol_ok) {
                        lang_engine_set_result_as_error(&result,"label '%s' is %s for cpu instruction: '%s'.",temp1,sst_number_valid_str_error[symbol_valid_temp],inst->inst_name);
                        return result;  
                }
                strcpy(result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.symbol,temp1);
                temp1 = temp2+1;
                SKIP_SPACE(temp1);
                temp2 = strchr(temp1,',');
                 /* Expected separator ',' */
                if(temp2== NULL) {
                    lang_engine_set_result_as_error(&result,"expected separator ',' for cpu instruction: '%s'.",inst->inst_name);
                    return result;
                }
                *temp2 = END_LINE;
                temp4 = strpbrk(temp1,SPACES);
                if(temp4) *temp4 = END_LINE;
                *temp3 = END_LINE;
                temp3++;
                SKIP_SPACE(temp3);
                /* Extraneous text */
                if(*temp3 != END_LINE) {
                    lang_engine_set_result_as_error(&result,"extraneous text for cpu instruction: '%s'.",inst->inst_name);
                    return result;
                }
                temp3 =temp2+1;
                SKIP_SPACE(temp3);
                temp3 = strpbrk(temp3,SPACES);
                if(temp3) *temp3 = END_LINE;
                for(i=0;i<2;i++,temp1= temp2+1){
                    SKIP_SPACE(temp1);
                    tp = lang_engine_parse_param(temp1,&temp3);
                    if(tp.error_str[0] !='\0') {
                        lang_engine_set_result_as_error(&result,"%s",tp.error_str);
                        return result;
                    }
                    result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.left_and_right_args[i]= tp.opt;
                    switch (tp.opt)
                    {
                    case tag_arg_tag_constant:
                        result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].constant_number = tp.param.constant_number;
                        break;
                    case tag_arg_tag_register:
                        result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].register_number = tp.param.register_number;
                        break;
                    case tag_arg_tag_symbol:
                        strcpy(result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.arg_2_symbol.arg_2_args_options[i].symbol,tp.param.symbol);
                        break;
                    }
                }
            }else /* other cases withoout label and brackets.. */{
                temp2 = strpbrk(temp1,SPACES); /* Find the first space character after the argument */
                if(temp2) {
                    *temp2 = END_LINE;
                    temp2++;
                    SKIP_SPACE(temp2);
                    if(*temp2 != END_LINE ) {
                        lang_engine_set_result_as_error(&result,"extraneous text for cpu instruction: '%s'.",inst->inst_name);
                        return result;
                    }
                }
                /* Parse the argument */
                tp = lang_engine_parse_param(temp1,&temp3);
                if(tp.error_str[0] !='\0') {
                    lang_engine_set_result_as_error(&result,"%s",tp.error_str);
                    return result;
                }
                /* Set the argument options based on the parsed parameter type */
                result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_opt = tp.opt;
                switch (tp.opt)
                {
                    /* If the parameter is a constant, set the constant number in the argument options */
                case tag_arg_tag_constant:
                    result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.constant_number = tp.param.constant_number;
                    break;
                    /* If the parameter is a register, set the register number in the argument options */
                case tag_arg_tag_register:
                    result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.register_number = tp.param.register_number;
                    break;
                    /* If the parameter is a symbol, copy the symbol name to the argument options */
                case tag_arg_tag_symbol:
                    strcpy(result.asm_directive_and_cpu_inst.cpu_inst.inst_arguments.group_B.sub_group_b.rest_of_group_b.arg_option.symbol,tp.param.symbol);
                    break;
                }
            }
        }
        else {/* group C*/
        /* Check if there is any text after the instruction */
            if(temp1) {
                temp1++;
                SKIP_SPACE(temp1);
                if(*temp1 != END_LINE && *temp1 !='\n') {
                    lang_engine_set_result_as_error(&result,"extraneous text for cpu instruction: '%s'.",inst->inst_name);
                    return result;
                }
            }
        }
    }
    /* Check if there is a directive present */
    if(dir) {
        /* Check if there are any arguments for the directive */
        if(!temp1) {
            lang_engine_set_result_as_error(&result,"no arguments for directive: '%s'",dir->dir_name);
            return result;
        }
        temp1++;
        result.asm_directive_and_cpu_inst.asm_directive.d_tag = dir->d_tag;
        SKIP_SPACE(temp1);
        switch (dir->d_tag)
        {
        case tag_entry: case tag_extern: {
             /* Check if there are any arguments for the directive */
                if(*temp1 == '\0') {
                    lang_engine_set_result_as_error(&result,"no arguments for directive: '%s'",dir->dir_name);
                    return result;
                }
                 /* Find the end of the argument and check for extraneous text */
                temp2 = strpbrk(temp1,SPACES);
                    if(temp2) {
                        *temp2 = END_LINE;
                        temp2++;
                        SKIP_SPACE(temp2);
                        if(*temp2 !='\0' && *temp2 !='\n') {
                            lang_engine_set_result_as_error(&result,"extraneous text for directive: '%s'.",dir->dir_name);
                            return result;
                        }
                    }
                    /* Check if the symbol is valid and copy it to the result */
            if((symbol_valid_temp = lang_engine_symbol_validation(temp1)) == sst_symbol_ok) {
                strcpy(result.asm_directive_and_cpu_inst.asm_directive.directive_union.symbol,temp1);
            }else {
                lang_engine_set_result_as_error(&result,"'%s' %s",temp1,sst_symbol_valid_tag_str_error[symbol_valid_temp]);
                return result;
            }
            break;
        }
            case tag_string:  {  
                 /* Find the beginning and ending quotes of the string */
                temp2 = strchr(temp1,'"');
                    if(temp2) {
                        
                            if(temp1 != temp2) {
                                lang_engine_set_result_as_error(&result," ending token '\"' without starting token '\"' for directive %s .",dir->dir_name);
                                return result;
                            }
                        temp2 = strchr(temp2+1,'"');
                            if(temp2) {
                                /* Copy the string to the result and check for extraneous text */
                                *temp2 = END_LINE;
                                strcpy(result.asm_directive_and_cpu_inst.asm_directive.directive_union.string,temp1+1);
                                temp2++;
                                SKIP_SPACE(temp2);
                                if(*temp2 != END_LINE) {
                                    lang_engine_set_result_as_error(&result,"extraneous text for directive: '%s'.",dir->dir_name);
                                    return result;
                                }
                            }else {
                                lang_engine_set_result_as_error(&result,"expected ending token '\"' for directive %s .",dir->dir_name);
                                return result;
                            }
                    }else {
                        lang_engine_set_result_as_error(&result,"expected starting token '\"' for directive %s .",dir->dir_name);
                        return result;
                    }
                break;
            }
            /* Parse the data values separated by "," */
            case tag_data: {
                for(i =0 ;i < max_data_in_a_line;i++) {
                    if((number_valid_temp = lang_engine_number_validation(temp1,&temp2,&result.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_array[i],MAX_C_NUMBER,MIN_C_NUMBER))  == sst_number_ok) {
                        if(*temp2 == '\n' || *temp2 == '\0')
                            break;
                        if(isspace(*temp2)){
                            SKIP_SPACE(temp2);
                                if(*temp2 == '\0')
                                    break;
                                if(*temp2 != ',') {
                                    lang_engine_set_result_as_error(&result,"expected separator ',' for directive '%s' ",dir->dir_name);
                                    return result;
                                }
                        }
                        if(*temp2 !=',') {
                            lang_engine_set_result_as_error(&result,"invalid character '%c' for directive '%s' ",*temp2,dir->dir_name);
                            return result;
                        }
                        temp1 = temp2+1;
                    }else {
                        lang_engine_set_result_as_error(&result,"%s",sst_number_valid_str_error[number_valid_temp]);
                        return result;
                    }
                }
                result.asm_directive_and_cpu_inst.asm_directive.directive_union.data_array.num_count = i + 1;
                break;
            }        
        }
    }

    return result;
}