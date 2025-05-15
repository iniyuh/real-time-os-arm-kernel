#include "tree.h"

U32 power_two (U32 n) {
    U32 x = 1;

    for (U32 i = 0; i < n; i++) {
        x *= 2;
    }

    return x;
}



U32 find_pos_from_k_and_x (knx data) {
    if (data.k == 0) {
        return 0;
    }

    U32 pos = power_two(data.k);

    return pos + data.x - 1;
}

knx find_k_and_x_from_pos (U32 pos) {
    if (pos == 0) {
        return ((knx){ .k = 0, .x = 0 });
    }

    pos += 1;

    U32 k = 0;
    U32 bound = 2;
    while (bound <= pos) {
        bound *= 2;
        k++;
    }

    U32 x = pos - bound/2;

    return ((knx){ .k = k, .x = x });
}



BOOL get_bit (U8 arr[], knx data) {
    U32 pos = find_pos_from_k_and_x(data);
    return (arr[pos/8] & (1 << pos%8)) > 0;
}

void set_bit_on (U8 arr[], knx data) {
    U32 pos = find_pos_from_k_and_x(data);
    arr[pos/8] |= 1 << pos%8;
}

void set_bit_off (U8 arr[], knx data) {
    U32 pos = find_pos_from_k_and_x(data);
    arr[pos/8] &= ~(1 << pos%8);
}



knx find_buddy (knx data) {
    if (data.k == 0) {
        return ((knx){ .k = 0, .x = 0 });
    }

    U32 pos = power_two(data.k);

    return find_k_and_x_from_pos(pos - 1 + (data.x ^ 1));
}

knx find_parent (knx data) {
    if (data.k == 0) {
        return ((knx){ .k = 0, .x = 0 });
    }

    U32 pos = power_two(data.k - 1);

    U32 floor = data.x / 2;

    return find_k_and_x_from_pos(pos + floor - 1);
}

knx find_left_child (knx data) {
    U32 pos = power_two(data.k + 1);

    return find_k_and_x_from_pos(pos + (2 * data.x) - 1);
}



U32 compute_block_size (knx data, size_t pool_size) {
    return pool_size / power_two(data.k);
}

U32 compute_block_address (knx data, size_t pool_size, U32 start_address) {
    if (data.x == 0) {
        return start_address;
    }

    return start_address + (pool_size / power_two(data.k) * data.x);
}
