/* Minimal Windows compatibility shims for POSIX functions missing on MSVC
 * Provides: strcasecmp, strncasecmp, strptime (limited), ftruncate, popen/pclose
 * These are intentionally small (not fully featured) and only implement the
 * behaviour needed by zsync2 for typical cases.
 */

#ifdef _WIN32

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <stdlib.h>

/* Case-insensitive string compares */
int strcasecmp(const char *a, const char *b) {
    return _stricmp(a, b);
}

int strncasecmp(const char *a, const char *b, size_t n) {
    return _strnicmp(a, b, n);
}

/* Minimal month name to number map */
static int month_str_to_int(const char *m) {
    /* Expecting 3-letter English month abbreviations */
    if (!m) return -1;
    char buf[4] = {0};
    for (int i = 0; i < 3; ++i) buf[i] = (char)tolower((unsigned char)m[i]);
    if (strcmp(buf, "jan") == 0) return 0;
    if (strcmp(buf, "feb") == 0) return 1;
    if (strcmp(buf, "mar") == 0) return 2;
    if (strcmp(buf, "apr") == 0) return 3;
    if (strcmp(buf, "may") == 0) return 4;
    if (strcmp(buf, "jun") == 0) return 5;
    if (strcmp(buf, "jul") == 0) return 6;
    if (strcmp(buf, "aug") == 0) return 7;
    if (strcmp(buf, "sep") == 0) return 8;
    if (strcmp(buf, "oct") == 0) return 9;
    if (strcmp(buf, "nov") == 0) return 10;
    if (strcmp(buf, "dec") == 0) return 11;
    return -1;
}

/* Limited strptime accepting two formats used by zsync:
 *  "%a, %d %b %Y %H:%M:%S %z" and "%d %b %Y %H:%M:%S %z"
 * It ignores the timezone offset (%z) but parses the numeric fields and
 * fills struct tm. Returns non-NULL on success, NULL on parse failure.
 */
char *strptime(const char *s, const char *fmt, struct tm *tm) {
    (void)fmt; /* we only support the specific formats used */
    int day, year, hour, min, sec;
    char month[4] = {0};
    int matched = 0;

    /* Try with weekday prefix: "Tue, 25 Jul 2006 20:02:17 +0000" */
#if defined(_MSC_VER)
    matched = sscanf_s(s, "%*[^,], %d %3s %d %d:%d:%d", &day, month, (unsigned)_countof(month), &year, &hour, &min, &sec);
#else
    matched = sscanf(s, "%*[^,], %d %3s %d %d:%d:%d", &day, month, &year, &hour, &min, &sec);
#endif
    if (matched == 6) {
        int mon = month_str_to_int(month);
        if (mon < 0) return NULL;
        memset(tm, 0, sizeof(*tm));
        tm->tm_mday = day;
        tm->tm_mon = mon;
        tm->tm_year = year - 1900;
        tm->tm_hour = hour;
        tm->tm_min = min;
        tm->tm_sec = sec;
        tm->tm_isdst = -1;
        return (char *)s + strlen(s);
    }

    /* Try without weekday: "25 Jul 2006 20:02:17 +0000" */
    matched = 0;
#if defined(_MSC_VER)
    matched = sscanf_s(s, "%d %3s %d %d:%d:%d", &day, month, (unsigned)_countof(month), &year, &hour, &min, &sec);
#else
    matched = sscanf(s, "%d %3s %d %d:%d:%d", &day, month, &year, &hour, &min, &sec);
#endif
    if (matched == 6) {
        int mon = month_str_to_int(month);
        if (mon < 0) return NULL;
        memset(tm, 0, sizeof(*tm));
        tm->tm_mday = day;
        tm->tm_mon = mon;
        tm->tm_year = year - 1900;
        tm->tm_hour = hour;
        tm->tm_min = min;
        tm->tm_sec = sec;
        tm->tm_isdst = -1;
        return (char *)s + strlen(s);
    }

    return NULL;
}

/* ftruncate replacement using _chsize_s when available */
int ftruncate(int fd, long long length) {
#if defined(_MSC_VER)
    /* _chsize_s returns 0 on success */
    if (_chsize_s(fd, (long long)length) == 0)
        return 0;
    return -1;
#else
    /* Fallback to _chsize if present */
    if (_chsize(fd, (long)length) == 0)
        return 0;
    return -1;
#endif
}

/* popen/pclose wrappers to map to MSVC names */
FILE *popen(const char *command, const char *type) {
    return _popen(command, type);
}

int pclose(FILE *stream) {
    return _pclose(stream);
}

#endif /* _WIN32 */
