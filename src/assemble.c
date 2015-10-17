#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "structs.h"
#define letter_a 'a'
#define letter_z 'z'
#define letter_A 'A'
#define letter_Z 'Z'
#define SPACE " "
#define SPACE_COMMA " ,"

uint16_t program_counter;
uint16_t memory_end;
uint32_t *ldr_const_array;
int index_of_consts;
addr_label_list_t *addr_label; 

shift_code_t shift_search[] = {
    {"lsl", 0},
    {"lsr", 1},
    {"asr", 2},
    {"ror", 3}
};

uint32_t mask_maker(uint32_t num) {
    // Creates a 32bit sequence with num 1s used as a mask
    return (1 << num) - 1;
}

uint32_t extract_bits(uint32_t instr, int start_bit, int num_bits) {
    // Used to take num_bits from the instr argument beginning at start_bit
    int num_to_shift = start_bit - num_bits + 1;
    instr >>= num_to_shift;
    return instr & (mask_maker(num_bits));
}

uint32_t make_imm_uint32(op_imm_t imm) {
    uint32_t rot = imm.rotate;
    rot <<= 8;
    rot += imm.imm_val;
    return rot;
}

uint32_t make_reg_uint32(op_reg_t reg) {
    uint32_t value = reg.value;
    value <<= 7;
    uint32_t shift_type = reg.shift_type;
    shift_type <<= 5;
    uint32_t opt = reg.opt;
    opt <<= 4;
    return (((value | shift_type) | opt) | reg.reg_m);
}

uint8_t look_up(char *shift_type) {
    int i = 0;
    while (strcmp(shift_type, shift_search[i].shift_type) != 0) {
        i++;
    }
    return shift_search[i].shift_code;
}

bool is_empty(char *s) {
    // This allows a form of comments in our assembly which only works
    // when a line begins with //
    if (strlen(s) >= 2) {
        if (s[0] == '/' && s[1] == '/') {
            return true;
        }
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ' && s[i] != '\t') {
            return false;
        }
    }
    return true;
}

bool is_label(char *string){
    return ((string[0] >= letter_a && string[0] <= letter_z)
        || (string[0] >= letter_A && string[0] <= letter_Z))
        && (string[strlen(string) - 1] == ':');
}

void print_addr_label(addr_label_t al) {
    printf("addr: %i\n", al.addr);
    printf("label: %s\n", al.label);
}

void print_addr_label_list(void) {
    for (int i = 0; i < addr_label->index; i++) {
        print_addr_label(addr_label->array[i]);
    }
}

void first_pass(char *file_in) {
    FILE *fp = fopen(file_in, "r");
    char info[512];
    while (!feof(fp)) {
        fscanf(fp, "%511[^\n]\n", info);
        if (!is_empty(info)) {
            program_counter += 4;
            if (is_label(info)) {
                printf("info: %s - PC: %i\n", info, program_counter - 4);
                program_counter -= 4;
                strtok(info, ":");
                char *copy = (char *) calloc(512, sizeof(char));
    		    alloc_check(copy, "first_pass");
                strcpy(copy, info);
                insert_addr_label(addr_label, copy, program_counter);
            }
        }
    }
    print_addr_label_list();
    memory_end = program_counter;
    ldr_const_array = (uint32_t *) calloc(memory_end, sizeof(uint32_t));    
    alloc_check(ldr_const_array, "first_pass");
    program_counter = 0;
    fclose(fp);
}

instr_t *data_proc_gen(data_proc_t *bin_instr) {
    bin_instr->cond = 0xe;
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    alloc_check(instr, "data_proc_gen");
    instr->data_proc = *bin_instr;
    free(bin_instr);
    return instr;
}

void set_op_2(data_proc_t *bin_instr, char *operand_2) {
    uint32_t imm_val;
    char *reg_m;
    printf("ops: %s\n", operand_2);
    switch (operand_2[0]) {
        case '#':
        case '=':
        if (operand_2[2] == 'x') {
            imm_val = (uint32_t) strtol(operand_2 + 1, NULL, 0);
        } else {
            imm_val = (uint32_t) atoi(operand_2 + 1);
        }
        op_imm_t immediate;
        if (extract_bits(imm_val, 31, 25) > 0) {
            uint32_t bit_0 = extract_bits(imm_val, 0, 1);
            int rot = 0;
            while (!bit_0) {
                imm_val >>= 1;
                rot++;
                bit_0 = extract_bits(imm_val, 0, 1);
            }
            rot = 32 - rot;
            if (rot % 2 != 0) {
                rot >>= 1;
                imm_val <<= 1;
                rot++;
            } else {
                rot >>= 1;
            }
            immediate.rotate = rot;
        } else {
            immediate.rotate = 0;
        }
        immediate.imm_val = imm_val;
        bin_instr->imm = 1;
        bin_instr->op_2 = make_imm_uint32(immediate);
        break;

        case 'r':
        reg_m = strtok(operand_2, SPACE_COMMA);
        printf("reg_m: %s\n", reg_m);
        char *shft_check = strtok(NULL, SPACE_COMMA);
        if (shft_check != NULL) {
            printf("shft_type: %s\n", shft_check);
        }
        op_reg_t reg;
        reg.reg_m = (uint32_t) atoi(reg_m + 1);
        if (shft_check != NULL) {
            char *shft_code = strtok(NULL, SPACE_COMMA);
            printf("shft_code: %s\n", shft_code);
            reg.shift_type = look_up(shft_check);
            uint32_t value;
            switch (shft_code[0]) {
                case '#':
                if (shft_code[2] == 'x') {
                    value = (uint32_t) strtol(shft_code + 1, NULL, 0);
                } else {
                    value = (uint32_t) atoi(shft_code + 1);
                }
                reg.value = value;
                reg.opt = 0;
                break;

                case 'r':
                // CHECK THIS AGAIN
                reg.value = ((uint32_t) atoi(shft_code + 1)) << 1;
                reg.opt = 1;
                break;
            }
        } else {
            reg.value = 0;
            reg.shift_type = 0;
            reg.opt = 0;
        }
        bin_instr->imm = 0;
        bin_instr->op_2 = make_reg_uint32(reg);
        break;
        //remember a default
    }
}

instr_t *data_proc_compute_gen(data_proc_t *bin_instr, char *operands) {
    printf("ops: %s\n", operands);
    char *reg_d = strtok(operands, SPACE_COMMA);
    printf("reg d: %s\n", reg_d);
    char *reg_n = strtok(NULL, SPACE_COMMA);
    printf("reg n: %s\n", reg_n);
    char *operand_2 = strtok(NULL, SPACE_COMMA);
    printf("op_2: %s\n", operand_2);
    bin_instr->reg_d = (uint32_t) atoi(reg_d + 1);
    bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);
    bin_instr->set = 0;
    char *extra_ops = (char *) calloc(sizeof(char), 512);
    alloc_check(extra_ops, "data_proc_compute_gen");
    strcpy(extra_ops, operand_2);
    char *shft_op = strtok(NULL, SPACE_COMMA);
    if (shft_op != NULL) {
        char *last_op = strtok(NULL, SPACE_COMMA);
        strcat(extra_ops, " ");
        strcat(extra_ops, shft_op);
        strcat(extra_ops, " ");
        strcat(extra_ops, last_op);
    }
    set_op_2(bin_instr, extra_ops);
    free(extra_ops);
    return data_proc_gen(bin_instr);
}

instr_t *add(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "add");
    bin_instr->opcode = 0x4;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *sub(char *operands) { 
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "sub");
    bin_instr->opcode = 0x2;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *rsb(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "rsb");
    bin_instr->opcode = 0x3;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *and(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "and");
    bin_instr->opcode = 0x0;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *eor(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "eor");
    bin_instr->opcode = 0x1;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *orr(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "orr");
    bin_instr->opcode = 0xc;
    return data_proc_compute_gen(bin_instr, operands);
}

instr_t *mov(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "mov");
    bin_instr->opcode = 0xd;
    char *reg_d = strtok(operands, SPACE_COMMA);
    bin_instr->reg_d = (uint32_t) atoi(reg_d + 1);
    printf("reg_d: %s\n", reg_d);
    bin_instr->reg_n = 0;
    bin_instr->set = 0;
    char *operand_2 = strtok(NULL, SPACE_COMMA);
    printf("op_2: %s\n", operand_2);
    char *extra_ops = (char *) calloc(sizeof(char), 512);
    alloc_check(extra_ops, "mov");
    strcpy(extra_ops, operand_2);
    char *shft_op = strtok(NULL, SPACE_COMMA);
    if (shft_op != NULL) {
        char *last_op = strtok(NULL, SPACE_COMMA);
        strcat(extra_ops, " ");
        strcat(extra_ops, shft_op);
        strcat(extra_ops, " ");
        strcat(extra_ops, last_op);
    }
    printf("xtra: %s\n", extra_ops);
    set_op_2(bin_instr, extra_ops);
    free(extra_ops);
    return data_proc_gen(bin_instr);
}

instr_t *tst(char *operands) { 
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "tst");
    bin_instr->opcode = 0x8;
    char *reg_n = strtok(operands, SPACE_COMMA);
    bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);
    bin_instr->reg_d = 0;
    bin_instr->set = 1;
    char *operand_2 = strtok(NULL, SPACE_COMMA);//check
    set_op_2(bin_instr, operand_2);
    return data_proc_gen(bin_instr);
}

instr_t *teq(char *operands) { 
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "teq");
    bin_instr->opcode = 0x9;
    char *reg_n = strtok(operands, SPACE_COMMA);
    bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);
    bin_instr->reg_d = 0;
    bin_instr->set = 1;
    char *operand_2 = strtok(NULL, SPACE_COMMA);//check
    set_op_2(bin_instr, operand_2);
    return data_proc_gen(bin_instr);
}

instr_t *cmp(char *operands) { 
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "cmp");
    bin_instr->opcode = 0xa;
    char *reg_n = strtok(operands, SPACE_COMMA);
    bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);
    bin_instr->reg_d = 0;
    bin_instr->set = 1;
    char *operand_2 = strtok(NULL, SPACE_COMMA);//check
    set_op_2(bin_instr, operand_2);
    return data_proc_gen(bin_instr);
}

instr_t *andeq(char *operands) {
    data_proc_t *bin_instr = (data_proc_t *) malloc(sizeof(data_proc_t)); 
    alloc_check(bin_instr, "andeq");
    bin_instr->cond = 0x0;
    bin_instr->imm = 0;
    bin_instr->opcode = 0x0;
    bin_instr->set = 0;
    bin_instr->reg_n = 0;
    bin_instr->reg_d = 0;
    bin_instr->op_2 = 0;
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    alloc_check(instr, "andeq");
    instr->data_proc = *bin_instr;
    free(bin_instr);
    return instr;
}

instr_t *lsl(char *operands) {
    char *reg_n = strtok(operands, SPACE_COMMA);
    char *saved_ops = strtok(NULL, SPACE_COMMA);
    printf("again: %s\n", saved_ops);
    char *new_ops = (char *) calloc(sizeof(char), 512);
    alloc_check(new_ops, "lsl");
    strcat(new_ops, reg_n);
    strcat(new_ops, ", ");
    strcat(new_ops, reg_n);
    strcat(new_ops, ", ");
    strcat(new_ops, "lsl");
    strcat(new_ops, " ");
    strcat(new_ops, saved_ops);
    printf("new ops for mov: %s\n", new_ops);
    instr_t *instr = mov(new_ops); //check
    free(new_ops);
    return instr;
}

instr_t *mul_gen(char *operands, mul_t *bin_instr) {
    bin_instr->cond = 0xe;
    bin_instr->bit_pattern_1001 = 9;
    char *reg_d = strtok(operands, SPACE_COMMA);
    char *reg_m = strtok(NULL, SPACE_COMMA);
    char *reg_s = strtok(NULL, SPACE_COMMA);
    if (bin_instr->acc_set == 2) {
        char *reg_n = strtok(NULL, SPACE_COMMA);
        bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);
    } else {
        bin_instr->reg_n = 0;
    }
    bin_instr->reg_d = (uint32_t) atoi(reg_d + 1);
    bin_instr->reg_m = (uint32_t) atoi(reg_m + 1);
    bin_instr->reg_s = (uint32_t) atoi(reg_s + 1);
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    alloc_check(instr, "mul_gen");
    instr->mul = *bin_instr;
    free(bin_instr);
    return instr;
}

instr_t *mul(char *operands) {    
    mul_t *bin_instr = (mul_t *) malloc(sizeof(mul_t));
    alloc_check(bin_instr, "mul");
    bin_instr->acc_set = 0;
    return mul_gen(operands, bin_instr);
}

instr_t *mla(char *operands) {
    mul_t *bin_instr = (mul_t *) malloc(sizeof(mul_t));
    alloc_check(bin_instr, "mla");
    bin_instr->acc_set = 2;
    return mul_gen(operands, bin_instr);
}

char *remove_leading(char *str, const char *garbage) {
    while (strchr(garbage, *str)) {
        str++;
    }
    return str;
}

instr_t *sdt_gen(single_data_t *bin_instr, char *operands) {
    char *saved_operands = (char *) malloc(strlen(operands) * sizeof(char));
    alloc_check(saved_operands, "sdt_gen");
    strcpy(saved_operands, operands);
    char *reg_d = strtok(operands, SPACE_COMMA);
        printf("reg_d: %s\n", reg_d);
    char *address = strtok(NULL, "");
        printf("addr: %s\n", address);
    address = remove_leading(address, SPACE_COMMA);
        printf("addr post: %s\n", address);
    char *second_arg;
    uint32_t offset_val;
    char *reg_n;
    bin_instr->cond = 0xe;
    bin_instr->reg_d = (uint32_t) atoi(reg_d + 1);
    switch (address[0]) {
        case '=': 
        if (strlen(address) - 1 > 2) {
            if (address[2] == 'x') {
                char *val = strtok(address, "=");
                offset_val = (uint32_t) strtol(val, NULL, 0);
            } else {
                offset_val = (uint32_t) atoi(address + 1);
            }
        } else {
            offset_val = (uint32_t) atoi(address + 1);
        }
        if (bin_instr->load_store == 1 && offset_val <= 0xFF) {
            return mov(saved_operands);
        } else {
            ldr_const_array[index_of_consts] = offset_val;
            index_of_consts++;
            offset_val = memory_end - program_counter - 8;
            memory_end += 4;
            bin_instr->reg_n = 0xf;
        }
        bin_instr->pre_post = 1;
        bin_instr->imm = 2;
        bin_instr->up = 1;
        break;
        
        case '[':
        if (address[strlen(address) - 1] == ']') {
            bin_instr->pre_post = 1;
        } else {
            bin_instr->pre_post = 0;
        }
        reg_n = strtok(address, " ,[]");
        printf("reg_n: %s\n", reg_n);
        second_arg = strtok(NULL, " ,[]");
        printf("2nd arg: %s\n", second_arg);
        if (second_arg == NULL) {
            offset_val = 0;
            bin_instr->imm = 2;
            bin_instr->up = 1;
        } else if (second_arg[0] == '#') {
            if (second_arg[1] == '-') {
                printf("%s\n", second_arg);
                if (second_arg[3] == 'x') {
                    offset_val = (uint32_t) strtol(second_arg + 2, NULL, 0);
                } else {
                    offset_val = (uint32_t) atoi(second_arg + 2);
                }
                bin_instr->up = 0;
            } else {
                if (second_arg[2] == 'x') {
                    offset_val = (uint32_t) strtol(second_arg + 1, NULL, 0);
                } else {
                    offset_val = (uint32_t) atoi(second_arg + 1);
                }
                bin_instr->up = 1;
            }
            bin_instr->imm = 2;
        } else {// second_arg[0] == 'r' || '-'
            if (second_arg[0] == '-') {
                bin_instr->up = 0;
            } else {
                bin_instr->up = 1;
            }
            char *shft_check = strtok(NULL, SPACE);
            if (shft_check != NULL) {
                uint16_t shft_code = look_up(shft_check) << 5;
                char *shft_arg = strtok(NULL, " ,[]");
                uint16_t base_reg = (uint16_t) atoi(second_arg + 1);
                if (shft_arg[0] == '#') {
                    uint16_t shft_num = (uint16_t) atoi(shft_arg + 1) << 7;
                    offset_val = (shft_num | shft_code) | base_reg;
                } else if (shft_arg[0] == 'r') {
                    uint16_t shft_reg = (uint16_t) atoi(shft_arg + 1) << 8;
                    offset_val = (shft_reg | shft_code) | base_reg;
                } else {
                    fprintf(stderr, "ERROR: Wrong args for sdt here\n");
                }
            } else {
                offset_val = (uint32_t) atoi(second_arg + 1);
            }
            bin_instr->imm = 3;
        }
        bin_instr->reg_n = (uint32_t) atoi(reg_n + 1);  
        break;
        
        default:
        break;    
    }
    bin_instr->offset = offset_val;
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    alloc_check(instr, "sdt_gen");
    instr->single_data = *bin_instr;
    free(bin_instr);
    free(saved_operands);
    return instr;
}

instr_t *ldr(char *operands) {
    single_data_t *bin_instr = (single_data_t *) malloc(sizeof(single_data_t));
    alloc_check(bin_instr, "ldr");
    bin_instr->load_store = 1;
    return sdt_gen(bin_instr, operands);
}

instr_t *str(char *operands) {
    single_data_t *bin_instr = (single_data_t *) malloc(sizeof(single_data_t));
    alloc_check(bin_instr, "str");
    bin_instr->load_store = 0;
    return sdt_gen(bin_instr, operands);
}

instr_t *stack_gen(single_data_t *bin_instr) {
    bin_instr->cond = 0xe;
    bin_instr->imm = 2;
    bin_instr->reg_n = 0xd; // 0xd = SP (Stack pointer)
    bin_instr->offset = 4;
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    instr->single_data = *bin_instr;
    free(bin_instr);
    return instr;
}

instr_t *push(char *operands) {
    single_data_t *bin_instr = (single_data_t *) malloc(sizeof(single_data_t));
    bin_instr->up = 0;
    bin_instr->pre_post = 1;
    bin_instr->load_store = 2;
    bin_instr->reg_d = (uint32_t) atoi(operands + 1);
    return stack_gen(bin_instr);
}

instr_t *pop(char *operands) {
    single_data_t *bin_instr = (single_data_t *) malloc(sizeof(single_data_t));
    bin_instr->up = 1;
    bin_instr->pre_post = 0;
    bin_instr->load_store = 1;
    bin_instr->reg_d = (uint32_t) atoi(operands + 1);
    return stack_gen(bin_instr);
}

instr_t *branch_gen(char *operand, branch_t *bin_instr) {
    printf("label: %s\n", operand);
    bin_instr->bit_pattern_1010 = 0xa;
    uint32_t addr = look_up_addr_label(addr_label, operand);
    printf("addr:    %i\n", addr);
    
    int offset = addr - program_counter - 8;
    printf("offset: %i, addr: %i, PC: %i\n", offset, addr, program_counter);
    offset >>= 2;
    bin_instr->offset = offset;
    
    instr_t *instr = (instr_t *) malloc(sizeof(instr_t));
    alloc_check(instr, "branch_gen");
    instr->branch = *bin_instr;
    free(bin_instr);
    return instr;
}

instr_t *beq(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "beq");
    bin_instr->cond = 0x0;
    return branch_gen(operands, bin_instr);
}

instr_t *bne(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "bne");
    bin_instr->cond = 0x1;
    return branch_gen(operands, bin_instr);
}

instr_t *bge(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "bge");
    bin_instr->cond = 0xa;
    return branch_gen(operands, bin_instr);
}

instr_t *blt(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "blt");
    bin_instr->cond = 0xb;
    return branch_gen(operands, bin_instr);
}

instr_t *bgt(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "bgt");
    bin_instr->cond = 0xc;
    return branch_gen(operands, bin_instr);
}

instr_t *ble(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "ble");
    bin_instr->cond = 0xd;
    return branch_gen(operands, bin_instr);
}

instr_t *b(char *operands) {
    branch_t *bin_instr = (branch_t *) malloc(sizeof(branch_t));
    alloc_check(bin_instr, "b");
    bin_instr->cond = 0xe;
    return branch_gen(operands, bin_instr);
}

void write_instr_little_end(FILE *fp, instr_t *instr) {
    // uint8_t ms_byte = extract_bits(instr->general_rep, 31, 8);
    // uint8_t second_byte = extract_bits(instr->general_rep, 23, 8);
    // uint8_t third_byte = extract_bits(instr->general_rep, 15, 8);
    // uint8_t ls_byte = extract_bits(instr->general_rep, 7, 8);
    // printf("Whole instr: %08x\n", instr->general_rep);
    // fwrite(&ls_byte, sizeof(uint8_t), 1, fp);
    // printf("LS byte: %02x\n", ls_byte);
    // fwrite(&third_byte, sizeof(uint8_t), 1, fp);
    // printf("Next: %02x\n", third_byte);
    // fwrite(&second_byte, sizeof(uint8_t), 1, fp);
    // printf("Next: %02x\n", second_byte);
    // fwrite(&ms_byte, sizeof(uint8_t), 1, fp);
    // printf("MS byte: %02x\n", ms_byte);
    fwrite(instr, sizeof(uint32_t), 1, fp);
}

void write_ldr_consts(FILE *fp) {
    for (int i = 0; i < index_of_consts; i++) {
        fwrite(ldr_const_array + i, sizeof(uint32_t), 1, fp);
    }
}

instr_t *perform_function(char *mnem, char *ops) {
    mnem_func_t mnem_func[] = {
        {"add", &add},
        {"sub", &sub},
        {"rsb", &rsb},
        {"and", &and},
        {"eor", &eor},
        {"orr", &orr},
        {"mov", &mov},
        {"tst", &tst},
        {"teq", &teq},
        {"cmp", &cmp},

        {"mul", &mul},
        {"mla", &mla},

        {"ldr", &ldr},
        {"str", &str},

        {"beq", &beq},
        {"bne", &bne},
        {"bge", &bge},
        {"blt", &blt},
        {"bgt", &bgt},
        {"ble", &ble},
        {"b", &b},
         
        {"lsl", &lsl},
        {"andeq", &andeq},

        {"push", &push},
        {"pop", &pop}
    };

    int i = 0;
    while (strcmp(mnem, mnem_func[i].mnem) != 0) {
        i++;
    }
    func_t *func = mnem_func[i].func;
    return func(ops);
}

void second_pass(char *file_in, char *file_out) {
    FILE *fp_in = fopen(file_in, "r");
    FILE *fp_out = fopen(file_out, "w");
    char info[512];

    while (!feof(fp_in)) {
        fscanf(fp_in, "%511[^\n]\n", info);
        // Here we have the instr stored in instr and we need to 
        // break it down into the type of instr and instead of printing
        // instr to fp_out we put the binary code for it in.
        // We need a "decode" type func for this.
        if (!is_empty(info)) {
            if (!is_label(info)) {
                char *mnem = strtok(info, SPACE);
                char *ops = strtok(NULL, "");
                printf("Function: %s\n", info);
                instr_t *instr = perform_function(mnem, ops);
                write_instr_little_end(fp_out, instr);
                printf("Instr = %08x\n", instr->general_rep);
                free(instr);
                printf("PC: %i\n", program_counter);
                program_counter += 4;
            }
        }
    }
    write_ldr_consts(fp_out);
    fclose(fp_in);
    fclose(fp_out);
}

int main(int argc, char **argv) {
    addr_label = init_addr_label(10);
    program_counter = 0;
    memory_end = 0;
    index_of_consts = 0;
    if (argc > 2) {
        first_pass(argv[1]);
        second_pass(argv[1], argv[2]);
    } else {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return EXIT_FAILURE;
    }
    free(ldr_const_array);
    destroy_addr_label(addr_label);
    return EXIT_SUCCESS;
}
