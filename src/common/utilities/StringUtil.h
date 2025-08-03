#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#include "cppformat/format.h"

#include "Common.h"

class Tokenizer
{
public:
    typedef std::vector<char const*> StorageType;

    typedef StorageType::size_type size_type;

    typedef StorageType::const_iterator const_iterator;
    typedef StorageType::reference reference;
    typedef StorageType::const_reference const_reference;

public:
    Tokenizer(const std::string &src, char const sep, uint32 vectorReserve = 0);
    ~Tokenizer() { delete[] m_str; }

    const_iterator begin() const { return m_storage.begin(); }
    const_iterator end() const { return m_storage.end(); }

    size_type size() const { return m_storage.size(); }

    reference operator [] (size_type i) { return m_storage[i]; }
    const_reference operator [] (size_type i) const { return m_storage[i]; }

private:
    char* m_str;
    StorageType m_storage;
};

namespace StringUtil
{
    
template<typename Format, typename... Args>
inline std::string format(Format&& fmt, Args&&... args)
{
    return fmt::sprintf(std::forward<Format>(fmt), std::forward<Args>(args)...);
}

#if PLATFORM == PLATFORM_WINDOWS

std::string wideCharToUtf8(const std::wstring& strWideChar);
std::wstring ansiToWideChar(std::string const& strAnsi);

#endif

} // namespace StringUtil



#endif // __STRING_UTIL_H__
