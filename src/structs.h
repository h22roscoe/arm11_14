#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef enum {
    // This is an enum for the type of instruction we are dealing with.
    DATAPROC,
    MULT,
    SDT,
    BRANCH,
    EMPTY,
    ERROR
} itype;

typedef struct {
    uint32_t rotate : 4;
    uint32_t imm_val : 8;
} op_imm_t;

typedef struct {
    uint32_t value : 5;
    uint32_t shift_type : 2;
    uint32_t opt : 1;
    uint32_t reg_m : 4;
} op_reg_t;

typedef struct {
    uint32_t op_2 : 12;
    uint32_t reg_d : 4;
    uint32_t reg_n : 4;
    uint32_t set : 1;
    uint32_t opcode : 4;
    uint32_t imm : 3; 
    //2 bits for code and one bit for the immediate value
    uint32_t cond : 4;
} data_proc_t;

typedef struct {
    uint32_t reg_m : 4;
    uint32_t bit_pattern_1001 : 4;
    // remember to set this to 9;
    uint32_t reg_s : 4;
    uint32_t reg_n : 4;
    uint32_t reg_d : 4; 
    // this comes before the reg_n. BEWARE!
    uint32_t acc_set : 8; 
    //6 bits for all zero and 1 bit each for accumulate and set
    uint32_t cond : 4;
} mul_t;

typedef struct {
    uint32_t offset : 12;
    uint32_t reg_d : 4;
    uint32_t reg_n : 4;
    uint32_t load_store : 3;
    // first to bits are zero. if set load else store.
    uint32_t up : 1;
    // if set we add, else subtract offset
    uint32_t pre_post : 1; 
    // if set then preprocessing
    uint32_t imm : 3;
    //remember to add 2 to the value of imm. if you want an
    // immediate then you need imm to be 010.
    uint32_t cond : 4;
} single_data_t;

typedef struct {
    uint32_t offset : 24;
    uint32_t bit_pattern_1010 : 4;
    //remember to set this to 10.
    uint32_t cond : 4;
} branch_t;

typedef struct {
    // This structure represents our machine and all of its states
    // including our memory and register arrays
    // PC and CPSR (N, Z, C, V are flags used to show negative result,
    // zero result, carry in result and overflow in result of an operation
    // respectively) are treated seperately from general registers.
    // The other variables are the three stages of our pipeline. Which
    // involves having instruction in these three states.
    uint8_t *memory;
    uint32_t *control_gpio;
    uint32_t *reg;
    int16_t program_counter;
    uint8_t N;
    uint8_t Z;
    uint8_t C;
    uint8_t V;
    uint32_t instr_fetched;
    itype instr_decoded;
    uint32_t instr_to_execute;
    uint32_t instr_executed;
} machine_t;

typedef union {
    data_proc_t data_proc;
    mul_t mul;
    single_data_t single_data;
    branch_t branch;
    uint32_t general_rep;
} instr_t;

typedef struct {
    uint16_t addr;
    char *label;
} addr_label_t;

typedef instr_t *(func_t)(char *);

typedef struct {
    char *mnem;
    func_t *func;
} mnem_func_t;

typedef struct {
    char *shift_type;
    uint8_t shift_code;
} shift_code_t;

typedef struct {
    addr_label_t *array;
    int index;
    int capacity;
} addr_label_list_t;

void insert_addr_label(addr_label_list_t *al, char *label, uint16_t addr);

uint16_t look_up_addr_label(addr_label_list_t *al, char *label);

addr_label_list_t *init_addr_label(int init_capacity);

void destroy_addr_label(addr_label_list_t *al);

void alloc_check(void *alloc, const char *func_name);

#endif
