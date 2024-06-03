#ifndef TU_H
#define TU_H
#include "../../utilities/generic-dynamic-array/inc/gda.h"



struct symbol {
    char            symbol_name[32 + 1];
    unsigned int    addr;
    enum {
        sym_type_data,
        sym_type_code,
        sym_type_extern,
        sym_type_entry,
        sym_type_code_entry,
        sym_type_data_entry
    }sym_type;
    int line_def;
};

/**
 * @brief 
 * @param symbol_name name of the extern
 * @param addresses array of unsigned shorts representing addresses that calls this symbol.
 */
struct extern_call {
    char symbol_name[32 + 1];
    gda  addresses;
};

/**
 * @brief contains a translation of the as file.
 * @param symbol_table an array of struct symbol.
 * @param bmc_code array of unsigned shorts for code section in memory.
 * @param bmc_data array of unsigned shorts for data section in memory.
 * @param extern_usage array of struct extern_call.
 */
struct translation_unit {
    gda symbol_table;
    gda bmc_code;
    gda bmc_data;
    gda extern_usage;
};





#endif