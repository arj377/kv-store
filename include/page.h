#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <optional>

#include "config.h"

struct PageHeader {
    uint16_t recordCount;
    uint16_t freeSpaceOffset;
};

struct RecordInfo {
    size_t offset;
    uint16_t keyLength;
    uint16_t valueLength;
};

class Page {
public:
    Page(page_id_t);
    Page();
    std::array<char, PAGE_SIZE>& getData();

    page_id_t getPageID() const;
    void setPageID(page_id_t);

    bool isDirty() const;
    void setDirty(bool);

    int getPinCount() const;
    void incrementPinCount();
    void resetPinCount();
    void decrementPinCount();

    void clearData();
    bool hasSpace(size_t keySize, size_t valueSize);

    PageHeader* getHeader();

    bool insert(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool deletePair(const std::string& key);
    bool update(const std::string& key, const std::string& value);

private:
    std::optional<RecordInfo> findRecord(const std::string &key);
    page_id_t pageID;
    std::array<char, PAGE_SIZE> data;
    int pinCount;
    bool dirty;
};
