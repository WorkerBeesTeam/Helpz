#include <unistd.h>
#include <fcntl.h>

#include "zfile.h"

namespace Helpz {

File::File(const std::string &name) : _fd(-1), _name(name) {}

File::File(const std::string &name, File::Open_Mode open_mode) : _fd(-1), _name(name) { open(open_mode); }

File::~File()
{
    close();
}

std::string File::read_all(const std::string &name, std::size_t size)
{
    File file(name, READ_ONLY);
    return file.read_all(size);
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

bool File::open(File::Open_Mode open_mode)
{
    close();
    _fd = ::open(_name.c_str(), open_mode);
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

bool File::write(const std::string &text)
{
    if (!is_opened())
        return false;

    std::size_t size = lseek(_fd, 0, SEEK_END);
    if (size == static_cast<std::size_t>(-1))
        return false;

    lseek(_fd, 0, SEEK_SET);

    bool res = ::write(_fd, text.c_str(), text.size()) == static_cast<ssize_t>(text.size());
    if (res && text.size() < size)
        ftruncate(_fd, text.size());
    return res;
}


} // namespace Helpz
