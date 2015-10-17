#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "structs.h"

#define BYTES_IN_WORD 4
#define BIT_PATTERN_1001 9
#define NUM_REGISTERS 14
#define MEM_SIZE 65536

void memory_loader(machine_t *m, const char *file_path) {
    // This function opens a given binary file and loads each byte
    // into memory as it comes.
    // PRE: The binary file has instructions in little endian format.
    uint8_t byte;
    FILE *fp = fopen(file_path, "r");
    int i = 0;

    while (fread(&byte, 1,  1, fp)) {
        m->memory[i] = byte;
        i++;
    }
    
    if (!feof(fp)) {
        // We check to see we didn't end our loop without reading
        // the whole file.
        fprintf(stderr,
        "ERROR: The end of file was not reached.\n");
        fprintf(stderr, "Last byte recorded was: %x\n", byte);
    }

    // We close the file at the end.
    fclose(fp);
}

uint32_t get_instr(machine_t *m, uint16_t byte) {
    // Returns the little endian instruction corresponding
    // to a given byte address
    uint32_t instr = m->memory[byte] << 24;
    instr |= (m->memory[byte + 1] << 16);
    instr |= (m->memory[byte + 2] << 8);
    instr |= (m->memory[byte + 3]);
    return instr;
}

uint32_t get_instr_b(machine_t *m, uint16_t byte) {
    // Returns the big endian instruction corresponding
    // to a given byte address
    uint32_t instr = m->memory[byte + 3] << 24;
    instr |= (m->memory[byte + 2] << 16);
    instr |= (m->memory[byte + 1] << 8);
    instr |= (m->memory[byte]);
    return instr;
}

uint32_t mask_maker(uint32_t num) {
    // Creates a 32 bit sequence with num 1s used as a mask
    return (1 << num) - 1;
}

void print_bits(uint32_t bit_code) {
    // Prints out the bits of a given 32 bit number
    uint32_t mask = 1 << 31;

    for (int i = 0; i < 32; i++) {
        // Goes through each bit by moving the bit code along
        // and bitwise anding with it so that if it is 1 we print 1
        // and if 0 then 0 is printed as required.
        printf("%i", (bit_code & mask) != 0);
        bit_code <<= 1;
    }
    
    printf("\n");
}

uint32_t extract_bits(uint32_t instr, int start_bit, int num_bits) {
    // Used to take num_bits from the instr argument beginning at start_bit
    int num_to_shift = start_bit - num_bits + 1;
    instr >>= num_to_shift;
    return instr & (mask_maker(num_bits));
}

int signed_extension(uint16_t num_to_extend_by, uint32_t instr_to_extend) {
    // Takes an instruction which needs to be sign extended and returns
    // the sign extended version by introducing the 1s if the most
    // significant bit is 1 or 0s if it is 0 to. MSbit is given by arg 0.
    uint32_t start_bit = 32 - num_to_extend_by;
    uint32_t mask = mask_maker(num_to_extend_by) << start_bit;
    uint32_t extracted_bit = extract_bits(instr_to_extend, start_bit, 1);

    if (extracted_bit == 1) {
        instr_to_extend |= mask;
    }

    return instr_to_extend;
}

uint32_t branch(machine_t *m, uint32_t instr) {
    // Executes a branch instruction.
    uint32_t shifted_instr = instr << 2;
    // This multiplies the given instr by 4 so that it is a valid word
    // address
    int offset = signed_extension(6, extract_bits(shifted_instr, 25, 26));
    // We collect the offset from the instr and sign it 
    m->program_counter += offset;
    m->instr_fetched = -1;
    m->instr_decoded = EMPTY;
    m->instr_to_execute = -1;
    // The pipeline is reset and we simply return the instr we executed
    return instr;
}

uint32_t multiply(machine_t *m, uint32_t instr) {
    // This executes the multiply instruction.
    int acc_set = extract_bits(instr, 21, 2);
    // This tells us whether we will add a register value to the
    // answer or not. And if it is odd (1 or 3) we set the CPSR.
    int r_dest = extract_bits(instr, 19, 4);
    // The register the answer is stored at.
    int reg_n = extract_bits(instr, 15, 4);
    // The register we may or may not add to the answer.
    int reg_s = extract_bits(instr, 11, 4);
    // One of the registers we want to multiply.
    int reg_m = extract_bits(instr, 3, 4);
    // The other
    
    int64_t val_at_reg_m = m->reg[reg_m];
    int32_t val_at_reg_s = m->reg[reg_s];
    // We grab the two values.

    val_at_reg_m *= val_at_reg_s;
    // Multiply them.
    
    if (acc_set > 1) {
        // We add the value at reg_n to result if acc is set.
        val_at_reg_m += m->reg[reg_n];
    }
    
    uint32_t trunc_val_reg_m = val_at_reg_m;
    // We truncate the value to 32 bits so it can fit in a register.
    m->reg[r_dest] = trunc_val_reg_m;

    if (acc_set % 2 == 1) {
        // We then set the CPSR in this case.
        uint8_t bit_31 = extract_bits(trunc_val_reg_m, 31 ,1);
        m->N = bit_31;
        // N is set is the same as bit_31 as this tells us if we're
        // negative or not.
        m->Z = (val_at_reg_m == 0);
        // Zero flag is set if the answer is 0.
    }
    // As with all execute methods - we return the instr we executed.
    return instr;
}

uint32_t combine_CPSR(machine_t *m) {
    // This method returns the value of the CPSR which is the four
    // bit values in the machine. It combines them and return the
    // value as the four least significant bits of a 32 bit unsigned int
    uint32_t cpsr = 0;
    uint8_t N = m->N;
    assert(N == 1 || N == 0);
    N <<= 3;
    cpsr |= N;
    
    uint8_t Z = m->Z;
    assert(Z == 1 || Z == 0);
    Z <<= 2;
    cpsr |= Z;

    uint8_t C = m->C;
    assert(C == 1 || C == 0);
    C <<= 1;
    cpsr |= C;
    
    uint8_t V = m->V;
    assert(V == 1 || V == 0);
    cpsr |= V;

    return cpsr;
}

bool cond_check(machine_t *m, uint32_t instr) {
    // This function checks to see whether an instruction should be
    // executed given (true) that it will have 4 condition bits as the most
    // significant bits.
    uint32_t check = extract_bits(instr, 31, 4);
    // We extract the four cond bits.

    switch (check) {
        case 0:
        // If our cond is zero then the we will only return true if the
        // zero bit of the CPSR is set.
        return m->Z;

        case 1:
        // The opposite of case 1.
        return m->Z == 0;
        
        case 10:
        // This will only be true if the negative and overflow CPSR bits
        // are equal.
        return m->N == m->V;
        
        case 11:
        // The oppsite of case 10.
        return m->N != m->V;
        
        case 12:
        // A combination of case 1 and 10 both being true then this instr
        // should execute
        return m->Z == 0 && m->N == m->V;
        
        case 13:
        // If case 0 or case 11 is true then we should execute instr
        return m->Z || m->N != m-> V;
        
        case 14:
        // We should always execute these instr
        return true;
        
        default:
        // We should never execute instr which don't have one of the above
        // cond patterns.
        return false;
    }
}

uint32_t and(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // and returns the value at reg_index bitwise anded with the operand
    // and sets the bits if s is set.
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = m->reg[reg_index] & operand;
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
    }
    return value;
}

uint32_t eor(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // eor returns the value at reg_index bitwise eored with the operand
    // and sets the bits if s is set.
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = m->reg[reg_index] ^ operand;
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
    }
    return value;
}

uint32_t sub(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // sub returns the value at reg_index - the operand in that order
    // and sets the bits if s is set.
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = m->reg[reg_index] - operand;
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
        m->C = m->reg[reg_index] >= operand;
    }
    return value;
}

uint32_t rsb(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // rsb returns the the operand - the value at the reg index so it is
    // essentially the inverse of sub (reverse sub) and sets the bits
    // if s is set.
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = operand - m->reg[reg_index];
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
        m->C = operand >= m->reg[reg_index];
    }
    return value;
}

uint32_t add(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // add returns the value at reg_index bitwise added to the operand
    // and sets the bits if s is set.
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = m->reg[reg_index] + operand;
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
        m->C = value > mask_maker(32);
    }
    return value;
}

uint32_t orr(machine_t *m, uint8_t reg_index, uint32_t operand, uint32_t s) {
    // and returns the value at reg_index bitwise anded with the operand
    // and sets the bits if s is set;
    assert (reg_index < NUM_REGISTERS && reg_index >= 0);
    uint32_t value = m->reg[reg_index] | operand;
    if (s) {
        m->N = extract_bits(value, 31, 1);
        m->Z = (value == 0);
    }
    return value;
}

uint32_t rotate_right(uint32_t operand, uint8_t rotate) {
    // Returns the operand rotated rotate many times where rotate is
    // a number less than or equal to 32. The leas sig bits become the most
    // sig as we move each bit along to the right.
    uint32_t bits_to_keep = 
                extract_bits(operand, rotate - 1, rotate); 
    operand >>= rotate;
    return operand | (bits_to_keep << (32 - rotate));
}

uint32_t arith_right(uint32_t operand, uint8_t shift_num) {
    // Returns the operand arithmetically shifted right shift_num times
    // meaning we retain the most significant bit 31 at each place we
    // go along.
    uint32_t bit_31 = extract_bits(operand, 31, 1);
    uint32_t shifted_op;
    if (bit_31) {
        uint32_t mask = mask_maker(shift_num) << (32 - shift_num);
        shifted_op = ((operand >> shift_num) | mask);
    } else {
        shifted_op = operand >> shift_num;
    }
    return shifted_op;
}

uint32_t data_proc(machine_t *m, uint32_t instr) {
    // Executes a data processing instruction by correctly calling the
    // appropriate function above.
    uint32_t i = extract_bits(instr, 25, 1);
    // If i is set we are looking at an immediate value for operand 2
    // otherwise a register. All data_proc instr will take two args and store
    // in a third register the first is always a register, the second depends
    // upon this i bit.
    uint32_t s = extract_bits(instr, 20, 1);
    // If s is set then we set the CPSR in this instr.
    uint32_t operand_2 = extract_bits(instr, 7, 8);
    // We assume immediate which is the final 8 bits of instr,
    // this changes if i not set.
    uint32_t carry = 0;
    // Needed for setting the carry bit in CPSR.
 
    if (i) {
        // If immediate then we need to also look at what we are rotating
        // the immediate by which is specified by 4 rotate bits.
        uint8_t rotate = extract_bits(instr, 11, 4);
        operand_2 = rotate_right(operand_2, rotate << 1);
    } else {
        // Otherwise we get the reg_index of op2
        uint32_t shift_num = extract_bits(instr, 11, 5);
        uint32_t reg_index = extract_bits(instr, 3, 4);
        uint32_t shift_code = extract_bits(instr, 6, 2);
        uint32_t value_4 = extract_bits(instr, 4, 1);
        // If bit 4 is set then we are looking to shift operand 2 by a
        // value specified in a register, else we are looking at shifting
        // by shift_num. The type of shift is specified by the code.       

        if (value_4) {
            uint32_t reg_s_index = extract_bits(instr, 11, 4);
            shift_num = m->reg[reg_s_index] & mask_maker(8);
        }
        
        switch (shift_code) {
            // This shifts operand 2 appropriately and records any carry values.
            case 0:
            operand_2 = m->reg[reg_index] << shift_num;
            carry = extract_bits(operand_2, 32 - (shift_num + 1), 1);
            break;

            case 1:
            operand_2 = m->reg[reg_index] >> shift_num;
            carry = extract_bits(operand_2, shift_num - 1, 1);
            break;

            case 2:
            operand_2 = arith_right(m->reg[reg_index], shift_num);
            carry = extract_bits(operand_2, shift_num - 1, 1);
            break;

            case 3:
            operand_2 = rotate_right(m->reg[reg_index], shift_num);
            carry = extract_bits(operand_2, shift_num - 1, 1);
            break;

            default:
            break;
        }

        if (s) {
            // If s then we can set the carry bit in CPSR to what we just got.
            m->C = carry; 
        }
    }

    uint32_t op_code = extract_bits(instr, 24, 4);
    uint8_t r_dest = extract_bits(instr, 15, 4);
    uint8_t reg_n = extract_bits(instr, 19, 4);
        
    switch (op_code) {
        // We then do the correct operation based upon the code
        // stored in dest reg and operand 1 is the value at reg_n
        case 0:
        m->reg[r_dest] = and(m, reg_n, operand_2, s);
        break;

        case 1:
        m->reg[r_dest] = eor(m, reg_n, operand_2, s);
        break;
            
        case 2:
        m->reg[r_dest] = sub(m, reg_n, operand_2, s);
        break;

        case 3:
        m->reg[r_dest] = rsb(m, reg_n, operand_2, s);
        break;
           
        case 4:
        m->reg[r_dest] = add(m, reg_n, operand_2, s);
        break;

        case 8:
        // We don't store any register in the next three cases
        // because these operations are mostly needed to set bits in CPSR.
        and(m, reg_n, operand_2, s);
        break;

        case 9:
        eor(m, reg_n, operand_2, s);
        break;

        case 10:
        sub(m, reg_n, operand_2, s);
        break;

        case 12:
        m->reg[r_dest] = orr(m, reg_n, operand_2, s);
        break;

        case 13:
        m->reg[r_dest] = operand_2;
        break;

        default:
        break;
    }
    // We return the instr we recieved as normal. 
    return instr;
}

void ldr(machine_t *m, uint32_t mem_byte, uint32_t r_dest) {
    // This function takes a memory byte address and loads the value
    // contained there into the dest register. It takes the memory value in
    // big endian order as required.
    if (mem_byte > MEM_SIZE) {
        // Unless we have an out of bounds access.
        switch (mem_byte) {
            case 0x20200000:
            printf("One GPIO pin from 0 to 9 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;

            case 0x20200004:
            printf("One GPIO pin from 10 to 19 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;

            case 0x20200008:
            printf("One GPIO pin from 20 to 29 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;
            
            default:
            printf("Error: Out of bounds memory access at address 0x%08x\n",
            mem_byte);
        }
    } else {
        m->reg[r_dest] = get_instr_b(m, mem_byte);
    }
}

void str(machine_t *m, uint32_t mem_byte, uint32_t r_dest) {
    // This function stores the value in the dest register into a given byte
    // in memory in little endian order as expected by memory.
    if (mem_byte > MEM_SIZE) {
        switch (mem_byte) {
            case 0x20200000:
            printf("One GPIO pin from 0 to 9 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;

            case 0x20200004:
            printf("One GPIO pin from 10 to 19 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;

            case 0x20200008:
            printf("One GPIO pin from 20 to 29 has been accessed\n");
            m->reg[r_dest] = mem_byte;
            break;
            
            case 0x2020001c:
            m->control_gpio[0] = m->reg[r_dest];
            printf("PIN ON\n");
            break;

            case 0x20200028:
            m->control_gpio[1] = m->reg[r_dest];
            printf("PIN OFF\n");
            break;

            default:
            printf("Error: Out of bounds memory access at address 0x%08x\n",
            mem_byte);
        }
    } else {
        uint32_t mask = mask_maker(8);
        m->memory[mem_byte + 3] = m->reg[r_dest] >> 24;
        m->memory[mem_byte + 2] = (m->reg[r_dest] >> 16) & mask;
        m->memory[mem_byte + 1] = (m->reg[r_dest] >> 8) & mask;
        m->memory[mem_byte] = m->reg[r_dest] & mask;
    }
}

uint32_t sdt(machine_t *m, uint32_t instr) {
    // sdt executes a single data transfer instruction
    uint32_t imm_bit = extract_bits(instr, 25, 1);
    // This tells us if we have an immediate val (0) or a shifted 
    // register value (1) in as our offset. 
    uint32_t pre_bit = extract_bits(instr, 24, 1);
    // This tells us if we are offsetting the base reg before memory
    // access (1) or after (0).
    uint32_t up_bit = extract_bits(instr, 23, 1);
    // Tells us to add (1) or subtract (0) the offset
    uint32_t base_reg = extract_bits(instr, 19, 4);
    uint32_t r_dest = extract_bits(instr, 15, 4);
    // This is the dest register for a load and the source register for str.
    uint32_t load_set = extract_bits(instr, 20, 1);
    uint32_t write_back = extract_bits(instr, 21, 1);
    // This bit tells us if we load (1) or store (0).
    uint32_t offset;

    if (imm_bit) {
        uint32_t shift_code = extract_bits(instr, 6, 2);
        uint32_t reg_index = extract_bits(instr, 3, 4);
        // This register has the value we want to shift.
        uint32_t shift_num = extract_bits(instr, 11, 5);
        uint32_t value_4 = extract_bits(instr, 4, 1);
        // the 4th bit tells us if our shift amount is a byte from
        // a register (1) or an immediate value (0).

        if (value_4) {
            uint32_t reg_s_index = extract_bits(instr, 11, 4);
            shift_num = m->reg[reg_s_index] & mask_maker(8);
        }
        
        switch (shift_code) {
            // We collect the offset here given the code.
            case 0:
                offset = m->reg[reg_index] << shift_num;
                break;

            case 1:
                offset = m->reg[reg_index] >> shift_num;
                break;

            case 2:
                offset = arith_right(m->reg[reg_index], shift_num);
                break;

            case 3:
                offset = rotate_right(m->reg[reg_index], shift_num);
                break;

            default:
                offset = 0;
                break;
        }

    } else {
        uint32_t imm = extract_bits(instr, 7, 8);
        uint8_t rotate = extract_bits(instr, 11, 4);
        offset = rotate_right(imm, rotate << 1);
        // We need to rotate right by double the value found in rotate
        // so we shift left by 1. It is 4 bit value stored in 8 bits
        // so this shouldn't be a problem.
    }
    
    if (base_reg == 15) {
        // In this case we are looking at the PC being the base register
        // which would be no different to below if it were not treated
        // differently by the struct.
        if (pre_bit) {
            if (up_bit) {
                if (load_set) {
                    ldr(m, m->program_counter + offset, r_dest);
                } else {
                    str(m, m->program_counter + offset, r_dest);
                }
            } else {
                if (load_set) {
                    ldr(m, m->program_counter - offset, r_dest);
                } else {
                    str(m, m->program_counter - offset, r_dest);
                }
            }
        } else {
            if (up_bit) {
                if (load_set) {
                    ldr(m, m->program_counter, r_dest);
                } else {
                    str(m, m->program_counter, r_dest);
                }
                m->program_counter += offset;
            } else {
                if (load_set) {
                    ldr(m, m->program_counter, r_dest);
                } else {
                    str(m, m->program_counter, r_dest);
                }
                m->program_counter -= offset;
            }
        }
    } else {
        // This is the normal case (base_reg is not the PC)
        if (pre_bit) {
            if (up_bit) {
                if (load_set) {
                    ldr(m, m->reg[base_reg] + offset, r_dest);
                } else {
                    str(m, m->reg[base_reg] + offset, r_dest);
                    if (write_back) {
                        m->reg[base_reg] += offset;
                    }
                }
            } else {
                if (load_set) {
                    ldr(m, m->reg[base_reg] - offset, r_dest);
                } else {
                    str(m, m->reg[base_reg] - offset, r_dest);
                    if (write_back) {
                        m->reg[base_reg] -= offset;
                    }
                }
            }
        } else {
            if (up_bit) {
                if (load_set) {
                    ldr(m, m->reg[base_reg], r_dest);
                } else {
                    str(m, m->reg[base_reg], r_dest);
                }
                m->reg[base_reg] += offset;
            } else {
                if (load_set) {
                    ldr(m, m->reg[base_reg], r_dest);
                } else {
                    str(m, m->reg[base_reg], r_dest);
                }
                m->reg[base_reg] -= offset;
            }
        }   
    }
    // We return the instr we executed as normal.
    return instr;
}

void register_print(machine_t *m) {
    // This prints the value of all the registers in a machine
    // including the PC and CPSR.
    printf("Registers:\n");
    for(int i = 0; i < NUM_REGISTERS; i++) {
        printf("$%-3i: %10i (0x%08x)\n", i, m->reg[i], m->reg[i]);
    }
    printf("PC  : %10i (0x%08x)\n", m->program_counter, m->program_counter);
    uint32_t cpsr = combine_CPSR(m) << 28;
    printf("CPSR: %10i (0x%08x)\n", cpsr, cpsr);
}

void memory_print(machine_t *m) {
    // This prints the value of all non-zero memory locations in the machine.
    int i = 0;
    printf("Non-zero memory:\n");
    while (i != MEM_SIZE) {
        while (get_instr(m, i) != 0) {
            printf("0x%08x: 0x%08x\n", i, get_instr(m, i));
            i += 4;
        }

        i += 4;
    }
}

itype decode(machine_t *m, uint32_t instr) {
    // Decode takes an instruction and returns the type of instruction (itype).
    // It is based upon bit patterns in the instr which are common to the
    // particular instruction type.
    int code = extract_bits(instr, 27, 2);
    itype instr_type;
    if (code == 1) {
        instr_type = SDT;
    } else if (code == 2) {
        instr_type = BRANCH;
    } else {
        int mult_check_1 = extract_bits(instr, 7, 4);
        int mult_check_2 = extract_bits(instr, 27, 6);
        
        if (mult_check_1 == BIT_PATTERN_1001 && mult_check_2 == 0) {
            instr_type = MULT;
        } else {
            instr_type = DATAPROC;
        }
    }
    
    if (instr_type == ERROR) {
        fprintf(stderr, "ERROR: Bad instruction.\n");
    }
    
    return instr_type;
}

uint32_t execute(machine_t *m, itype it, uint32_t instr) {
    // Execute takes in the itype of a given instr and calls the correct
    // instruction before returning the instr it just executed.
    if (cond_check(m, instr)) {
        switch(it) {
             case DATAPROC:
                return data_proc(m, instr);
             case MULT:
                return multiply(m, instr);
             case SDT:
                return sdt(m, instr);
             case BRANCH:
                return branch(m, instr);
             case ERROR:
             case EMPTY:
                return instr;
        }
    }

    return instr;
}

void pipe_line(machine_t *m) {  
    // This function runs the machines code by cycling through the memory
    // fetching instructions, decoding it in the next cycle and executing
    // it in the third iteration.
    m->instr_fetched = get_instr_b(m, m->program_counter);
    m->instr_decoded = EMPTY;
    m->instr_to_execute = -1;
    m->instr_executed = -1;

    while (m->instr_to_execute != 0) {
        // We stop when we are about to execute an all-zero instruction
        // because this signals a halt instruction.
        m->program_counter += 4;
        m->instr_executed = execute(m, m->instr_decoded, m->instr_to_execute);
        m->instr_decoded = decode(m, m->instr_fetched);
        m->instr_to_execute = m->instr_fetched;
        m->instr_fetched = get_instr_b(m, m->program_counter);
    }
   
    m->program_counter += 4;
    // We then add 4 to the program counter so that it catches up.
    m->instr_executed = execute(m, m->instr_decoded, m->instr_to_execute);
    if (m->instr_executed != 0) {
        // We make sure the instr we just executed was empty.
        fprintf(stderr, "ERROR: The instruction wasn't all zero\n");
    }
}

int main(int argc, char *argv[]) {
    // argv should only contain 1 value which should be the file path
    // to a binary file which the program can load and execute.
    machine_t *m = (machine_t *) malloc(sizeof(machine_t));
    m->program_counter = 0;
    m->control_gpio = (uint32_t *) calloc(2, sizeof(uint32_t));
    m->reg = (uint32_t *) calloc(NUM_REGISTERS, sizeof(uint32_t));
    m->memory = (uint8_t *) calloc(MEM_SIZE, sizeof(uint32_t));
    m->reg[0xd] = 0x2000;
    m->N = 0;
    m->Z = 0;
    m->C = 0;
    m->V = 0;
    // Here we set up the machine.
    
    if (argc > 1) {
        memory_loader(m, argv[1]);
    } else {
        fprintf(stderr,
        "ERROR: Not enough args. Please specify a file path.\n");
        return EXIT_FAILURE;
    }
    
    pipe_line(m);
    // With the memory loaded we can act on the instructions by calling this.
    register_print(m);
    // Once finished we print all the registers,
    memory_print(m);
    // And all the non-zero memory locations.
    free(m->control_gpio);
    free(m->reg);
    free(m->memory);
    free(m);
    // And then free the memory that the machine had allocated.
    return EXIT_SUCCESS;
}
