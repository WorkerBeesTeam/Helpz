#ifndef HELPZ_FILE_H
#define HELPZ_FILE_H

#include <string>

namespace Helpz {

class File
{
public:
    enum Open_Mode {
        READ_ONLY  = 00,
        WRITE_ONLY = 01,
        READ_WRITE = 02,

        CREATE     = 0100,

        TRUNCATE   = 01000,
        APPEND     = 02000
    };

    enum File_Permissions {
        READ_BY_OWNER   = 0400,
        WRITE_BY_OWNER  = 0200,
        EXEC_BY_OWNER   = 0100,

        READ_WRITE_EXEC_BY_OWNER = (READ_BY_OWNER|WRITE_BY_OWNER|EXEC_BY_OWNER),

        READ_BY_GROUP   = (READ_BY_OWNER >> 3),
        WRITE_BY_GROUP  = (WRITE_BY_OWNER >> 3),
        EXEC_BY_GROUP   = (EXEC_BY_OWNER >> 3),

        READ_WRITE_EXEC_BY_GROUP = (READ_BY_GROUP|WRITE_BY_GROUP|EXEC_BY_GROUP),

        READ_BY_OTHER   = (READ_BY_GROUP >> 3),
        WRITE_BY_OTHER  = (WRITE_BY_GROUP >> 3),
        EXEC_BY_OTHER   = (EXEC_BY_GROUP >> 3),

        READ_WRITE_EXEC_BY_OTHER = (READ_BY_OTHER|WRITE_BY_OTHER|EXEC_BY_OTHER),

        DEFAULT_CREATE_PERM = (READ_BY_OWNER|WRITE_BY_OWNER|READ_BY_GROUP|WRITE_BY_GROUP|READ_BY_OTHER|WRITE_BY_OTHER),
        DEFAULT_MKDIR_PERM = (READ_WRITE_EXEC_BY_OWNER|READ_BY_GROUP|READ_BY_OTHER)
    };

    explicit File(const std::string& name);
    explicit File(const std::string& name, int open_mode, int permissions = DEFAULT_CREATE_PERM);
    ~File();

    static bool exist(const std::string& name);
    static std::string read_all(const std::string& name, std::size_t size = -1);
    static bool write(const std::string& name, const char* data, std::size_t data_size,
                      int open_mode = CREATE | WRITE_ONLY | TRUNCATE, int permissions = DEFAULT_CREATE_PERM);

    static bool create_dir(const std::string& name, int permissions = DEFAULT_MKDIR_PERM);

    std::string read_all(std::size_t size = -1);
    bool is_opened() const;
    bool open(int open_mode = READ_WRITE, int file_attibute = DEFAULT_CREATE_PERM);
    void close();

    bool rewrite(const std::string& text);
    bool rewrite(const char* data, std::size_t data_size);

    bool write(const std::string& text);
    bool write(const char* data, std::size_t data_size);

    bool truncate(std::size_t size);

    bool seek(int pos);

private:
    int _fd;
    const std::string _name;
};

} // namespace Helpz

#endif // HELPZ_FILE_H
