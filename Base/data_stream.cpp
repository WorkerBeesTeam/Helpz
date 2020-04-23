#include "data_stream.h"

namespace Helpz {

Data_Stream::Data_Stream(const std::vector<uint8_t> *data) :
    m_data(data), m_idx(0), m_status(Ok),
    m_byteOrder(BigEndian), m_systemOrder(systemByteOrder()) {}

Data_Stream::Data_Stream(std::shared_ptr<Data_Stream::Storage> data) :
    m_ptr(data), m_data(m_ptr.get()), m_idx(0), m_status(Ok),
    m_byteOrder(BigEndian), m_systemOrder(systemByteOrder()) {}

Data_Stream::Data_Stream(std::unique_ptr<Data_Stream::Storage> &&data) :
    m_ptr(data.release()), m_data(m_ptr.get()), m_idx(0), m_status(Ok),
    m_byteOrder(BigEndian), m_systemOrder(systemByteOrder()) {}

Data_Stream::Data_Stream(Data_Stream::Storage &&data) :
    m_ptr(new Storage(std::move(data))), m_data(m_ptr.get()),
    m_idx(0), m_status(Ok), m_byteOrder(BigEndian), m_systemOrder(systemByteOrder()) {}

bool Data_Stream::atEnd() const { return m_idx == m_data->size(); }

Data_Stream &Data_Stream::operator>>(int8_t &i) {
    return read(i);
}

Data_Stream &Data_Stream::operator>>(int16_t &i) {
    return read(i);
}

Data_Stream &Data_Stream::operator>>(int32_t &i) {
    return read(i);
}

Data_Stream &Data_Stream::operator>>(uint8_t &i) {
    return read(i);
}

Data_Stream &Data_Stream::operator>>(uint16_t &i) {
    return read(i);
}

Data_Stream &Data_Stream::operator>>(uint32_t &i) {
    return read(i);
}

void Data_Stream::setByteOrder(Data_Stream::ByteOrder b) { m_byteOrder = b; }

Data_Stream::ByteOrder Data_Stream::byteOrder() const { return m_byteOrder; }

Data_Stream::ByteOrder Data_Stream::systemByteOrder() {
    const uint32_t t = 1;
    return (reinterpret_cast<const uint8_t*>(&t)) ? LittleEndian : BigEndian;
}

bool Data_Stream::has(size_t count) const { return m_idx + count <= m_data->size(); }


} // namespace Helpz
