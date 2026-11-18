# SD Studio: pictures and text from the SD card

Lab 6 put full-screen pictures on the display straight from the card. SD Studio takes that the rest of the way: pictures of any size, anywhere on the screen, and text in any font your laptop has, in any size and color. You make the files on your laptop, put them on the card, and the board draws them with the same SpiBus, SdCard and FatFs you used in Lab 6.

There's no new driver to learn here. `SdGraphics::drawImage()` is your Lab 6 `load()` with the size read from the file instead of fixed at 240 × 272, and text is just a row of small pictures, one per character.

## What's in here

| File | What it is | Where it goes |
|---|---|---|
| `sd_studio.html` | Makes images and fonts and writes them to the card. Chrome or Edge. | Stays here |
| `SDWriter.bin` | SD Writer 2.0. Same idea as Lab 6, now with folders and fonts. | Dragged onto the board when you load the card |
| `sdcard/DEMO/`, `sdcard/FONTS/` | The demo's picture and two fonts | On your card |
| `src/r565.h`, `r565.c` | The file formats, and helpers to read them | `Core/Inc`, `Core/Src` |
| `src/SdGraphics.hpp`, `.cpp` | Draws an image or a line of text from the card | `Core/Inc`, `Core/Src` |
| `src/SdDemo.cpp`, `sd_demo.h` | The sample program | `Core/Src`, `Core/Inc` |
| `demo_screen.png` | What the demo should look like | |

`r565` and `SdGraphics` are provided files; you shouldn't need to change them. `SdDemo.cpp` is yours to take apart.

## Running the demo

You need your working Lab 6 project. The demo uses its drivers, its FatFs and its rtos classes, and swaps one line so the demo task starts instead of the Lab 6 tasks. Work on a branch so your graded Lab 6 commits stay as they are:

```
git checkout -b sd-demo
```

### 1. Put the files on the card

Your Lab 6 images stay where they are; this adds two folders next to them.

1. Card in the shield, USB cable in. Drag `SDWriter.bin` onto the **NOD_G474RE** drive. The screen goes black with a gray bar; the square at the right end is green if it found the card.
2. Open `sd_studio.html` (double-click it), click **Connect to the board** and pick the STLink COM port. Close any serial terminal that has it open first.
3. Go to the **Card** tab. Open the `sdcard` folder in File Explorer, select `DEMO` and `FONTS` together, and drag both onto the drop area. (Or use **Choose a folder…**, once for `DEMO` and once for `FONTS`.) You'll see each file land on the board's screen as it's written. The fonts are 192 small files, so give it a minute.
4. In the file list, open `FONTS` and click **Try text** next to `SANS20B`. That takes you down to **Text on the board's screen** with the font picked; type something and click **Draw it**. If you see your text on the board, the card is ready.

Drag the two folders inside `sdcard`, not `sdcard` itself. The demo looks for `DEMO` and `FONTS` at the top of the card, and an extra `SDCARD` folder above them gets you a red screen and "a font folder is missing" on the console.

If you have a card reader, you can skip the board entirely and copy `DEMO` and `FONTS` onto the card from File Explorer.

### 2. Add the code

1. Move the six files in `src/` into `Core/Inc` and `Core/Src` per the table above.
2. In `Core/Src/app_freertos.c`, two changes, each inside its USER CODE block:

| USER CODE block | Change |
|---|---|
| `Includes` | add `#include "sd_demo.h"` under `#include "app.h"` |
| `RTOS_THREADS` | `app_start();` becomes `sd_demo_start();` |

That's it. `app.cpp` still builds, but nothing starts its tasks. To go back to the Lab 6 viewer, change that one line back.

3. Build, then flash from CubeIDE (this replaces the SD Writer on the board).

### 3. What you should see

The picture across the top, "SD Studio" in orange under it, two lines of white text, and an "Up 12 s" line at the bottom counting once a second. `demo_screen.png` is a capture of it. On the console:

```
demo: picture 240 x 160, fonts 24 and 16 px tall
demo: picture 160 ms, text 520 ms
```

Your times will be a little different. A red screen means something wasn't found, and the console says what.

## How the demo works

`SdDemo.cpp` is one task, built like the Lab 6 tasks: a class derived from `rtos::Task`, a static stack, and `sd_demo_start()` to create the SPI mutex and start it. Its `run()` goes through these steps in order:

1. `TFT_Init()` under a SpiLock, same as your Lab 6 display task.
2. `attachSdDisk()` and `f_mount()`, same as `ImageLoader::mount()`.
3. Reads `FONT.TXT` from both font folders with `r565_font_parse()`, for each font's height and background color.
4. Fills the screen with the font's background color. The glyphs have their background baked in, so this is what keeps the text from sitting in navy boxes on a black screen.
5. `imageSize()` and then `drawImage()` to center the picture at the top.
6. `textWidth()` and then `drawText()` to center each line under it, moving down one font height per line.
7. Once a second, blanks the bottom line with `TFT_FillRect()` and draws the new uptime. Without the blanking it would look fine for a while, since a longer string covers the old one. A shorter one leaves the ends of the old one on the screen.

One thing to notice in the timing: the text takes longer than the picture, even though it's far fewer pixels. Every character is its own file, and `centered()` opens each one twice (once to measure, once to draw). Opening a file costs a directory search on the card. If your project draws a lot of text, read the glyph widths once at start-up and keep them in an array.

## The files

**Images (`.565`).** An 8-byte header, then the pixels, RGB565 high byte first, row by row from the top left. Same byte order as Lab 6, so a row goes from the card to `TFT_WriteData()` untouched.

```c
typedef struct {
    char     magic[4];   // 'R' '5' '6' '5'
    uint16_t width;      // little-endian, like the Cortex-M4
    uint16_t height;
} r565_header_t;         // then width * height * 2 bytes of pixels
```

A file with no header that's exactly 130,560 bytes is a Lab 6 image, and `drawImage()` takes those too.

**Fonts.** A folder like `FONTS/SANS14` with one image per character, named by the character's code in hex: `041.565` is 'A' (0x41), `020.565` is the space. Every glyph is the font's full height, and its width is how far to move right for the next one. `FONT.TXT` holds the height, baseline, colors and character range as `key=value` lines.

**Names.** The card uses 8.3 names: up to 8 letters, digits, `_` or `-`, then a dot and up to 3 more. Folders follow the same rule. `sd_studio.html` makes every name fit.

## Making your own

On the **Images** tab, drop a picture, pick a size (full screen, the Lab 6 area, its own size, or anything you type in) and how to fit it: crop, pad with a color, fit inside, or stretch. Transparent PNG pixels take the background color, which is handy for icons. Leave dithering on for photos; turn it off for flat-color graphics.

On the **Fonts** tab, pick any font on your laptop (or load a .ttf), list the sizes you want (`12, 16, 24` makes three folders), and set bold, italic, the text and background colors, and which characters to include. A digits-only font for a big clock readout is a few dozen files instead of 95. The preview shows your sample text exactly the way the board will lay it out.

Then either **Write to the card** through the board, **Download .zip** to copy with a card reader, or **Save to a folder…** to write straight onto a card in a reader. Change the names at the top of `SdDemo.cpp` to use your files.

## Using it in your own project

`SdGraphics` follows the Lab 6 SPI rule: it calls `f_read()` with no lock held (SdCard locks for itself) and holds a SpiLock only while it sends rows to the display. So call it from a task that isn't holding a SpiLock, the same as `load()`. A few other things to keep in mind:

- Mount the card once. If your project also runs the Lab 6 `ImageLoader`, it has already called `f_mount()`; don't mount again with a second `FATFS`.
- One `SdGraphics` object per task. It has a 2 KB buffer inside, and two tasks sharing it would take turns overwriting each other's rows.
- Give the drawing task at least 512 words of stack. `drawImage()` puts a FatFs `FIL` (about 550 bytes) on it. The demo uses 1024.
- Anything off the screen is clipped, so a picture can slide in from the edge.
