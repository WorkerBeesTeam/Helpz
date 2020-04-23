#ifndef HELPZ_DATA_STREAM_H
#define HELPZ_DATA_STREAM_H

#include <vector>
#include <memory>
#include <cstdint>
#include <cassert>

namespace Helpz {

class Data_Stream
{
public:
    typedef std::vector<uint8_t> Storage;
    enum ByteOrder { BigEndian, LittleEndian };
    enum Status { Ok, ReadPastEnd };

    Data_Stream(const std::vector<uint8_t> * data);
    Data_Stream(std::shared_ptr<Storage> data);
    Data_Stream(std::unique_ptr<Storage> && data);
    Data_Stream(Storage && data);
    bool atEnd() const;
    Data_Stream & operator>>(int8_t & i);
    Data_Stream & operator>>(int16_t & i);
    Data_Stream & operator>>(int32_t & i);
    Data_Stream & operator>>(uint8_t & i);
    Data_Stream & operator>>(uint16_t & i);
    Data_Stream & operator>>(uint32_t & i);
    void setByteOrder(ByteOrder b);
    ByteOrder byteOrder() const;

private:
    std::shared_ptr<const Storage> m_ptr;
    const Storage * m_data;
    size_t m_idx;
    Status m_status;
    ByteOrder m_byteOrder, m_systemOrder;
    static ByteOrder systemByteOrder();

    bool has(size_t count) const;

    template<typename T>
    Data_Stream & read(T & i)
    {
        if (has(sizeof(T)) && Ok == m_status) {
            T result = *reinterpret_cast<const T*>(&(*m_data)[m_idx]);
            m_idx += sizeof(T);
            if (m_byteOrder != m_systemOrder) {
                T tmp = 0;
                for (uint8_t i = 0; i < sizeof(T); ++i) {
                    tmp = (tmp << 8) | (result & 0xFF);
                    result = result >> 8;
                }
                i = tmp;
            } else
                i = result;
        } else {
            m_status = ReadPastEnd;
        }
        return *this;
    }
};

} // namespace Helpz

//int main()
//{
//    std::vector<uint8_t> v;
//    v.push_back(1);
//    v.push_back(2);
//    v.push_back(3);
//    v.push_back(4);
//    uint32_t val;
//    Data_Stream s_be(&v);
//    s_be >> val;
//    assert(val == 0x01020304); // big endian
//    Data_Stream s_le(&v);
//    s_le.setByteOrder(Data_Stream::LittleEndian);
//    s_le >> val;
//    assert(val == 0x04030201); // little endian
//    return 0;
//}

#endif // HELPZ_DATA_STREAM_H
