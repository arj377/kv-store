#include "page.h"
#include "config.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

Page::Page(page_id_t id) : pageID(id), dirty(false), pinCount(0) {
    data.fill(0);

    PageHeader *header = reinterpret_cast<PageHeader *>(data.data());
    header->freeSpaceOffset = sizeof(PageHeader);
    header->recordCount = 0;
}

Page::Page() : Page(INVALID_PAGE_ID) {
}

std::array<char, PAGE_SIZE> &Page::getData() {
    return data;
}

page_id_t Page::getPageID() const {
    return pageID;
}

void Page::setPageID(page_id_t id) {
    pageID = id;
}

bool Page::isDirty() const {
    return dirty;
}

void Page::setDirty(bool val) {
    dirty = val;
}

int Page::getPinCount() const {
    return pinCount;
}

void Page::incrementPinCount() {
    pinCount++;
}

void Page::resetPinCount() {
    pinCount = 0;
}

void Page::decrementPinCount() {
    if (pinCount > 0) {
        pinCount--;
    }
}

void Page::clearData() {
    data.fill(0);
    PageHeader *header = getHeader();
    header->recordCount = 0;
    header->freeSpaceOffset = sizeof(PageHeader);
}

PageHeader* Page::getHeader() {
    return reinterpret_cast<PageHeader *>(data.data());
}

bool Page::insert(const std::string &key, const std::string &value) {
    PageHeader *header = getHeader();
    size_t offset = header->freeSpaceOffset;
    if (PAGE_SIZE - offset < (2 * sizeof(uint16_t)) + key.size() + value.size()) {
        return false;
    }
    uint16_t keyLength = static_cast<uint16_t>(key.size());
    std::memcpy(data.data() + offset, &keyLength, sizeof(keyLength));
    offset += sizeof(uint16_t);

    uint16_t valueLength = static_cast<uint16_t>(value.size());
    std::memcpy(data.data() + offset, &valueLength, sizeof(valueLength));
    offset += sizeof(uint16_t);

    std::memcpy(data.data() + offset, key.data(), key.size());
    offset += key.size();
    std::memcpy(data.data() + offset, value.data(), value.size());
    offset += value.size();

    header->freeSpaceOffset = offset;
    header->recordCount++;
    return true;
}

std::optional<std::string> Page::get(const std::string &key) {
    PageHeader *header = getHeader();
    int records = header->recordCount;
    size_t offset = sizeof(PageHeader);
    for (int i = 0; i < records; i++) {
        uint16_t storedKeyLength;
        memcpy(&storedKeyLength, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        uint16_t storedValueLength;
        memcpy(&storedValueLength, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if (storedKeyLength == key.size()) {
            if (std::memcmp(key.data(), data.data() + offset, key.size()) == 0) {
                offset += storedKeyLength;
                std::string ret(data.data() + offset, storedValueLength);
                return ret;
            }
        }
        offset += storedKeyLength + storedValueLength;
    }
    return std::nullopt;
}

bool Page::deletePair(const std::string &key) {
    PageHeader *header = getHeader();
    int records = header->recordCount;
    size_t offset = sizeof(PageHeader);
    for (int i = 0; i < records; i++) {
        uint16_t storedKeyLength;
        memcpy(&storedKeyLength, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        uint16_t storedValueLength;
        memcpy(&storedValueLength, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if (storedKeyLength == key.size()) {
            if (std::memcmp(key.data(), data.data() + offset, key.size()) == 0) {
                offset += storedKeyLength + storedValueLength;
                size_t recordSize = (2 * sizeof(uint16_t)) + storedKeyLength + storedValueLength;
                size_t bytesToMove = header->freeSpaceOffset - offset;

                memmove(data.data() + (offset - recordSize), data.data() + offset, bytesToMove);

                header->freeSpaceOffset -= recordSize;
                header->recordCount--;
                return true;
            }
        }
        offset += storedKeyLength + storedValueLength;
    }
    return false;
}

bool Page::update(const std::string &key, const std::string &value) {
    auto rec = findRecord(key);

    if (!rec) {
        return false;
    }

    size_t oldRecordSize = 2 * sizeof(uint16_t) + rec->keyLength + rec->valueLength;

    size_t newRecordSize = 2 * sizeof(uint16_t) + key.size() + value.size();

    size_t freeSpace = PAGE_SIZE - getHeader()->freeSpaceOffset;

    if (freeSpace + oldRecordSize < newRecordSize) {
        return false;
    }

    if (!deletePair(key)) {
        return false;
    }

    return insert(key, value);
}

bool Page::hasSpace(size_t keySize, size_t valueSize) {
    size_t required = (2 * sizeof(uint16_t)) + keySize + valueSize;
    return PAGE_SIZE - getHeader()->freeSpaceOffset >= required;
}

std::optional<RecordInfo> Page::findRecord(const std::string &key) {
    PageHeader *header = getHeader();
    size_t offset = sizeof(PageHeader);

    for (int i = 0; i < header->recordCount; i++) {
        uint16_t keyLength;
        std::memcpy(&keyLength, data.data() + offset, sizeof(uint16_t));

        uint16_t valueLength;
        std::memcpy(&valueLength,
                    data.data() + offset + sizeof(uint16_t),
                    sizeof(uint16_t));

        size_t keyOffset = offset + 2 * sizeof(uint16_t);

        if (keyLength == key.size() &&
            std::memcmp(key.data(), data.data() + keyOffset, keyLength) == 0) {
            return RecordInfo{
                offset,
                keyLength,
                valueLength
            };
        }

        offset += 2 * sizeof(uint16_t) + keyLength + valueLength;
    }

    return std::nullopt;
}