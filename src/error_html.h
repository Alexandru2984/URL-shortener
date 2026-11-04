#ifndef ERROR_HTML_H
#define ERROR_HTML_H

#define ERROR_HTML_TEMPLATE \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>%s</title>\n" \
"</head>\n" \
"<body>\n" \
"  <main class=\"narrow centered\">\n" \
"    <section class=\"container text-center\">\n" \
"      <div class=\"code-hero\">%d</div>\n" \
"      <h1>%s</h1>\n" \
"      <p class=\"subtitle\">%s</p>\n" \
"      <a class=\"btn primary-inline\" href=\"/\">&larr; Back to Shortener</a>\n" \
"    </section>\n" \
"  </main>\n" \
"</body>\n" \
"</html>"

#endif
