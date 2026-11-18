/**
 ******************************************************************************
 * @file        SdGraphics.hpp
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - drawing from the card
 * @brief       Draws SD Studio images and fonts from the card onto the
 *              ILI9341, on top of the Lab 6 drivers. Same rule as Lab 6:
 *              f_read() with no SpiLock held (SdCard locks for itself), then
 *              a SpiLock just long enough to send the rows. drawImage() is
 *              your Lab 6 load() grown up: any size, anywhere on the screen,
 *              clipped at the edges.
 *
 *              The card must already be mounted (f_mount) by the caller.
 *              One object, one task: the 2 KB buffer inside isn't shared
 *              safely between tasks.
 * @dependencies
 *              - r565.h/.c (SD Studio reader), FatFs, SpiBus, ili9341
 *
 * PROVIDED FILE - you should not need to modify this file.
 ******************************************************************************
 */

#ifndef SD_GRAPHICS_HPP
#define SD_GRAPHICS_HPP

#include <cstdint>
#include "SpiBus.hpp"

namespace lab6 {

class SdGraphics {
public:
    static constexpr uint32_t kBufBytes = 2048;   ///< rows read per f_read()

    explicit SdGraphics(SpiBus& bus) : mBus(bus), mBuf() {}

    /// @brief Draws a .565 image (R565 or Lab 6) with its top left at x, y,
    ///        clipped to the screen.
    /// @return its width, or -1 if the file isn't there or isn't an image
    int16_t drawImage(const char* path, int16_t x, int16_t y);

    /// @brief Reads an image's width and height without drawing it.
    /// @return false if the file isn't there or isn't an image
    bool imageSize(const char* path, uint16_t* w, uint16_t* h);

    /// @brief Draws text with a font folder ("FONTS/ARIAL16"), left to
    ///        right from x, y. Characters the font doesn't have are skipped.
    /// @return the x just past the last character
    int16_t drawText(const char* fontDir, const char* text, int16_t x, int16_t y);

    /// @brief How wide text would be, without drawing it (opens each glyph).
    int16_t textWidth(const char* fontDir, const char* text);

private:
    SpiBus& mBus;
    uint8_t mBuf[kBufBytes];
};

}  // namespace lab6

#endif /* SD_GRAPHICS_HPP */
