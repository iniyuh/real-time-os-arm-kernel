#include "log.h"

U32 log_floor (U32 v) {
    v |= v >> 1; 
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return MultiplyDeBruijnBitPosition[(U32)(v * 0x07C4ACDDU) >> 27];
}

U32 log_ceil (U32 v) {
    U32 r = log_floor(v);
    
    if ( v - ((U32) 1 << r) != 0 ) {
        return r + 1;
    }

    return r;
}
