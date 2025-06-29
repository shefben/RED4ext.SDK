#include "zstd.h"
#include <string.h>

size_t ZSTD_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int level)
{
    (void)level;
    if (dstCapacity < srcSize)
        return (size_t)-1;
    memcpy(dst, src, srcSize);
    return srcSize;
}

size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize)
{
    if (dstCapacity < compressedSize)
        return (size_t)-1;
    memcpy(dst, src, compressedSize);
    return compressedSize;
}

unsigned ZSTD_isError(size_t code)
{
    return code == (size_t)-1;
}

size_t ZSTD_compressBound(size_t srcSize)
{
    return srcSize + 64;
}
