#include "disk_manager.h"
#include "page.h"

#include <fcntl.h>   // open
#include <stdexcept> // std::runtime_error (if you throw exceptions)
#include <unistd.h>  // read, write, lseek, close
#include <algorithm>
#include <iostream>

DiskManager::DiskManager() {
    if ((fd = open("database.db", O_CREAT | O_RDWR, 0644)) == -1) {
        perror("open");
        throw std::runtime_error("Failed to open database.db");
    }
    
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == -1) {
        perror("lseek");
        close(fd);
        throw std::runtime_error("Failed to access database.db size");
    }

    nextAvailableID = fileSize / PAGE_SIZE;
    std::cout << "fileSize = " << fileSize
          << ", nextAvailableID = " << nextAvailableID
          << '\n';
}

DiskManager::~DiskManager() {
    if (fd != -1) {
        close(fd);
    }
}

void DiskManager::readPage(page_id_t id, Page &page) {
    if (id >= nextAvailableID) {
        throw std::runtime_error("Page does not exist");
    }
    off_t offset = id * PAGE_SIZE;
    size_t total = 0;
    ssize_t bytesRead;
    while (total != PAGE_SIZE) {
        bytesRead = pread(fd, page.getData().data() + total, PAGE_SIZE - total, offset + total);
        if (bytesRead == -1) {
            perror("pread");
            throw std::runtime_error("Failed to read database.db data");
        } else if (bytesRead == 0) {
            throw std::runtime_error("Page too short");
        }
        total += bytesRead;
    }
}

void DiskManager::writePage(page_id_t id, Page &page) {
    if (id >= nextAvailableID) {
        throw std::runtime_error("Page does not exist");
    }
    off_t offset = id * PAGE_SIZE;
    size_t total = 0;
    ssize_t bytesWritten;
    while (total != PAGE_SIZE) {
        bytesWritten = pwrite(fd, page.getData().data() + total, PAGE_SIZE - total, offset + total);
        if (bytesWritten == -1) {
            perror("pwrite");
            throw std::runtime_error("Failed to write to database.db");
        } else if (bytesWritten == 0) {
            throw std::runtime_error("Page too short");
        }
        total += bytesWritten;
    }
    if (fsync(fd) == -1) {
        perror("fsync");
        throw std::runtime_error("Failed to write page");
    }
}

page_id_t DiskManager::allocatePage() {
    page_id_t pageID;

    if (!freePageList.empty()) {
        pageID = freePageList.back();
        freePageList.pop_back();
    } else {
        pageID = nextAvailableID++;
    }

    Page newPage(pageID);

    off_t offset = static_cast<off_t>(pageID) * PAGE_SIZE;

    size_t total = 0;
    ssize_t bytesWritten;

    while (total != PAGE_SIZE) {
        bytesWritten =
            pwrite(fd, newPage.getData().data() + total, PAGE_SIZE - total, offset + total);

        if (bytesWritten == -1) {
            perror("pwrite");
            throw std::runtime_error("Failed to create a new page");
        } else if (bytesWritten == 0) {
            throw std::runtime_error("Failed to create a new page");
        }

        total += bytesWritten;
    }

    if (fsync(fd) == -1) {
        perror("fsync");
        throw std::runtime_error("Failed to create a new page");
    }

    return pageID;
}

void DiskManager::deallocatePage(page_id_t pageID) {
    if (std::find(freePageList.begin(), freePageList.end(), pageID) == freePageList.end()) {
        freePageList.push_back(pageID);
    }
}

page_id_t DiskManager::getNextAvailableID() const {
    return nextAvailableID;
}
