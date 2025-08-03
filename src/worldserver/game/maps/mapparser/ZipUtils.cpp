#include "ZipUtils.h"

#include <assert.h>
#include <stdlib.h>
#include <map>

#include <zlib.h>

#include "logging/Log.h"

// memory in iPhone is precious
// Should buffer factor be 1.5 instead of 2 ?
#define BUFFER_INC_FACTOR (2)

int ZipUtils::inflateMemoryWithHint(unsigned char *in, size_t inLength, unsigned char **out, size_t *outLength, size_t outLengthHint)
{
    /* ret value */
    int err = Z_OK;
    
    size_t bufferSize = outLengthHint;
    *out = (unsigned char*)malloc(bufferSize);
    
    z_stream d_stream; /* decompression stream */
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    
    d_stream.next_in  = in;
    d_stream.avail_in = static_cast<unsigned int>(inLength);
    d_stream.next_out = *out;
    d_stream.avail_out = static_cast<unsigned int>(bufferSize);
    
    /* window size to hold 256k */
    if( (err = inflateInit2(&d_stream, 15 + 32)) != Z_OK )
        return err;
    
    for (;;)
    {
        err = inflate(&d_stream, Z_NO_FLUSH);
        
        if (err == Z_STREAM_END)
        {
            break;
        }
        
        switch (err)
        {
            case Z_NEED_DICT:
                err = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&d_stream);
                return err;
        }
        
        // not enough memory ?
        if (err != Z_STREAM_END)
        {
            *out = (unsigned char*)realloc(*out, bufferSize * BUFFER_INC_FACTOR);
            
            /* not enough memory, ouch */
            if (! *out )
            {
                NS_LOG_ERROR("mapparser", "ZipUtils: realloc failed");
                inflateEnd(&d_stream);
                return Z_MEM_ERROR;
            }
            
            d_stream.next_out = *out + bufferSize;
            d_stream.avail_out = static_cast<unsigned int>(bufferSize);
            bufferSize *= BUFFER_INC_FACTOR;
        }
    }
    
    *outLength = bufferSize - d_stream.avail_out;
    err = inflateEnd(&d_stream);
    return err;
}

size_t ZipUtils::inflateMemoryWithHint(unsigned char *in, size_t inLength, unsigned char **out, size_t outLengthHint)
{
    size_t outLength = 0;
    int err = inflateMemoryWithHint(in, inLength, out, &outLength, outLengthHint);
    
    if (err != Z_OK || *out == nullptr) {
        if (err == Z_MEM_ERROR)
        {
			NS_LOG_ERROR("mapparser", "ZipUtils: Out of memory while decompressing map data!");
        } else
            if (err == Z_VERSION_ERROR)
            {
				NS_LOG_ERROR("mapparser", "ZipUtils: Incompatible zlib version!");
            } else
                if (err == Z_DATA_ERROR)
                {
					NS_LOG_ERROR("mapparser", "ZipUtils: Incorrect zlib compressed data!");
                }
                else
                {
					NS_LOG_ERROR("mapparser", "ZipUtils: Unknown error while decompressing map data!");
                }

        if(*out) {
            free(*out);
            *out = nullptr;
        }
        outLength = 0;
    }
    
    return outLength;
}

size_t ZipUtils::inflateMemory(unsigned char *in, size_t inLength, unsigned char **out)
{
    // 256k for hint
    return inflateMemoryWithHint(in, inLength, out, 256 * 1024);
}