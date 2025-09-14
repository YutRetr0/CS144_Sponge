#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <iostream>
#include <optional>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn){
        _isn = seg.header().seqno;
        _init = true;
    }
    if (!_init) return;
    if (seg.payload().size() == 0 && seg.header().syn && !seg.header().fin){
        return;
    }
    
    // syn + data
    size_t seq_no = 0;
    if (seg.header().syn && seg.payload().size() != 0){
        seq_no = unwrap(
            seg.header().seqno + 1, _isn,
            _reassembler.get_assembled_idx());
    } else {
        seq_no = unwrap(seg.header().seqno, _isn, _reassembler.get_assembled_idx());
    }
    // syn occupy one index
    _reassembler.push_substring(
        seg.payload().copy(),
        seq_no - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_init) return optional<WrappingInt32>();
    auto ret = wrap(_reassembler.get_assembled_idx() + 1, _isn);
    std::cout << "ret " << ret.raw_value() << endl;
    return ret;
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.get_acceptable_size();
}
