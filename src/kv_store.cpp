#include "kv_store.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFSIZE 4096
#define MODE 0644 // Owner can read and write, everyone else can only read

std::unordered_map<std::string, std::string> database;

void loadDatabase()
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
    close(fd);
}

void saveDatabase()
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
                close(fd);
                return;
            }
            bytesWritten += n;
        }
    }
    if (fsync(fd) == -1) // Immediately write all changes to the disk
    {
        perror("fsync");
    }
    close(fd);
}

bool exists(const std::string &key)
{
    return database.find(key) != database.end();
}

void set(const std::string &key, const std::string &value)
{
    database[key] = value;
    saveDatabase(); // Save changes
}

std::string get(const std::string &key)
{
    return database.at(key) + "\n"; // Throws if the key doesn't exist
}

bool del(const std::string &key)
{
    if (database.find(key) == database.end())
    {
        return false;
    }
    database.erase(key);
    saveDatabase(); // Save changes
    return true;
}

std::string execute(const std::string &input)
{
    std::istringstream parser(input);
    std::string command, key, value;
    parser >> command; // creates parser to get command
    if (command.empty())
    {
        return "ERROR: Enter a command and its key and/or value.\n";
    }

    if (command == "SET")
    {
        // make sure there is a key
        if (!(parser >> key))
        {
            return "ERROR: SET expects exactly one key.\n";
        }
        parser >> std::ws;      // skip all whitespace
        getline(parser, value); // set the rest of parser to value

        // make sure there is a value
        if (value.empty())
        {
            return "ERROR: SET expects exactly one value.\n";
        }
        set(key, value);
        return "DONE.\n";
    }

    if (command == "GET")
    {
        // make sure there is at least one key
        if (!(parser >> key))
        {
            return "ERROR: GET expects exactly one key.\n";
        }
        // make sure there is no more than one key
        if (parser >> value)
        {
            return "ERROR: GET expects exactly one key.\n";
        }

        if (exists(key))
        { // if the key exists, execute the function
            return get(key);
        }
        else
        {
            return "ERROR: Key \"" + key + "\" does not exist.\n";
        }
    }

    if (command == "DEL")
    {
        // make sure there is at least one key
        if (!(parser >> key))
        {
            return "ERROR: DEL expects exactly one key.\n";
        }
        // make sure there is no more than one key
        if (parser >> value)
        {
            return "ERROR: DEL expects exactly one key.\n";
        }
        if (del(key))
        { // success
            return "DONE.\n";
        }
        else
        {
            return "ERROR: Key \"" + key + "\" does not exist.\n";
        }
    }

    return "ERROR: Unknown command.\n";
}