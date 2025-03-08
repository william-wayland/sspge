#pragma once

#include <span>
#include <type_traits>
#include <vector>

using byte = unsigned char;
using bytes = std::vector<byte>;

template <typename T> bytes Serialise(T &&t) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  std::span<byte> s(reinterpret_cast<byte *>(&t), sizeof(T));
  return bytes(s.begin(), s.end());
}

template <typename T> T Deserialise(const bytes &data) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  return *(reinterpret_cast<T *>(data.data()));
}

template <typename T> T Deserialise(std::span<byte> data) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  return *(reinterpret_cast<T *>(data.data()));
}

enum class MessageID {
  Ok = 0,
  Error,
  Version,
  RequestEntityID,
  EntityID,
  Quit
};

struct Version {
  size_t major;
  size_t minor;
  size_t build;
  MessageID id = MessageID::Version;
};
