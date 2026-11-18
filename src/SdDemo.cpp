/**
 ******************************************************************************
 * @file        SdDemo.cpp
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - sample program
 * @brief       A picture from the card with text under it, on top of the
 *              Lab 6 drivers. One task: it mounts the card, reads the two
 *              fonts' FONT.TXT files, paints the screen the fonts'
 *              background color, draws DEMO/PHOTO.565 centered at the top,
 *              writes a title and two lines under it, then updates an
 *              uptime line at the bottom once a second.
 *
 *              The files it wants on the card (sdcard/ in this package):
 *                  DEMO/PHOTO.565       a 240 x 160 picture
 *                  FONTS/SANS20B/...    DejaVu Sans 20 bold, orange on navy
 *                  FONTS/SANS14/...     DejaVu Sans 14, white on navy
 *              Change the names below to use your own.
 * @dependencies
 *              - sd_demo.h, SdGraphics, r565, and from Lab 6: SpiBus,
 *                SdCard, SdDisk, FatFs, ili9341, rtos/Task
 ******************************************************************************
 */

#include <cstdio>
#include <cstring>
#include "main.h"               // HAL before any rtos/ header (Lab 6 gotcha)
#include "spi.h"
#include "ff.h"
#include "ili9341.h"
#include "Task.hpp"
#include "SpiBus.hpp"
#include "SdCard.hpp"
#include "SdDisk.hpp"
#include "SdGraphics.hpp"
#include "r565.h"
#include "sd_demo.h"

using namespace lab6;

namespace {

/* What to draw. 8.3 names, folders separated by '/'. */
constexpr const char* kPhoto     = "DEMO/PHOTO.565";
constexpr const char* kTitleFont = "FONTS/SANS20B";
constexpr const char* kTextFont  = "FONTS/SANS14";
constexpr const char* kTitle     = "SD Studio";
constexpr const char* kLine1     = "A picture and text,";
constexpr const char* kLine2     = "all from the SD card.";

constexpr int16_t kMargin = 8;          // pixels around things

/* The objects, all static like Lab 6 */
SpiBus     s_bus(&hspi1);
SdCard     s_card(s_bus, SD_CS_GPIO_Port, SD_CS_Pin, TFT_CS_GPIO_Port, TFT_CS_Pin);
SdGraphics s_gfx(s_bus);
FATFS      s_fs;

constexpr uint32_t kDemoStackWords = 1024;
uint32_t sDemoStack[kDemoStackWords];

/* Reads a font folder's FONT.TXT. */
bool loadFont(const char* dir, r565_font_t* font)
{
    char path[40];
    char text[300];
    FIL  f;
    UINT got = 0;

    std::snprintf(path, sizeof(path), "%s/FONT.TXT", dir);
    if (f_open(&f, path, FA_READ) != FR_OK) {
        return false;
    }
    FRESULT fr = f_read(&f, text, sizeof(text), &got);
    f_close(&f);
    return fr == FR_OK && r565_font_parse(text, got, font);
}

/* A filled rectangle, under the lock like every TFT_*() call. */
void fill(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    SpiLock lock(s_bus);
    TFT_FillRect(x, y, w, h, color);
}

/* Text centered across the screen; returns the y under it. */
int16_t centered(const char* font, uint16_t height, const char* text, int16_t y)
{
    int16_t w = s_gfx.textWidth(font, text);
    s_gfx.drawText(font, text, static_cast<int16_t>((TFT_WIDTH - w) / 2), y);
    return static_cast<int16_t>(y + height);
}

/* Something went wrong: a red screen and a message on the console. */
void fail(const char* what)
{
    std::printf("demo: %s\r\n", what);
    fill(0, 0, TFT_WIDTH, TFT_HEIGHT, RGB565(120, 0, 0));
    for (;;) {
        osDelay(1000);
    }
}

class DemoTask : public rtos::Task {
public:
    DemoTask()
        : Task("demo", osPriorityNormal, sDemoStack, kDemoStackWords)
    {
    }

protected:
    void run() override
    {
        r565_font_t title, text;
        uint16_t    pw = 0, ph = 0;

        {
            SpiLock lock(s_bus);
            TFT_Init(&hspi1);
            TFT_FillScreen(TFT_BLACK);
        }

        attachSdDisk(s_card);
        if (f_mount(&s_fs, "", 1) != FR_OK) {
            fail("no card");
        }
        if (!loadFont(kTitleFont, &title) || !loadFont(kTextFont, &text)) {
            fail("a font folder is missing (FONT.TXT not found)");
        }
        if (!s_gfx.imageSize(kPhoto, &pw, &ph)) {
            fail("the picture is missing or isn't a .565 image");
        }
        std::printf("demo: picture %u x %u, fonts %u and %u px tall\r\n",
                    pw, ph, title.height, text.height);

        // The glyphs carry their own background, so the screen gets the
        // same color and the text sits on it without a box around it.
        fill(0, 0, TFT_WIDTH, TFT_HEIGHT, text.bg);

        uint32_t t0 = HAL_GetTick();
        s_gfx.drawImage(kPhoto, static_cast<int16_t>((TFT_WIDTH - pw) / 2), kMargin);
        uint32_t t1 = HAL_GetTick();

        int16_t y = static_cast<int16_t>(kMargin + ph + kMargin);
        y = centered(kTitleFont, title.height, kTitle, y);
        y = static_cast<int16_t>(y + 4);
        y = centered(kTextFont, text.height, kLine1, y);
        centered(kTextFont, text.height, kLine2, y);
        uint32_t t2 = HAL_GetTick();
        std::printf("demo: picture %lu ms, text %lu ms\r\n",
                    static_cast<unsigned long>(t1 - t0), static_cast<unsigned long>(t2 - t1));

        // A line that changes: blank the old one first. A shorter string
        // would leave the end of the longer one behind otherwise.
        const int16_t upY = static_cast<int16_t>(TFT_HEIGHT - kMargin - text.height);
        char line[32];
        for (uint32_t s = 0; ; s++) {
            std::snprintf(line, sizeof(line), "Up %lu s", static_cast<unsigned long>(s));
            fill(0, upY, TFT_WIDTH, static_cast<int16_t>(text.height), text.bg);
            centered(kTextFont, text.height, line, upY);
            osDelay(1000);
        }
    }
};

DemoTask s_demoTask;

}  // namespace

extern "C" void sd_demo_start(void)
{
    bool ok = s_bus.begin();
    ok = s_demoTask.start() && ok;
    if (!ok) {
        std::printf("sd_demo_start: an RTOS object didn't create\r\n");
    }
}
