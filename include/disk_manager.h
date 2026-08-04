#pragma once

#include "config.h"
#include "page.h"
#include <string>
#include <fstream>
#include <array>
#include <vector>
#include <cstdint>

class DiskManager {
public:
    DiskManager();
    ~DiskManager();
    void readPage(page_id_t, Page&); // Read from the page specified to a Page
    void writePage(page_id_t, Page&); // Write the contents of Page to the page specified
    page_id_t allocatePage();
    void deallocatePage(page_id_t);
    page_id_t getNextAvailableID() const;


private:
    int fd; // Database file
    page_id_t nextAvailableID;
    std::vector<page_id_t> freePageList;

};