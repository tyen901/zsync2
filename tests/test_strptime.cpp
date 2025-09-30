#include <gtest/gtest.h>
#include <time.h>
#include <string.h>

/* strptime shim lives in src/windows_compat.c on Windows and is available in build */
extern "C" char *strptime(const char *s, const char *fmt, struct tm *tm);

TEST(StrptimeTest, ParsesRfc822WithZ) {
    const char *s = "Tue, 25 Jul 2006 20:02:17 +0000";
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    char *res = strptime(s, "%a, %d %b %Y %H:%M:%S %z", &tm);
    EXPECT_NE(res, nullptr);
    EXPECT_EQ(tm.tm_mday, 25);
    EXPECT_EQ(tm.tm_mon, 6); /* July = 6 */
    EXPECT_EQ(tm.tm_year, 2006 - 1900);
    EXPECT_EQ(tm.tm_hour, 20);
    EXPECT_EQ(tm.tm_min, 2);
    EXPECT_EQ(tm.tm_sec, 17);
}
