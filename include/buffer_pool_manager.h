#pragma once

#include <vector>
#include <unordered_map>
#include <optional>

#include "config.h"
#include "page.h"
#include "disk_manager.h"
#include "lru_replacer.h"

class BufferPoolManager
{
public:
    BufferPoolManager();
    Page* fetchPage(page_id_t);
    Page* newPage();
    void unpinPage(page_id_t);
    bool flushPage(page_id_t);
    bool flushAllPages();
    void deletePage(page_id_t);
    bool pageExists(page_id_t id) const;
    page_id_t getPageCount() const;

private:
    std::optional<frame_id_t> getFreeFrame();
    std::optional<frame_id_t> evictFrame();
    std::vector<Page> bufferPool; // Stores the pages in main memory
    DiskManager diskManager;
    LRUReplacer replacer;
    std::unordered_map<page_id_t, frame_id_t> pageTable; // Page ID to Frame conversion
    std::vector<frame_id_t> freeFrameList;
};