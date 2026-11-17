// Brazilian Portuguese UI localization of PS Vita-specific panels.
// Set via GENERAL > UI LANGUAGE; does not alter game save language.
#ifndef UI_LOCALE_H
#define UI_LOCALE_H
#include <stdbool.h>
bool UiLocale_IsPortuguese(void);
const char *UiLocale_Translate(const char *text);
#endif
