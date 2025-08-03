#include "StringUtil.h"

#include "utf8.h"

#if PLATFORM == PLATFORM_WINDOWS
#include <Windows.h>
#endif

Tokenizer::Tokenizer(const std::string &src, const char sep, uint32 vectorReserve)
{
    m_str = new char[src.length() + 1];
    memcpy(m_str, src.c_str(), src.length() + 1);

    if (vectorReserve)
        m_storage.reserve(vectorReserve);

    char* posold = m_str;
    char* posnew = m_str;

    for (;;)
    {
        if (*posnew == sep)
        {
            m_storage.push_back(posold);
            posold = posnew + 1;

            *posnew = '\0';
        }
        else if (*posnew == '\0')
        {
            // Hack like, but the old code accepted these kind of broken strings,
            // so changing it would break other things
            if (posold != posnew)
                m_storage.push_back(posold);

            break;
        }

        ++posnew;
    }
}

#if PLATFORM == PLATFORM_WINDOWS

std::string StringUtil::wideCharToUtf8(std::wstring const& strWideChar)
{
	std::string ret;
	if (!strWideChar.empty())
	{
		int nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, nullptr, 0, nullptr, FALSE);
		if (nNum)
		{
			char* utf8String = new char[nNum + 1];
			utf8String[0] = 0;

			nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, utf8String, nNum + 1, nullptr, FALSE);

			ret = utf8String;
			delete[] utf8String;
		}
		else
		{
			printf("Wrong convert to Utf8 code:0x%x", GetLastError());
		}
	}

	return ret;
}

std::wstring StringUtil::ansiToWideChar(std::string const& strAnsi)
{
	std::wstring ret;
	if (!strAnsi.empty())
	{
		int nNum = MultiByteToWideChar(CP_ACP, 0, strAnsi.c_str(), -1, nullptr, 0);
		if (nNum)
		{
			WCHAR* wideCharString = new WCHAR[nNum + 1];
			wideCharString[0] = 0;

			nNum = MultiByteToWideChar(CP_ACP, 0, strAnsi.c_str(), -1, wideCharString, nNum + 1);

			ret = wideCharString;
			delete[] wideCharString;
		}
		else
		{
			printf("Wrong convert to WideChar code:0x%x", GetLastError());
		}
	}
	return ret;
}

#endif