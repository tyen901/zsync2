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

/* my_timegm: portable wrapper to convert a UTC struct tm to time_t.
 * On MSVC use _mkgmtime; on platforms with timegm use that; otherwise
 * return -1 to indicate not implemented.
 */
#if defined(_MSC_VER)
static time_t my_timegm(struct tm *t) { return _mkgmtime(t); }
#else
#if defined(HAVE_TIMEGM)
static time_t my_timegm(struct tm *t) { return timegm(t); }
#else
static time_t my_timegm(struct tm *t) { (void)t; return (time_t)-1; }
#endif
#endif

/* Limited strptime accepting two formats used by zsync:
 *  "%a, %d %b %Y %H:%M:%S %z" and "%d %b %Y %H:%M:%S %z"
 * It ignores the timezone offset (%z) but parses the numeric fields and
 * fills struct tm. Returns non-NULL on success, NULL on parse failure.
 */
char *strptime(const char *s, const char *fmt, struct tm *tm) {
    (void)fmt; /* we only support the specific formats used */
    int day, year, hour, min, sec;
    char month[4] = {0};
    char tzstr[8] = {0};
    int matched = 0;
    struct tm tmp = {0};

    /* use file-scope my_timegm helper */

    /* Try with weekday prefix: "Tue, 25 Jul 2006 20:02:17 +0000" */
#if defined(_MSC_VER)
    matched = sscanf_s(s, "%*[^,], %d %3s %d %d:%d:%d %7s", &day, month, (unsigned)_countof(month), &year, &hour, &min, &sec, tzstr, (unsigned)_countof(tzstr));
#else
    matched = sscanf(s, "%*[^,], %d %3s %d %d:%d:%d %7s", &day, month, &year, &hour, &min, &sec, tzstr);
#endif
    if (matched >= 6) {
        int mon = month_str_to_int(month);
        if (mon < 0) return NULL;
        tmp.tm_mday = day;
        tmp.tm_mon = mon;
        tmp.tm_year = year - 1900;
        tmp.tm_hour = hour;
        tmp.tm_min = min;
        tmp.tm_sec = sec;
        tmp.tm_isdst = -1;

        /* If timezone string present parse and adjust to localtime representation */
        if (matched == 7 && tzstr[0] && (tzstr[0] == '+' || tzstr[0] == '-')) {
            int sign = (tzstr[0] == '-') ? -1 : 1;
            int tzh = 0, tzm = 0;
            if (sscanf(tzstr + 1, "%2d%2d", &tzh, &tzm) >= 1) {
                long offset = sign * (tzh * 3600 + tzm * 60);
                time_t epoch = my_timegm(&tmp);
                if (epoch == (time_t)-1) {
                    /* cannot compute timegm: fall back to naive behavior */
                    *tm = tmp;
                    return (char *)s + strlen(s);
                }
                /* The parsed tmp represents wall-clock time in the timezone
                 * given by the offset. Compute the absolute epoch then
                 * subtract the offset to get UTC epoch, and finally convert
                 * that epoch to local broken-down time (localtime) so the
                 * returned struct tm matches glibc behaviour where callers
                 * often call mktime() on the returned value. Using localtime
                 * here ensures mktime(&tm) produces the same epoch as the
                 * original timestamp. */
                epoch -= offset;
                /* Convert to local time in a thread-safe way */
#if defined(_MSC_VER)
                struct tm local;
                errno_t er = localtime_s(&local, &epoch);
                if (er != 0) return NULL;
                *tm = local;
#else
                struct tm localbuf;
                struct tm *localptr = localtime_r(&epoch, &localbuf);
                if (!localptr) return NULL;
                *tm = *localptr;
#endif
                return (char *)s + strlen(s);
            }
        }

        /* No timezone parsed or couldn't parse tz: return tm as local wall-time */
        *tm = tmp;
        return (char *)s + strlen(s);
    }

    /* Try without weekday: "25 Jul 2006 20:02:17 +0000" */
#if defined(_MSC_VER)
    matched = sscanf_s(s, "%d %3s %d %d:%d:%d %7s", &day, month, (unsigned)_countof(month), &year, &hour, &min, &sec, tzstr, (unsigned)_countof(tzstr));
#else
    matched = sscanf(s, "%d %3s %d %d:%d:%d %7s", &day, month, &year, &hour, &min, &sec, tzstr);
#endif
    if (matched >= 6) {
        int mon = month_str_to_int(month);
        if (mon < 0) return NULL;
        tmp.tm_mday = day;
        tmp.tm_mon = mon;
        tmp.tm_year = year - 1900;
        tmp.tm_hour = hour;
        tmp.tm_min = min;
        tmp.tm_sec = sec;
        tmp.tm_isdst = -1;

        if (matched == 7 && tzstr[0] && (tzstr[0] == '+' || tzstr[0] == '-')) {
            int sign = (tzstr[0] == '-') ? -1 : 1;
            int tzh = 0, tzm = 0;
            if (sscanf(tzstr + 1, "%2d%2d", &tzh, &tzm) >= 1) {
                long offset = sign * (tzh * 3600 + tzm * 60);
                time_t epoch = my_timegm(&tmp);
                if (epoch == (time_t)-1) {
                    *tm = tmp;
                    return (char *)s + strlen(s);
                }
                epoch -= offset;
#if defined(_MSC_VER)
                struct tm local;
                errno_t er = localtime_s(&local, &epoch);
                if (er != 0) return NULL;
                *tm = local;
#else
                struct tm localbuf;
                struct tm *localptr = localtime_r(&epoch, &localbuf);
                if (!localptr) return NULL;
                *tm = *localptr;
#endif
                return (char *)s + strlen(s);
            }
        }

        *tm = tmp;
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
