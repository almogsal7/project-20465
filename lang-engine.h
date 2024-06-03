#ifndef maman14_lang_engine_h
#define maman14_lang_engine_h

#define max_symbol_len 30
#define syntax_error_buf_len 120
#define max_data_in_a_line 80
#define max_line_size 85

#define is_i_tag_groupA(i_tag) (i_tag >= tag_mov && i_tag <= tag_lea)
#define is_i_tag_groupB(i_tag) (i_tag >= tag_not && i_tag <= tag_jsr)
#define is_i_tag_groupC(i_tag) (i_tag >= tag_rts && i_tag <= tag_stop)

enum inst_tag{
    /* group A*/
    tag_mov,
    tag_cmp,
    tag_add,
    tag_sub,
    tag_lea,

    /* group B*/
    tag_not,
    tag_clr,
    tag_inc,
    tag_dec,
    tag_jmp,
    tag_bne,
    tag_red,
    tag_prn,
    tag_jsr, 

    /* group C*/
    tag_rts,
    tag_stop
};

enum dir_tag{
    tag_data, 
    tag_string, 
    tag_extern,  
    tag_entry  
};
enum argument_option{
    tag_arg_tag_constant, 
    tag_arg_tag_symbol,
    tag_arg_tag_register = 3
};


/* This struct in the heart of the project. it contains information about the type of directive or instruction, its arguments and any syntax errors. */
struct syntax_struct {
    char symbol[max_symbol_len +1 ];
    char syntax_error_buffer[syntax_error_buf_len + 1];
    enum {
        tag_dir = 0,
        tag_inst,
        tag_syntax_error,
        tag_line_null
    }dir_or_inst_tag;
    
    union {
        struct {
            enum dir_tag d_tag;
            union {
                char symbol[max_symbol_len +1];
                struct {
                    int num_array[max_data_in_a_line];
                    int num_count;
                }data_array;
                char string[max_line_size + 1];
            }directive_union;
        }asm_directive;

        struct {
            enum inst_tag i_tag;
            union {
                struct {
                    enum argument_option left_and_right_args[2];
                    union {
                        char register_number;
                        int constant_number;
                        char symbol[max_symbol_len +1];
                    }arg_option[2];
                }group_A;

                struct {
                    enum {
                        tag_1_arg,
                        tag_arg_2_args_with_symbol
                    }arg_options;
                    union {
                        struct {
                            char symbol[max_symbol_len +1];
                            enum argument_option left_and_right_args[2];
                            union {
                                char register_number;
                                int constant_number;
                                char symbol[max_symbol_len +1];
                            }arg_2_args_options[2];
                        }arg_2_symbol;

                        struct {
                            enum argument_option arg_opt;
                            union {
                                char register_number;
                                int constant_number;
                                char symbol[max_symbol_len +1];
                            }arg_option;
                        }rest_of_group_b;

                    }sub_group_b;
                }group_B;
            }inst_arguments;
        }cpu_inst;

    }asm_directive_and_cpu_inst;
};

/**
 * @brief 
 * 
 * @param logical_line 
 * @return struct syntax_struct 
 */
struct syntax_struct lang_engine_create_ss_from_logical_line(char *logical_line);





#endif
