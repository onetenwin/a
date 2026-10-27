/* crc32.h - simple CRC-32 (IEEE 802.3) for application-level error detection.
   Message format on the wire:  payload|XXXXXXXX   (XXXXXXXX = CRC in hex) */
#ifndef CRC32_H
#define CRC32_H
#include <stdio.h>
#include <string.h>
static unsigned int crc32_calc(const unsigned char *d, size_t n) {
    unsigned int c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}
static int crc_pack(char *out, size_t sz, const char *payload) {
    return snprintf(out, sz, "%s|%08X", payload, crc32_calc((const unsigned char *)payload, strlen(payload)));
}
/* returns 1 if CRC OK (and strips the CRC field), 0 otherwise */
static int crc_unpack(char *buf) {
    char *bar = strrchr(buf, '|'); unsigned int rx;
    if (!bar || sscanf(bar + 1, "%x", &rx) != 1) return 0;
    *bar = 0;
    return rx == crc32_calc((const unsigned char *)buf, strlen(buf));
}
#endif
