#pragma once
#include <string>
#include <mutex>
#include <vector>

enum Operation{SET, DEL, GET}; // Get only appears in execute, and not replay

struct LogRecord {
    Operation op;
    std::string key;
    std:: string value;
};

class WAL
{
public:
    WAL(const std::string& filename);
    ~WAL();
    void append(LogRecord log);
    void flush();
    std::vector<std::string> replay();
    void truncate();
    

private:
    int fd; // WAL file descriptor
    std::string filename; // Path to wal.log
    std::mutex mutex; // Protect concurrent appends (if multithreaded)

};