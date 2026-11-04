#ifndef PASSWORD_HTML_H
#define PASSWORD_HTML_H

#define PASSWORD_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>Protected Link - c.micutu.com</title>\n" \
"</head>\n" \
"<body>\n" \
"  <main class=\"narrow centered\">\n" \
"    <section class=\"container text-center\" aria-labelledby=\"lockTitle\">\n" \
"      <h1 id=\"lockTitle\">Password Protected Link</h1>\n" \
"      <p class=\"subtitle\">Enter the password to unlock and continue to the destination URL.</p>\n" \
"      <form id=\"unlockForm\">\n" \
"        <div class=\"field\">\n" \
"          <label for=\"pwd\">Link Password</label>\n" \
"          <input type=\"password\" id=\"pwd\" placeholder=\"Enter password\" required autofocus>\n" \
"        </div>\n" \
"        <button type=\"submit\" class=\"primary\" id=\"btn\">Unlock &amp; Continue</button>\n" \
"      </form>\n" \
"      <p id=\"err\" class=\"error\" role=\"alert\" aria-live=\"polite\"></p>\n" \
"      <p class=\"hint\"><a href=\"/\">&larr; Back to c.micutu.com</a></p>\n" \
"    </section>\n" \
"  </main>\n" \
"  <script src=\"/assets/password.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#endif
