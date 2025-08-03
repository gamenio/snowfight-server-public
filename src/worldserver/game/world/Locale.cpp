#include "Locale.h"

char const* langTags[TOTAL_LANGS] = {
	"en-US",
	"zh-CN",
	"ja-JP",
	"ko-KR",
};

LangType getLangTypeByLangTag(std::string const& langTag)
{
	for (uint32 i = 0; i < TOTAL_LANGS; ++i)
		if (langTag == langTags[i])
			return LangType(i);

	return LANG_enUS;
}
