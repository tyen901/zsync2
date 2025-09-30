#include <gtest/gtest.h>
#include <string.h>

#include "../include/gcrypt_compat.h"

TEST(GcryptCompat, Sha1Basic) {
    gcry_md_hd_t h = nullptr;
    int err = gcry_md_open(&h, GCRY_MD_SHA1, 0);
    EXPECT_EQ(err, 0);
    const char *msg = "abc";
    gcry_md_write(h, msg, strlen(msg));
    const unsigned char *digest = gcry_md_read(h, GCRY_MD_SHA1);
    EXPECT_NE(digest, nullptr);
    size_t dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
    EXPECT_EQ(dlen, 20u);
    gcry_md_close(h);
}
