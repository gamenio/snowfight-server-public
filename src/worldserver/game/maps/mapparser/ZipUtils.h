#ifndef __ZIP_UTILS_H__
#define __ZIP_UTILS_H__

#include <string>

class ZipUtils
{
public:
	/**
	* Inflates either zlib or gzip deflated memory. The inflated memory is expected to be freed by the caller.
	*
	* It will allocate 256k for the destination buffer. If it is not enough it will multiply the previous buffer size per 2, until there is enough memory.
	*
	* @return The length of the deflated buffer.
	* @since v0.8.1
	*/
	static size_t inflateMemory(unsigned char *in, size_t inLength, unsigned char **out);

	/**
	* Inflates either zlib or gzip deflated memory. The inflated memory is expected to be freed by the caller.
	*
	* @param outLengthHint It is assumed to be the needed room to allocate the inflated buffer.
	*
	* @return The length of the deflated buffer.
	* @since v1.0.0
	*/
	static size_t inflateMemoryWithHint(unsigned char *in, size_t inLength, unsigned char **out, size_t outLengthHint);

private:
	static int inflateMemoryWithHint(unsigned char *in, size_t inLength, unsigned char **out, size_t *outLength, size_t outLengthHint);
};


#endif // __ZIP_UTILS_H__
