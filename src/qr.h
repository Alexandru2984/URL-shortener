#ifndef QR_H
#define QR_H

#include <stddef.h>

// Generates a standards-compliant ISO/IEC 18004 QR Code as a self-contained SVG string.
// Caller must free() the returned string. Returns NULL on error or if text is too long.
char *qr_generate_svg(const char *text, int border_modules);

// Generates a themed SVG QR Code using validated #RRGGBB foreground/background colors.
char *qr_generate_svg_themed(const char *text, int border_modules,
                             const char *fg_hex, const char *bg_hex);

#endif
