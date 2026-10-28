#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace diag::util {

class ByteBuffer {
 public:
  ByteBuffer() = default;
  explicit ByteBuffer(std::vector<uint8_t> data) : data_(std::move(data)) {}

  void put_u8(uint8_t v);
  void put_u16(uint16_t v);
  void put_u32(uint32_t v);
  void put_bytes(const uint8_t* p, std::size_t n);

  uint8_t get_u8();
  uint16_t get_u16();
  uint32_t get_u32();
  void get_bytes(uint8_t* out, std::size_t n);

  std::size_t remaining() const noexcept { return data_.size() - read_pos_; }
  std::size_t size() const noexcept { return data_.size(); }
  bool empty() const noexcept { return remaining() == 0; }
  void reset() noexcept { read_pos_ = 0; }
  void skip(std::size_t n);

  const std::vector<uint8_t>& data() const noexcept { return data_; }

 private:
  void CheckReadable(std::size_t n) const;

  std::vector<uint8_t> data_;
  std::size_t read_pos_ = 0;
};

}  // namespace diag::util
