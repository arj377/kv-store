#include "kv_store.h"
#include "wal.h"
#include <iostream>
#include <sstream>

// Save the database and reset the WAL page
KVStore::KVStore() : wal("wal.log"), writeCount(0) {
    if (!bufferPoolManager.pageExists(0)) {
        Page *p = bufferPoolManager.newPage();
        if (p != nullptr) {
            bufferPoolManager.unpinPage(p->getPageID());
        }
    }
    recover();
}

KVStore::~KVStore() {
    // Nothing to do
}

// Recover from WAL and redo actions
void KVStore::recover() {
    auto lines = wal.replay();

    for (const auto &line : lines) {
        LogRecord rec;
        std::string error;

        if (!parseCommand(line, rec, error)) {
            std::cerr << "Invalid WAL record: " << error << '\n';
            continue;
        }

        if (rec.op == SET) {
            set(rec.key, rec.value, false);
        } else if (rec.op == DEL) {
            del(rec.key, false);
        }
    }
}

void KVStore::checkpoint() {
    bufferPoolManager.flushAllPages();
    wal.truncate();
}

bool KVStore::set(const std::string &key, const std::string &value, bool log) {
    std::lock_guard<std::mutex> lock(mutex_);
    page_id_t targetPageID = -1;
    bool updating = false;
    Page *targetPage = nullptr;

    // Find either:
    // 1. the page containing the key
    // 2. the first page with enough free space
    for (page_id_t id = 0; id < bufferPoolManager.getPageCount(); id++) {
        Page *page = bufferPoolManager.fetchPage(id);

        if (page == nullptr) {
            continue;
        }

        if (page->get(key) != std::nullopt) {
            updating = true;
            targetPageID = id;
            bufferPoolManager.unpinPage(id);
            break;
        }

        if (targetPageID == -1 && page->hasSpace(key.size(), value.size())) {
            targetPageID = id;
        }

        bufferPoolManager.unpinPage(id);
    }

    // Need another page
    if (!updating && targetPageID == -1) {
        targetPage = bufferPoolManager.newPage();

        if (targetPage == nullptr) {
            return false;
        }

        targetPageID = targetPage->getPageID();

        if (!targetPage->hasSpace(key.size(), value.size())) {
            bufferPoolManager.unpinPage(targetPageID);
            return false;
        }

        bufferPoolManager.unpinPage(targetPageID);
    }

    // WAL must hit disk before modifying the page
    if (log) {
        LogRecord rec;
        rec.key = key;
        rec.value = value;
        rec.op = SET;

        wal.append(rec);
        wal.flush();
    }

    targetPage = bufferPoolManager.fetchPage(targetPageID);

    if (targetPage == nullptr) {
        return false;
    }

    bool success;

    if (updating) {
        success = targetPage->update(key, value);
    } else {
        success = targetPage->insert(key, value);
    }

    if (success) {
        targetPage->setDirty(true);
    }

    bufferPoolManager.unpinPage(targetPageID);

    if (!success) {
        return false;
    }

    if (log) {
        writeCount++;

        if (writeCount >= 100) {
            checkpoint();
            writeCount = 0;
        }
    }

    return true;
}

std::optional<std::string> KVStore::get(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (page_id_t id = 0; id < bufferPoolManager.getPageCount(); id++) {
        Page *page = bufferPoolManager.fetchPage(id);

        if (page == nullptr) {
            continue;
        }

        std::optional<std::string> value = page->get(key);
        bufferPoolManager.unpinPage(id);

        if (value.has_value()) {
            return value;
        }
    }

    return std::nullopt;
}

bool KVStore::del(const std::string &key, bool log) {
    std::lock_guard<std::mutex> lock(mutex_);
    page_id_t targetPageID = -1;

    // Find the page containing the key.
    for (page_id_t id = 0; id < bufferPoolManager.getPageCount(); id++) {
        Page *page = bufferPoolManager.fetchPage(id);

        if (page == nullptr) {
            continue;
        }

        bool found = page->get(key) != std::nullopt;
        bufferPoolManager.unpinPage(id);

        if (found) {
            targetPageID = id;
            break;
        }
    }

    if (targetPageID == -1) {
        return false;
    }

    // WAL must reach disk before the page modification.
    if (log) {
        LogRecord rec;
        rec.key = key;
        rec.value = "";
        rec.op = DEL;

        wal.append(rec);
        wal.flush();
    }

    Page *page = bufferPoolManager.fetchPage(targetPageID);

    if (page == nullptr) {
        return false;
    }

    bool success = page->deletePair(key);

    if (success) {
        page->setDirty(true);
    }

    bufferPoolManager.unpinPage(targetPageID);

    if (!success) {
        return false;
    }

    if (log) {
        writeCount++;

        if (writeCount >= 100) {
            checkpoint();
            writeCount = 0;
        }
    }

    return true;
}

bool KVStore::parseCommand(const std::string &line, LogRecord &rec, std::string &error) {
    std::istringstream parser(line);
    std::string command, key, value;
    parser >> command; // Creates parser to get command
    if (command.empty()) {
        error = "No command.";
        return false;
    }
    if (command == "SET") {
        // Make sure there is a key
        if (!(parser >> key)) {
            error = "Missing key.";
            return false;
        }
        parser >> std::ws;
        getline(parser, value); // Set the rest of parser to value

        // Make sure there is a value
        if (value.empty()) {
            error = "Missing value.";
            return false;
        }
        rec.op = SET;
        rec.key = key;
        rec.value = value;
    } else if (command == "GET") {
        // make sure there is one key
        if (!(parser >> key)) {
            error = "Missing key";
            return false;
        }
        // make sure there is no more than one key
        if (parser >> value) {
            error = "Too many arguments for GET.";
            return false;
        }
        rec.op = GET;
        rec.key = key;
        rec.value = "";
    }

    else if (command == "DEL") {
        // Make sure there is at least one key
        if (!(parser >> key)) {
            error = "Missing key.";
            return false;
        }
        // Make sure there is no more than one key
        if (parser >> value) {
            error = "Too many arguments for DEL.";
            return false;
        }
        rec.op = DEL;
        rec.key = key;
        rec.value = "";
    } else {
        error = "Invalid command.";
        return false;
    }

    return true;
}