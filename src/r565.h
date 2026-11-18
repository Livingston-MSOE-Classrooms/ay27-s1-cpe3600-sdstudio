/**
 ******************************************************************************
 * @file        r565.h
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - reading its files on the board
 * @brief       The file formats SD Studio writes, and small helpers for
 *              them. Nothing here touches the card or the display: open the
 *              file with FatFs, hand the first 8 bytes to r565_parse(), and
 *              draw the pixels the way you drew Lab 6 images.
 *
 *              Image (.565): an 8-byte header, then width * height pixels,
 *              RGB565, two bytes each, HIGH byte first, row by row from the
 *              top left. That's the order the ILI9341 wants after RAMWR, so
 *              rows go from the card to TFT_WriteData() untouched once the
 *              window is the image's rectangle.
 *
 *                  offset 0  'R' '5' '6' '5'
 *                  offset 4  width   (uint16, little-endian)
 *                  offset 6  height  (uint16, little-endian)
 *                  offset 8  pixels
 *
 *              A file with no header that's exactly 130,560 bytes is a Lab 6
 *              image, 240 x 272.
 *
 *              Font: a folder, e.g. FONTS/ARIAL16, with one image per
 *              character named by its code in hex (041.565 is 'A', 020.565
 *              the space) and FONT.TXT (key=value lines: height, baseline,
 *              first, last, fg, bg, ...). Every glyph is the same height;
 *              its width is how far to move right for the next one. The
 *              colors are baked in.
 *
 * PROVIDED FILE - you should not need to modify this file.
 ******************************************************************************
 */

#ifndef R565_H
#define R565_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define R565_HEADER_BYTES   8U
#define R565_LAB6_W         240U
#define R565_LAB6_H         272U
#define R565_LAB6_BYTES     (R565_LAB6_W * R565_LAB6_H * 2U)   /* 130,560 */

/** @brief What r565_parse() found out about a file. */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t pixelOffset;   /**< where the pixels start: 8, or 0 for Lab 6 */
    uint32_t rowBytes;      /**< width * 2 */
} r565_info_t;

/**
 * @brief  Reads the start of an image file.
 * @param  head      the first bytes of the file (8 is enough)
 * @param  n         how many bytes are in head
 * @param  fileSize  f_size() of the file, to check it's all there
 * @param  info      filled in on success
 * @return true if it's an R565 or a Lab 6 image of the right size
 */
bool r565_parse(const uint8_t *head, uint32_t n, uint32_t fileSize, r565_info_t *info);

/**
 * @brief  Builds the file name for one character of a font:
 *         r565_glyph_path("FONTS/ARIAL16", 'A', buf, size) -> "FONTS/ARIAL16/041.565"
 * @return false if it didn't fit in out
 */
bool r565_glyph_path(const char *fontDir, uint8_t ch, char *out, uint32_t size);

/** @brief What FONT.TXT says (the parts you're likely to need). */
typedef struct {
    uint16_t height;        /**< every glyph's height, pixels */
    uint16_t baseline;      /**< rows from the top to the baseline */
    uint8_t  first, last;   /**< character codes in the font */
    uint16_t fg, bg;        /**< RGB565 colors the glyphs were made with */
} r565_font_t;

/**
 * @brief  Parses the text of a FONT.TXT (read it into a buffer first;
 *         it's about 250 bytes). Unknown keys are ignored.
 * @return true if it had a height
 */
bool r565_font_parse(const char *text, uint32_t n, r565_font_t *font);

#ifdef __cplusplus
}
#endif

#endif /* R565_H */
