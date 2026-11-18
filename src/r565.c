/**
 ******************************************************************************
 * @file        r565.c
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - reading its files on the board
 * @brief       Helpers for the SD Studio file formats. See r565.h.
 *
 * PROVIDED FILE - you should not need to modify this file.
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "r565.h"

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

bool r565_parse(const uint8_t *head, uint32_t n, uint32_t fileSize, r565_info_t *info)
{
    if (n >= R565_HEADER_BYTES && head[0] == 'R' && head[1] == '5' &&
        head[2] == '6' && head[3] == '5') {
        uint16_t w = get16(head + 4);
        uint16_t h = get16(head + 6);
        if (w > 0 && h > 0 &&
            fileSize == R565_HEADER_BYTES + 2UL * w * h) {
            info->width = w;
            info->height = h;
            info->pixelOffset = R565_HEADER_BYTES;
            info->rowBytes = 2UL * w;
            return true;
        }
    }
    if (fileSize == R565_LAB6_BYTES) {
        info->width = R565_LAB6_W;
        info->height = R565_LAB6_H;
        info->pixelOffset = 0;
        info->rowBytes = 2UL * R565_LAB6_W;
        return true;
    }
    return false;
}

bool r565_glyph_path(const char *fontDir, uint8_t ch, char *out, uint32_t size)
{
    int k = snprintf(out, size, "%s/%03X.565", fontDir, (unsigned)ch);
    return k > 0 && (uint32_t)k < size;
}

/* The number after "key=" in text, or -1. Takes decimal or 0x hex. */
static long value(const char *text, uint32_t n, const char *key)
{
    size_t klen = strlen(key);
    uint32_t i = 0;

    while (i < n) {
        /* at the start of a line */
        if (i + klen < n && strncmp(&text[i], key, klen) == 0 && text[i + klen] == '=') {
            char num[16];
            uint32_t j = i + (uint32_t)klen + 1, m = 0;
            while (j < n && m < sizeof(num) - 1 && text[j] != '\r' && text[j] != '\n') {
                num[m++] = text[j++];
            }
            num[m] = '\0';
            return strtol(num, NULL, 0);
        }
        while (i < n && text[i] != '\n') {
            i++;
        }
        i++;
    }
    return -1;
}

bool r565_font_parse(const char *text, uint32_t n, r565_font_t *font)
{
    long h = value(text, n, "height");
    long b = value(text, n, "baseline");
    long f = value(text, n, "first");
    long l = value(text, n, "last");
    long fg = value(text, n, "fg");
    long bg = value(text, n, "bg");

    if (h <= 0) {
        return false;
    }
    font->height = (uint16_t)h;
    font->baseline = (b >= 0) ? (uint16_t)b : (uint16_t)h;
    font->first = (f >= 0) ? (uint8_t)f : 0x20;
    font->last = (l >= 0) ? (uint8_t)l : 0x7E;
    font->fg = (fg >= 0) ? (uint16_t)fg : 0xFFFF;
    font->bg = (bg >= 0) ? (uint16_t)bg : 0x0000;
    return true;
}
