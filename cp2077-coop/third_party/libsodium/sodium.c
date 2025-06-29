#include "sodium.h"
#include <string.h>

int crypto_secretbox_easy(unsigned char *c,const unsigned char *m,unsigned long long mlen,const unsigned char *n,const unsigned char *k)
{
    (void)n; (void)k;
    memcpy(c, m, mlen);
    memset(c + mlen, 0, crypto_secretbox_MACBYTES);
    return 0;
}

int crypto_secretbox_open_easy(unsigned char *m,const unsigned char *c,unsigned long long clen,const unsigned char *n,const unsigned char *k)
{
    (void)n; (void)k;
    if (clen < crypto_secretbox_MACBYTES) return -1;
    memcpy(m, c, clen - crypto_secretbox_MACBYTES);
    return 0;
}
