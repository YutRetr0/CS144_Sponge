#include "stream_reassembler.hh"
#include <cstdint>
#include <iterator>
#include <iostream>
#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _unass_mask(capacity, false), _unass_bytes(capacity, '\0'), unass_size(0), unass_base(0)
{
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // update idx if _output has peekout
    if (eof) {
        _eof = true;
    }
    if (data.size() == 0 && eof && unass_size == 0){
        _output.end_input();
        return;
    }
    if (index >= unass_base + _capacity) {
        return;
    }
    
    if (index >= unass_base){
        int offset = index - unass_base;
        size_t real_len = min(data.size(), _capacity - _output.buffer_size() - offset);
        if (real_len < data.size()){
            _eof = false;
        }
        for (size_t i = 0; i < real_len; i++){
            if (_unass_mask[i + offset]) continue;
            _unass_bytes[i + offset] = data[i];
            _unass_mask[i + offset] = true;
            unass_size++;
        }
    }else if (index + data.size() > unass_base){
        int offset = unass_base - index;
        size_t real_len = min(data.size() - offset, _capacity - _output.buffer_size());
        if (real_len < data.size() - offset){
            _eof = false;
        }
        for (size_t i = 0; i < real_len; i++){
            if (_unass_mask[i]) continue;
            _unass_bytes[i] = data[i + offset];
            _unass_mask[i] = true;
            unass_size++;
        }
    }

    if (_unass_mask[0]){
        string input;
        size_t clear_len = 0;
        for (; clear_len < _capacity; clear_len++){
            if (!_unass_mask[clear_len]){
                break;
            }
            input += _unass_bytes[clear_len];
            unass_size--;
        }
        unass_base += clear_len;
        for (size_t j = 0; j < clear_len; j++){
            _unass_bytes.pop_front();
            _unass_mask.pop_front();
            _unass_mask.push_back(false);
            _unass_bytes.push_back('\0');
        }
        _output.write(input);
    }
    if (_eof && unass_size == 0){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return unass_size;
}

bool StreamReassembler::empty() const {
    return _output.buffer_empty() && unassembled_bytes() == size_t(0);
}
