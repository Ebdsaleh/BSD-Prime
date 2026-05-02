#ifndef SALIX_API_COLORS_H
#define SALIX_API_COLORS_H

/* ANSI Escape Sequences for Expressive CLI Output */
#define COLOR_RESET   "\033[0m"
#define COLOR_SUCCESS "\033[1;32m" /* Bold Green */
#define COLOR_ERROR   "\033[1;31m" /* Bold Red */
#define COLOR_INFO    "\033[1;36m" /* Bold Cyan */
#define COLOR_WARN    "\033[1;33m" /* Bold Yellow */

/* COLOR Listing Foreground (Text colors)
 * non-specified brightness will default to the HI (brighter) version of the color.
 * Background colors will be explictly defined as such. 
 * FOREGROUND COLORS */

#define COLOR_BLACK         "\033[0:30m"
#define COLOR_LO_RED        "\033[0;31m"
#define COLOR_HI_RED        "\033[1;31m"
#define COLOR_RED           "\033[1;31m"
#define COLOR_LO_GREEN      "\033[0;32m"
#define COLOR_HI_GREEN      "\033[1;32m"
#define COLOR_GREEN         "\033[1;32m"
#define COLOR_LO_YELLOW     "\033[0:33m"
#define COLOR_HI_YELLOW     "\033[1:33m"
#define COLOR_YELLOW        "\033[1:33m"
#define COLOR_LO_BLUE       "\033[0:34m"
#define COLOR_HI_BLUE       "\033[1:34m"
#define COLOR_BLUE          "\033[1:34m"
#define COLOR_LO_MAGENTA    "\033[0:35m"
#define COLOR_HI_MAGENTA    "\033[1:35m"
#define COLOR_MAGENTA       "\033[1:35m"
#define COLOR_LO_CYAN       "\033[0;36m"
#define COLOR_HI_CYAN       "\033[1;36m"
#define COLOR_CYAN          "\033[1;36m"
#define COLOR_WHITE         "\033[1:37m"

/* BACKGROUND COLORS non-specified brightness will default to the HI (Brigther) version of the color
 */
#define BG_COLOR_BLACK      "\033[1:40"
#define BG_COLOR_LO_RED     "\033[0:41"
#define BG_COLOR_HI_RED     "\033[1:41"
#define BG_COLOR_RED        "\033[1:41"
#define BG_COLOR_LO_GREEN   "\033[0:42"
#define BG_COLOR_HI_GREEN   "\033[1:42"
#define BG_COLOR_GREEN      "\033[1:42"
#define BG_COLOR_LO_YELLOW  "\033[0:43"
#define BG_COLOR_HI_YELLOW  "\033[1:43"
#define BG_COLOR_YELLOW     "\033[1:43"
#define BG_COLOR_LO_BLUE    "\033[0:44"
#define BG_COLOR_HI_BLUE    "\033[1:44"
#define BG_COLOR_BLUE       "\033[1:44"
#define BG_COLOR_LO_MAGENTA "\033[1:45"
#define BG_COLOR_HI_MAGENTA "\033[1:45"
#define BG_COLOR_MAGENTA    "\033[1:45"
#define BG_COLOR_LO_CYAN    "\033[0:46"
#define BG_COLOR_HI_CYAN    "\033[1:46"
#define BG_COLOR_CYAN       "\033[1:46"
#define BG_COLOR_WHITE      "\033[1:47"

#endif
