#include "diag/util/byte_buffer.h"

#include <stdexcept>

namespace diag::util {

void ByteBuffer::put_u8(uint8_t v) { data_.push_back(v); }

void ByteBuffer::put_u16(uint16_t v) {
  data_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
  data_.push_back(static_cast<uint8_t>(v & 0xFF));
}

void ByteBuffer::put_u32(uint32_t v) {
  data_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
  data_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
  data_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
  data_.push_back(static_cast<uint8_t>(v & 0xFF));
}

void ByteBuffer::put_bytes(const uint8_t* p, std::size_t n) {
  data_.insert(data_.end(), p, p + n);
}

void ByteBuffer::CheckReadable(std::size_t n) const {
  if (remaining() < n) {
    throw std::out_of_range("ByteBuffer read past end");
  }
}

uint8_t ByteBuffer::get_u8() {
  CheckReadable(1);
  return data_[read_pos_++];
}

uint16_t ByteBuffer::get_u16() {
  CheckReadable(2);
  uint16_t v =
      static_cast<uint16_t>((data_[read_pos_] << 8) | data_[read_pos_ + 1]);
  read_pos_ += 2;
  return v;
}

uint32_t ByteBuffer::get_u32() {
  CheckReadable(4);
  uint32_t v = (static_cast<uint32_t>(data_[read_pos_]) << 24) |
               (static_cast<uint32_t>(data_[read_pos_ + 1]) << 16) |
               (static_cast<uint32_t>(data_[read_pos_ + 2]) << 8) |
               static_cast<uint32_t>(data_[read_pos_ + 3]);
  read_pos_ += 4;
  return v;
}

void ByteBuffer::get_bytes(uint8_t* out, std::size_t n) {
  CheckReadable(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = data_[read_pos_ + i];
  }
  read_pos_ += n;
}

void ByteBuffer::skip(std::size_t n) {
  CheckReadable(n);
  read_pos_ += n;
}

}  // namespace diag::util
