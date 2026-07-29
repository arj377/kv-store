#include "kv_store.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "wal.h"

#define BUFFSIZE 4096
#define MODE 0644 // Owner can read and write, everyone else can only read

// Save the database and reset the WAL page
KVStore::KVStore() : wal("wal.log")
{
    loadDatabase();
    recover();
}

KVStore::~KVStore()
{
    // Nothing to do
}

// Recover from WAL and redo actions
void KVStore::recover()
{
    auto lines = wal.replay();

    for (const auto& line : lines)
    {
        LogRecord rec;
        std::string error;

        if (!parseCommand(line, rec, error))
        {
            std::cerr << "Invalid WAL record: " << error << '\n';
            continue;
        }

        if (rec.op == SET)
        {
            set(rec.key, rec.value, false);
        }
        else if (rec.op == DEL)
        {
            del(rec.key, false);
        }
    }
}   


void KVStore::checkpoint()
{
    saveDatabase();
    wal.truncate();
}

void KVStore::loadDatabase()
{
    int n, fd; // Stores the amount of bytes read from database.db and the fd for database.db
    char buf[BUFFSIZE];
    // Open the database file and create if it doesn't exist
    if ((fd = open("database.db", O_RDWR | O_CREAT, MODE)) == -1)
    {
        perror("open");
        return;
    }

    while ((n = read(fd, buf, BUFFSIZE)) > 0) // Keep readng until EOF
    {
        std::string buffer(buf, n);
        int keyIndex = 0, valueIndex;
        std::string key, value;
        bool space = false;

        for (int i = 0; i < buffer.size(); i++)
        {
            char c = buffer[i];
            if (c == ' ' && space == false) // If first space, set this as the key
            {
                key = buffer.substr(keyIndex, i - keyIndex);
                valueIndex = i + 1;
                space = true;
            }
            if (c == '\n') // If a newline is hit, set this as the key
            {
                value = buffer.substr(valueIndex, i - valueIndex);
                database[key] = value; // Update database
                if (i + 1 < buffer.size())
                {
                    keyIndex = i + 1;
                }
                space = false;
            }
        }
    }
    if (n == -1)
    {
        perror("read");
    }
    if (close(fd) == -1)
    {
        perror("close");
    }
}

void KVStore::saveDatabase()
{
    int fd, n;
    if ((fd = open("database.db", O_WRONLY | O_CREAT | O_TRUNC, MODE)) == -1) // Open the file and overwrite anything from before
    {
        perror("open");
        return;
    }
    for (const auto &[key, value] : database)
    {
        std::string line = key + " " + value + "\n";
        int bytesWritten = 0;

        // Loop until all bytes are written
        while (bytesWritten < line.size()) // Continue until entire line is written
        {
            n = write(fd, line.c_str() + bytesWritten, line.size() - bytesWritten);
            if (n == -1)
            {
                perror("write");
                if (close(fd) == -1)
                {
                    perror("close");
                }
                return;
            }
            bytesWritten += n;
        }
    }
    if (fsync(fd) == -1) // Immediately write all changes to the disk
    {
        perror("fsync");
    }
    if (close(fd) == -1)
    {
        perror("close");
    }
}

bool KVStore::exists(const std::string &key)
{
    return database.find(key) != database.end();
}

void KVStore::set(const std::string &key, const std::string &value, bool log)
{
    // If this is not being recovered, log it to the WAL
    if (log)
    {
        LogRecord rec;
        rec.key = key;
        rec.value = value;
        rec.op = SET;
        wal.append(rec);
        wal.flush();
        writeCount++;

        // Once the checkpoint is reached, send everything to  disk
        if (writeCount >= 100)
        {
            checkpoint();
            writeCount = 0;
        }
    }

    database[key] = value;
}

std::string KVStore::get(const std::string &key)
{
    return database.at(key) + "\n"; // Throws if the key doesn't exist
}

bool KVStore::del(const std::string &key, bool log)
{
    // If this is not being recovered, log it to the WAL
   if (log)
    {
        LogRecord rec;
        rec.key = key;
        rec.op = DEL;
        wal.append(rec);
        wal.flush();
        writeCount++;

        // Once the checkpoint is reached, send everything to  disk
        if (writeCount >= 100)
        {
            checkpoint();
            writeCount = 0;
        }
    }

    if (database.find(key) == database.end())
    {
        return false;
    }
    database.erase(key);
    return true;
}

bool KVStore::parseCommand(const std::string& line, LogRecord& rec, std::string& error)
{
    std::istringstream parser(line);
    std::string command, key, value;
    parser >> command; // Creates parser to get command
    if (command.empty())
    {
        error = "No command.";
        return false;
    }
    if (command == "SET")
    {
        // Make sure there is a key
        if (!(parser >> key))
        {
            error = "Missing key.";
            return false;
        }
        parser >> std::ws;
        getline(parser, value); // Set the rest of parser to value

        // Make sure there is a value
        if (value.empty())
        {
            error = "Missing value.";
            return false;
        }
        rec.op = SET;
        rec.key = key;
        rec.value = value;
    }
    else if (command == "GET")
    {
        // make sure there is one key
        if (!(parser >> key))
        {
            error = "Missing key";
            return false;
        }
        // make sure there is no more than one key
        if (parser >> value)
        {
            error = "Too many arguments for GET.";
            return false;
        }
        rec.op = GET;
        rec.key = key;
        rec.value = "";
    }

    else if (command == "DEL")
    {
        // Make sure there is at least one key
        if (!(parser >> key))
        {
            error = "Missing key.";
            return false;
        }
        // Make sure there is no more than one key
        if (parser >> value)
        {
            error = "Too many arguments for DEL.";
            return false;
        }
        rec.op = DEL;
        rec.key = key;
        rec.value = "";
    }
    else
    {
        error = "Invalid command.";
        return false;
    }
    
    return true;
}
std::string KVStore::execute(const std::string &input)
{
    std::string error;
    LogRecord record;
    if(!parseCommand(input, record, error))
    {
        return "ERROR: " + error + '\n';
    }

    if (record.op == SET)
    {
        set(record.key, record.value);
    }
    else if (record.op == GET)
    {
        if (!exists(record.key))
        { 
            return "ERROR: Key does not exist.\n";
        }
        return get(record.key); // If the key exists, execute the function
    }
    else
    {
        if (!del(record.key))
        {
            return "ERROR: Key does not exist.\n";
        }
    }
    return "DONE!\n";
}
