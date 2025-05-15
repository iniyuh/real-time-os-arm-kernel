#ifndef TREE_H
#define TREE_H

#include "common.h"

typedef struct k_and_x {
    U32 k;
    U32 x;
} knx;

U32 power_two (U32 n);

U32 find_pos_from_k_and_x (knx data);
knx find_k_and_x_from_pos (U32 pos);

BOOL get_bit (U8 arr[], knx data);
void set_bit_on (U8 arr[], knx data);
void set_bit_off (U8 arr[], knx data);

knx find_buddy (knx data);
knx find_parent (knx data);
knx find_left_child (knx data);

U32 compute_block_size (knx data, size_t pool_size);
U32 compute_block_address (knx data, size_t pool_size, U32 start_address);

#endif
