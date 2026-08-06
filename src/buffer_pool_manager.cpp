#include "buffer_pool_manager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

BufferPoolManager::BufferPoolManager() : bufferPool(BUFFER_POOL_SIZE) {
    for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
        freeFrameList.emplace_back(i);
    }
}

std::optional<frame_id_t> BufferPoolManager::getFreeFrame() {
    if (!freeFrameList.empty()) {
        frame_id_t frame = freeFrameList.front();
        freeFrameList.erase(freeFrameList.begin());
        return frame;
    } else {
        return evictFrame();
    }
}

std::optional<frame_id_t> BufferPoolManager::evictFrame() {
    auto victim = replacer.evict();
    if (!victim) // No evictable frame
    {
        return std::nullopt;
    }
    frame_id_t frame = victim.value();

    Page &p = bufferPool[frame];
    if (p.isDirty()) {
        flushPage(p.getPageID());
    }
    pageTable.erase(p.getPageID());
    return frame;
}

bool BufferPoolManager::flushPage(page_id_t pageID) {
    if (pageTable.find(pageID) == pageTable.end()) {
        return false;
    }
    frame_id_t frame = pageTable[pageID];
    Page &p = bufferPool[frame];
    if (p.isDirty()) {
        diskManager.writePage(pageID, p);
        p.setDirty(false);
    }
    return true;
}

bool BufferPoolManager::flushAllPages() {
    for (const auto &[pageID, frameID] : pageTable) {
        if (!flushPage(pageID)) {
            return false;
        }
    }
    return true;
}

Page *BufferPoolManager::fetchPage(page_id_t pageID) {
    if (pageTable.find(pageID) != pageTable.end()) {
        frame_id_t frame = pageTable[pageID];
        Page &p = bufferPool[frame];

        p.incrementPinCount();
        replacer.recordAccess(frame);
        replacer.setEvictable(frame, false);

        return &p;
    }

    if (!pageExists(pageID)) {
        return nullptr;
    }

    auto ff = getFreeFrame();
    if (!ff) {
        return nullptr;
    }
    frame_id_t frame = ff.value();
    Page &p = bufferPool[frame];

    diskManager.readPage(pageID, p);

    p.setPageID(pageID);
    p.setDirty(false);
    p.resetPinCount();
    p.incrementPinCount();

    pageTable[pageID] = frame;
    replacer.recordAccess(frame);
    replacer.setEvictable(frame, false);
    return &p;
}

Page *BufferPoolManager::newPage() {
    auto ff = getFreeFrame();
    if (!ff) {
        return nullptr;
    }
    page_id_t pageID = diskManager.allocatePage();
    frame_id_t frame = ff.value();
    Page &p = bufferPool[frame];

    p.setPageID(pageID);
    p.setDirty(false);
    p.resetPinCount();
    p.incrementPinCount();
    p.clearData();

    pageTable[pageID] = frame;
    replacer.recordAccess(frame);
    replacer.setEvictable(frame, false);
    return &p;
}

void BufferPoolManager::unpinPage(page_id_t pageID) {
    if (pageTable.find(pageID) == pageTable.end()) {
        return;
    }
    frame_id_t frame = pageTable[pageID];
    Page &p = bufferPool[frame];
    p.decrementPinCount();
    if (p.getPinCount() == 0) {
        replacer.setEvictable(frame, true);
    }
}

void BufferPoolManager::deletePage(page_id_t pageID) {
    if (pageTable.find(pageID) == pageTable.end()) {
        diskManager.deallocatePage(pageID);
        return;
    }
    frame_id_t frame = pageTable[pageID];
    Page &p = bufferPool[frame];
    if (p.getPinCount() > 0) {
        return;
    }
    pageTable.erase(pageID);
    replacer.remove(frame);

    p.setPageID(INVALID_PAGE_ID);
    p.setDirty(false);
    p.resetPinCount();
    p.clearData();

    freeFrameList.emplace_back(frame);
    diskManager.deallocatePage(pageID);
}

bool BufferPoolManager::pageExists(page_id_t id) const {
    return id >= 0 && id < diskManager.getNextAvailableID();
}

page_id_t BufferPoolManager::getPageCount() const {
    return diskManager.getNextAvailableID();
}
