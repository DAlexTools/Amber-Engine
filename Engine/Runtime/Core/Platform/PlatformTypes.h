#ifndef AMBER_PLATFORM_TYPES_H
#define AMBER_PLATFORM_TYPES_H

#include <cstddef>
#include <cstdint>

namespace AE
{

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using ANSICHAR = char;
using WIDECHAR = wchar_t;
using UTF8CHAR = char;
using TCHAR = WIDECHAR;

using UPtrInt = std::uintptr_t;
using PtrInt = std::intptr_t;
using SizeT = std::size_t;
using SSizeT = std::ptrdiff_t;

using UPTRINT = UPtrInt;
using PTRINT = PtrInt;
using SIZE_T = SizeT;
using SSIZE_T = SSizeT;

using TYPE_OF_NULL = int32;
using TYPE_OF_NULLPTR = decltype(nullptr);

static_assert(sizeof(int8) == 1, "int8 must be 1 byte");
static_assert(sizeof(uint8) == 1, "uint8 must be 1 byte");
static_assert(sizeof(int16) == 2, "int16 must be 2 bytes");
static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes");
static_assert(sizeof(int32) == 4, "int32 must be 4 bytes");
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes");
static_assert(sizeof(int64) == 8, "int64 must be 8 bytes");
static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes");
static_assert(sizeof(UPtrInt) == sizeof(void*), "UPtrInt must match pointer size");
static_assert(sizeof(PtrInt) == sizeof(void*), "PtrInt must match pointer size");

} // namespace AE

using AE::int16;
using AE::int32;
using AE::int64;
using AE::int8;
using AE::PtrInt;
using AE::SizeT;
using AE::SSizeT;
using AE::uint16;
using AE::uint32;
using AE::uint64;
using AE::uint8;
using AE::UPtrInt;

#endif
