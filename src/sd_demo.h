/**
 ******************************************************************************
 * @file        sd_demo.h
 * @author      Dr. Livingston
 * @date        November 2026
 * @course      CPE 3600 - Embedded Systems Design, MSOE
 * @assignment  SD Studio - sample program
 * @brief       The bridge from app_freertos.c into the demo, the same idea
 *              as app.h in Lab 6. Call sd_demo_start() in place of
 *              app_start() to run the demo; put app_start() back for Lab 6.
 * @dependencies
 *              - SdDemo.cpp (C++)
 *
 * PROVIDED FILE - you should not need to modify this file.
 ******************************************************************************
 */

#ifndef SD_DEMO_H
#define SD_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Creates the SPI mutex and starts the demo task. Call once from
           MX_FREERTOS_Init(), USER CODE BEGIN RTOS_THREADS. */
void sd_demo_start(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_DEMO_H */
