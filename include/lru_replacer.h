#pragma once

#include "config.h"
#include <list>
#include <unordered_map>
#include <optional>
#include <cstddef>

class LRUReplacer {
public:
    LRUReplacer();
    std::optional<frame_id_t> evict();
    void recordAccess(frame_id_t);
    void setEvictable(frame_id_t, bool);
    void remove(frame_id_t frame);

    size_t size();
private:
    std::list<frame_id_t> recencyList;
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> table;
    std::unordered_map<frame_id_t, bool> isEvictable;
};