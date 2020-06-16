#ifndef HELPZ_FILE_H
#define HELPZ_FILE_H

#include <string>

namespace Helpz {

class File
{
public:
    enum Open_Mode { READ_ONLY, WRITE_ONLY, READ_WRITE };

    explicit File(const std::string& name);
    explicit File(const std::string& name, Open_Mode open_mode);
    ~File();

    static std::string read_all(const std::string& name, std::size_t size = -1);
    std::string read_all(std::size_t size = -1);
    bool is_opened() const;
    bool open(Open_Mode open_mode = READ_WRITE);
    void close();
    bool write(const std::string& text);

private:
    int _fd;
    const std::string _name;
};

} // namespace Helpz

#endif // HELPZ_FILE_H
