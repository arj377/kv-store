#pragma once

#include "config.h"
#include "page.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class DiskManager {
  public:
    DiskManager();
    ~DiskManager();
    void readPage(page_id_t, Page &);  // Read from the page specified to a Page
    void writePage(page_id_t, Page &); // Write the contents of Page to the page specified
    page_id_t allocatePage();
    void deallocatePage(page_id_t);
    page_id_t getNextAvailableID() const;

  private:
    int fd; // Database file
    page_id_t nextAvailableID;
    std::vector<page_id_t> freePageList;
};