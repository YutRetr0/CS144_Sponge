#include "wrapping_integers.hh"
#include <cstdint>
#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t raw_value = (n % (1ll << 32) + isn.raw_value() ) % (1ll << 32);
    return WrappingInt32{raw_value};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t dis = (n - isn) % (1ul << 32);
    uint64_t ratio = checkpoint / (1ul << 32);
    uint64_t ret = 0;
    uint64_t cur_diff = UINT64_MAX;
    for (int i = -1; i <= 1; i++){
        uint64_t value = (ratio + i)* (1ul << 32) + dis;
        uint64_t diff = value > checkpoint ? value - checkpoint : checkpoint - value;
        if (cur_diff > diff) {
            ret = value;
            cur_diff = diff;
        }
    }
    return ret;
}
