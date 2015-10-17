#include <stdio.h>
#include <stdlib.h>

typedef struct machine {
    uint8_t *memory;
    uint32_t *reg;
    uint16_t programCounter;
    uint8_t N;
    uint8_t Z;
    uint8_t C;
    uint8_t V;
} machine_t;

#define bitPattern1001 9

int maskMaker(int num){
    return (1 << num) - 1;
}

uint32_t extractBits(uint32_t instr, int startBit, int numBits) {
    int numToShift = startBit - numBits + 1;
    instr >>= numToShift;
    return instr & (maskMaker(numBits)); 
}

int decode(machine_t *m, uint32_t instr) {
    int code = extractBits(instr, 27, 2);
    //if (condCheck(m, instr)) {
        switch (code) {
            case 1:
            //sdt(m,instr);
            return 1;

            case 2:
            //branch(m,instr);
            return 2;
        }
    
        int multCheck1 = extractBits(instr, 7, 4);
        int multCheck2 = extractBits(instr, 27, 6);
    
        if (multCheck1 == bitPattern1001 && multCheck2 == 0) {
            //multiply(m, instr);
            return 3;
        } else {
            //dataProc(m, instr);
            return 4;
        }
    
        fprintf(stderr, "ERROR: Bad instruction.\n");
        return 5;
    //}
}

int signedExtension(uint16_t numToExtendBy, uint32_t instrToExtend) {
    int startBit = 32 - numToExtendBy;
    int mask = maskMaker(numToExtendBy) << startBit;
    int extractedBit = extractBits(instrToExtend, startBit, 1);
    if (extractedBit == 1) {
        instrToExtend = instrToExtend | mask;
    }
    return instrToExtend;
}

void testExtractBits(void){
    printf("----------------------TESTING EXTRACTBITS------------------------\n");
    uint32_t ret1 = extractBits(100, 3, 3);
    uint32_t ret2 = extractBits(0, 2, 2);
    uint32_t ret3 = extractBits(285253859, 27, 2);
    uint32_t ret4 = extractBits(570458412, 27, 2);
    uint32_t expe1 = 2;
    uint32_t expe2 = 0;
    uint32_t expe3 = 0;
    uint32_t expe4 = 0;
    if(!(ret1 == expe1 && ret2 == expe2 && ret3 == expe3 && ret4 == expe4)){
        printf("ret1 = %i : expected : %i\n", ret1, expe1);
        printf("ret2 = %i : expected : %i\n", ret2, expe2);
        printf("ret3 = %i : expected : %i\n", ret3, expe3);
        printf("ret4 = %i : expected : %i\n", ret4, expe4);
    }
}

void testDecode(machine_t *m){
    printf("-------------------------TESTING DECODE--------------------------\n");
    uint32_t code1 = decode(m, 285253859);
    uint32_t code2 = decode(m, 570458412);
    uint32_t code3 = decode(m, 587235808);
    uint32_t expe1 = 4;
    uint32_t expe2 = 4;
    uint32_t expe3 = 4;
    if(!(code1 == expe1 && code2 == expe2 && code3 == expe3)){
        printf("code1 = %i : expected : %i\n", code1, expe1);
        printf("code2 = %i : expected : %i\n", code2, expe2);
        printf("code3 = %i : expected : %i\n", code3, expe3);
    }
}

void testSignedExtension(){
    printf("--------------------TESTING SIGNEDEXTENSION----------------------\n");
    uint32_t num1 = signedExtension(10, 15);
    uint32_t num2 = signedExtension(3, -13);
    uint32_t num3 = signedExtension(4, 12371239);
    uint32_t num4 = signedExtension(2, -1237911);
    uint32_t expe1 = 15;
    uint32_t expe2 = -13;
    uint32_t expe3 = 12371239;
    uint32_t expe4 = -1237911;
    if(!(num1 == expe1 && num2 == expe2 && num3 == expe3 && num4 == expe4)){
        printf("num1 = %i : expected %i\n", num1, expe1);
        printf("num2 = %i : expected %i\n", num2, expe2);
        printf("num3 = %i : expected %i\n", num3, expe3);
        printf("num4 = %i : expected %i\n", num4, expe4);   
    }
}

int main(void){
	machine_t *m = malloc(sizeof(machine_t));
    m->programCounter = 0;
    m->reg = calloc(13, sizeof(uint32_t));
    m->memory = calloc(65536, sizeof(uint32_t));
    m->N = 0;
    m->Z = 0;
    m->C = 0;
    m->V = 0;
    testExtractBits();
    testDecode(m);
    testSignedExtension();
//    testSDT();
//    testMult();
//    testDataProc();
//    testBranch();
}
