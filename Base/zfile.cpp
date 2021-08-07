#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> // mkdir
#include <cstring>

#include "zfile.h"

namespace Helpz {

File::File(const std::string &name) : _name(name) {}
File::File(const std::string &name, int open_mode, int permissions) : _name(name) { open(open_mode, permissions); }

File::File(File &&o) :
    _fd{std::move(o._fd)},
    _name{std::move(o._name)}
{
    o._fd = -1;
}

File &File::operator=(File &&o)
{
    _name = std::move(o._name);
    _fd = std::move(o._fd);
    o._fd = -1;
    return *this;
}

File::~File()
{
    close();
}

/*static*/ bool File::exist(const std::string &name)
{
    return access( name.c_str(), F_OK ) != -1;
}

/*static*/ std::string File::read_all(const std::string &name, std::size_t size)
{
    File file(name, READ_ONLY);
    return file.read_all(size);
}

/*static*/ bool File::write(const std::string &name, const char *data, std::size_t data_size, int open_mode, int permissions)
{
    File file(name, open_mode, permissions);
    return file.write(data, data_size);
}

/*static*/ bool File::create_dir(const std::string &name, int permissions)
{
    return mkdir(name.c_str(), permissions) != -1;
}

std::string File::name() const
{
    return _name;
}

std::size_t File::pos() const { return lseek(_fd, 0, SEEK_CUR); }

std::size_t File::size() const
{
    std::size_t curr = pos();
    lseek(_fd, 0, SEEK_END);
    std::size_t size = pos();
    lseek(_fd, curr, SEEK_SET);
    return size;
}

std::size_t File::read(char *data, std::size_t size)
{
    return ::read(_fd, data, size);
}

std::string File::read(std::size_t size)
{
    std::string text(size, char());
    size = ::read(_fd, const_cast<char*>(text.data()), size);
    if (size > 0)
        text.resize(size);
    return text;
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
    size = ::read(_fd, const_cast<char*>(text.data()), size);
    if (size > 0)
        text.resize(size);
    return text;
}

bool File::is_opened() const
{
    return _fd >= 0;
}

File::operator bool() const
{
    return is_opened();
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

std::string File::last_error() const
{
    return std::strerror(errno);
}

int File::descriptor() const
{
    return _fd;
}

} // namespace Helpz
