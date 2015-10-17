#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "structs.h"
#include "mnem_func.h"

void alloc_check(void *alloc, const char *func_name) {
    if (alloc == NULL) {
    	fprintf(stderr, "malloc/calloc failure in function: %s\n", func_name);
	    exit(EXIT_FAILURE);
    }
}

addr_label_list_t *init_addr_label(int init_capacity) {
    addr_label_list_t *al = (addr_label_list_t *) 
            malloc(sizeof(addr_label_list_t));
    alloc_check(al, "init_addr_label");
    al->array = (addr_label_t *)
            malloc(init_capacity * sizeof(addr_label_t));
    alloc_check(al->array, "init_addr_label");
    al->capacity = init_capacity;
    al->index = 0;
    return al;
}

uint16_t look_up_addr_label(addr_label_list_t *al, char *label) {
    for (int i = 0; i < al->index; i++) {
        printf("label in array: %s, label to look up: %s\n", al->array[i].label, label);
        if (strcmp(al->array[i].label, label) == 0) {
            return al->array[i].addr;
        }
    }

    return 0;
}

void insert_addr_label(addr_label_list_t *al, char *label, uint16_t addr) {
    if (al->index + 1 >= al->capacity) {
        al->capacity *= 2;
        al->array = (addr_label_t *)
                realloc(al->array, al->capacity * sizeof(addr_label_t));
    }
    
    al->array[al->index].addr = addr;
    al->array[al->index].label = label;
    al->index++;
}

void destroy_addr_label(addr_label_list_t *al) {
    for (int i = 0; i < al->index; i++) {
        free(al->array[i].label);
    }
    free(al->array);
    free(al);
}
