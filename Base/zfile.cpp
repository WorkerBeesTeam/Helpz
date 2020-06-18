#include <unistd.h>
#include <fcntl.h>

#include "zfile.h"

namespace Helpz {

File::File(const std::string &name) : _fd(-1), _name(name) {}

File::File(const std::string &name, int open_mode, int file_attibute) : _fd(-1), _name(name) { open(open_mode, file_attibute); }

File::~File()
{
    close();
}

std::string File::read_all(const std::string &name, std::size_t size)
{
    File file(name, READ_ONLY);
    return file.read_all(size);
}

bool File::write(const std::string &name, const char *data, std::size_t data_size, int open_mode, int file_attibute)
{
    File file(name, open_mode, file_attibute);
    return file.write(data, data_size);
}

std::string File::read_all(std::size_t size)
{
    if (!is_opened())
        return {};

    if (size == static_cast<std::size_t>(-1))
    {
        size = lseek(_fd, 0, SEEK_END);
        if (size == static_cast<std::size_t>(-1))
            return {};
    }

    lseek(_fd, 0, SEEK_SET);

    std::string text(size, char());
    size = read(_fd, text.data(), size);
    if (size > 0)
        text.resize(size);
    return text;
}

bool File::is_opened() const
{
    return _fd != -1;
}

bool File::open(int open_mode, int file_attibute)
{
    close();
    _fd = ::open(_name.c_str(), open_mode, file_attibute);
    return is_opened();
}

void File::close()
{
    if (is_opened())
    {
        ::close(_fd);
        _fd = -1;
    }
}

bool File::rewrite(const std::string &text)
{
    return rewrite(text.c_str(), text.size());
}

bool File::rewrite(const char *data, std::size_t data_size)
{
    return seek(0) && write(data, data_size) && truncate(data_size);
}

bool File::write(const std::string &text)
{
    return write(text.c_str(), text.size());
}

bool File::write(const char *data, std::size_t data_size)
{
    if (!is_opened())
        return false;
    return ::write(_fd, data, data_size) == static_cast<ssize_t>(data_size);
}

bool File::truncate(std::size_t size)
{
    return ::ftruncate(_fd, size) != -1;
}

bool File::seek(int pos)
{
    return lseek(_fd, pos, SEEK_SET) != -1;
}

} // namespace Helpz
