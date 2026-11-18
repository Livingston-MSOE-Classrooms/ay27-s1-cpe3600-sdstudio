/**
 ******************************************************************************
 * @file        SdGraphics.cpp
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - drawing from the card
 * @brief       See SdGraphics.hpp.
 *
 * PROVIDED FILE - you should not need to modify this file.
 ******************************************************************************
 */

#include "main.h"               // HAL before any rtos/ header (Lab 6 gotcha)
#include "ff.h"
#include "ili9341.h"
#include "r565.h"
#include "SdGraphics.hpp"

namespace lab6 {

int16_t SdGraphics::drawImage(const char* path, int16_t x, int16_t y)
{
    FIL         f;
    UINT        got = 0;
    r565_info_t info;

    if (f_open(&f, path, FA_READ) != FR_OK) {
        return -1;
    }
    if (f_read(&f, mBuf, R565_HEADER_BYTES, &got) != FR_OK ||
        !r565_parse(mBuf, got, f_size(&f), &info)) {
        f_close(&f);
        return -1;
    }

    // the part of the image that's on the screen
    int16_t c0 = 0, r0 = 0;
    int16_t c1 = static_cast<int16_t>(info.width), r1 = static_cast<int16_t>(info.height);
    if (x < 0) {
        c0 = static_cast<int16_t>(-x);
    }
    if (y < 0) {
        r0 = static_cast<int16_t>(-y);
    }
    if (x + c1 > TFT_WIDTH) {
        c1 = static_cast<int16_t>(TFT_WIDTH - x);
    }
    if (y + r1 > TFT_HEIGHT) {
        r1 = static_cast<int16_t>(TFT_HEIGHT - y);
    }
    if (c0 >= c1 || r0 >= r1) {
        f_close(&f);
        return static_cast<int16_t>(info.width);      // all off the screen
    }

    const uint32_t visBytes = 2UL * static_cast<uint32_t>(c1 - c0);
    const bool     fullRows = (c0 == 0 && c1 == static_cast<int16_t>(info.width));
    uint16_t       rows = 1;
    if (fullRows && info.rowBytes <= kBufBytes) {
        rows = static_cast<uint16_t>(kBufBytes / info.rowBytes);
    }

    for (int16_t r = r0; r < r1; r = static_cast<int16_t>(r + rows)) {
        uint16_t n = rows;
        if (r + n > r1) {
            n = static_cast<uint16_t>(r1 - r);
        }
        uint32_t want = visBytes;
        if (fullRows) {
            want = n * info.rowBytes;
        }
        if (want > kBufBytes) {
            want = kBufBytes;   // one row wider than the buffer: clipped
        }
        // no SpiLock here: f_read() talks to the card, and SdCard locks
        if (f_lseek(&f, info.pixelOffset + static_cast<uint32_t>(r) * info.rowBytes + 2UL * c0) != FR_OK ||
            f_read(&f, mBuf, want, &got) != FR_OK || got != want) {
            break;
        }
        SpiLock lock(mBus);
        TFT_SetWindow(static_cast<uint16_t>(x + c0), static_cast<uint16_t>(y + r),
                      static_cast<uint16_t>(x + c0 + want / (2U * n) - 1),
                      static_cast<uint16_t>(y + r + n - 1));
        TFT_WriteData(mBuf, static_cast<uint16_t>(want));
    }
    f_close(&f);
    return static_cast<int16_t>(info.width);
}

bool SdGraphics::imageSize(const char* path, uint16_t* w, uint16_t* h)
{
    FIL         f;
    UINT        got = 0;
    r565_info_t info;
    bool        ok = false;

    if (f_open(&f, path, FA_READ) != FR_OK) {
        return false;
    }
    if (f_read(&f, mBuf, R565_HEADER_BYTES, &got) == FR_OK &&
        r565_parse(mBuf, got, f_size(&f), &info)) {
        *w = info.width;
        *h = info.height;
        ok = true;
    }
    f_close(&f);
    return ok;
}

int16_t SdGraphics::drawText(const char* fontDir, const char* text, int16_t x, int16_t y)
{
    char path[48];

    for (const char* p = text; *p != '\0'; p++) {
        if (!r565_glyph_path(fontDir, static_cast<uint8_t>(*p), path, sizeof(path))) {
            continue;
        }
        int16_t w = drawImage(path, x, y);
        if (w > 0) {
            x = static_cast<int16_t>(x + w);
        }
    }
    return x;
}

int16_t SdGraphics::textWidth(const char* fontDir, const char* text)
{
    char        path[48];
    int16_t     w = 0;
    FIL         f;
    UINT        got = 0;
    r565_info_t info;

    for (const char* p = text; *p != '\0'; p++) {
        if (!r565_glyph_path(fontDir, static_cast<uint8_t>(*p), path, sizeof(path)) ||
            f_open(&f, path, FA_READ) != FR_OK) {
            continue;
        }
        if (f_read(&f, mBuf, R565_HEADER_BYTES, &got) == FR_OK &&
            r565_parse(mBuf, got, f_size(&f), &info)) {
            w = static_cast<int16_t>(w + info.width);
        }
        f_close(&f);
    }
    return w;
}

}  // namespace lab6
