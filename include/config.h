#pragma once

#include <cstddef>
#include <cstdint>

using page_id_t = int32_t;
using frame_id_t = int32_t;

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t BUFFER_POOL_SIZE = 64;
constexpr page_id_t INVALID_PAGE_ID = -1;