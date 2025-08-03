#ifndef __LOCALE_H__
#define __LOCALE_H__

#include "Common.h"

enum LangType
{
	LANG_enUS,
	LANG_zhCN,
	LANG_jaJP,
	LANG_koKR,
	TOTAL_LANGS
};

#define DEFAULT_LANG			LANG_enUS

extern char const* langTags[TOTAL_LANGS];

LangType getLangTypeByLangTag(std::string const& langTag);

#endif // __LOCALE_H__