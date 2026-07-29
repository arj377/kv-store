#include "wal.h"
#include "kv_store.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream> // if parsing log records

#define BUFSIZE 4096
#define MODE 0644

// Opens/creates WAL file and initalizes the filename
WAL::WAL(const std::string &file)
{
    fd = open(file.c_str(), O_RDWR | O_CREAT, MODE);
    if (fd == -1)
    {
        perror("open");
    }
    filename = file;
}

// Close file descriptor
WAL::~WAL()
{
    if (close(fd) == -1)
    {
        perror("close");
    }
}

// Add a new log record to wal.log
void WAL::append(LogRecord log)
{
    std::string operation = "";
    switch (log.op)
    {
    case Operation::SET:
        operation = "SET";
        break;
    case Operation::DEL:
        operation = "DEL";
        break;
    default:
        break;
    }

    std::string buf = operation + " " + log.key + " " + log.value + "\n"; // Readable format
    size_t total = buf.size();
    size_t counter = 0;

    std::lock_guard<std::mutex> lock(mutex); // Only one writer at a time
    while (counter < total) // Write until entire log is written
    {
        ssize_t charWritten = write(fd, buf.c_str() + counter, total - counter);
        if (charWritten <= 0)
        {
            perror("write");
            break;
        }
        else
        {
            counter += charWritten;
        }
    }
}

// Send everything to wal.log
void WAL::flush()
{
    if (fsync(fd) == -1)
    {
        perror("fsync");
    }
}

// Redo all the actions from wal.log
std::vector<std::string> WAL::replay()
{
    ssize_t bytesRead;
    char buf[BUFSIZE];
    std::string remaining = "";
    std::vector<std::string> records;

    if (lseek(fd, 0, SEEK_SET) == -1)
    { // Make sure to start at the beginning of the file
        perror("lseek");
        return records;
    }
    while (true)
    {
        bytesRead = read(fd, buf, BUFSIZE);
        if (bytesRead == -1)
        {
            perror("read");
            break;
        }
        if (bytesRead == 0)
        {
            break;
        }
        std::string buffer = remaining + std::string(buf, bytesRead);
        size_t index;
        while ((index = buffer.find("\n")) != std::string::npos) // New line = new command, key, value
        {
            records.emplace_back(buffer.substr(0,index));
            buffer = buffer.substr(index + 1);
        }
        remaining = buffer; // Store the partial bytes for the next iteration
    }

    if (!remaining.empty())
    {
        std::cerr << "Invalid WAL record encountered: Incomplete record.\n";
    }

    return records;
}

// Empty the WAL page
void WAL::truncate()
{
    if (close(fd) == -1)
    {
        perror("close");
    }
    fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, MODE); // Old content can be overwritten
    if (fd == -1)
    {
        perror("open");
    }
}