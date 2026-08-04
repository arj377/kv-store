#include "lru_replacer.h"

LRUReplacer::LRUReplacer() = default;

std::optional<frame_id_t> LRUReplacer::evict() {
    if (isEvictable.empty()) {
        return std::nullopt;
    }

    for (auto it = recencyList.begin(); it != recencyList.end(); ++it) {
        // it is the current element in the linked list
        if (isEvictable[*it]) {
            auto temp = *it;
            isEvictable.erase(temp);
            table.erase(temp);
            recencyList.erase(it);
            return temp;
        }
    }
    return std::nullopt;
}

void LRUReplacer::recordAccess(frame_id_t frame) {
    auto it = table.find(frame); // it = (frame, iterator_into_recencyList)
    if (it != table.end()) {
        recencyList.erase(it->second);
        table.erase(it);
    }
    recencyList.emplace_back(frame);
    table[frame] = std::prev(recencyList.end()); // Go one step back from the end
}

void LRUReplacer::setEvictable(frame_id_t frame, bool allowed) {
    isEvictable[frame] = allowed;
}

size_t LRUReplacer::size() {
    size_t counter = 0;
    for (const auto &[key, value] : isEvictable) {
        if (value) {
            counter++;
        }
    }
    return counter;
}

void LRUReplacer::remove(frame_id_t frame) {
    auto it = table.find(frame);

    if (it == table.end()) {
        return;
    }

    recencyList.erase(it->second);
    table.erase(it);
    isEvictable.erase(frame);
}
