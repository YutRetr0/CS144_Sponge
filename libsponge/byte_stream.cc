#include "byte_stream.hh"
#include <queue>
#include "string.h"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    this->capacity = capacity;
}

size_t ByteStream::write(const string &data) {
    auto remain_size = remaining_capacity();
    if (remain_size == 0) return 0;
    auto written_len = min(data.size(), remain_size);
    byte_stream.insert(byte_stream.end(), data.begin(), data.begin()+written_len);
    _written_len += written_len;
    return written_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if (len == 0) return "";
    size_t peek_size = byte_stream.size() > len ? len : byte_stream.size();
    return string(byte_stream.begin(), byte_stream.begin() + peek_size);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len == 0) return;
    size_t pop_size = byte_stream.size() > len ? len : byte_stream.size();
    _read_len += pop_size;
    byte_stream.erase(byte_stream.begin(), byte_stream.begin() + pop_size);
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() {
    is_end = true;
}

bool ByteStream::input_ended() const {
    return is_end;
}

size_t ByteStream::buffer_size() const {
    return byte_stream.size();
}

bool ByteStream::buffer_empty() const {
    return byte_stream.empty();
}

bool ByteStream::eof() const {
    return is_end && buffer_empty();
}

size_t ByteStream::bytes_written() const {
    return _written_len;
}

size_t ByteStream::bytes_read() const {
    return _read_len;
}

size_t ByteStream::remaining_capacity() const {
    return capacity - byte_stream.size();
}
