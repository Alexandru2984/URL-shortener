#ifndef QR_H
#define QR_H

#include <stddef.h>

// Generates a standards-compliant ISO/IEC 18004 QR Code as a self-contained SVG string.
// Caller must free() the returned string. Returns NULL on error or if text is too long.
char *qr_generate_svg(const char *text, int border_modules);

#endif
