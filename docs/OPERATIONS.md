# Operations and safe rollout

The application binds to `127.0.0.1` by default. NGINX is the only public edge;
Cloudflare should proxy to that edge and NGINX must have its Cloudflare real-IP
configuration enabled before applying request limits.

## Configuration

Use an API-key file in production, not an `Environment=API_KEY=...` line in a
systemd unit. The application accepts printable ASCII keys up to 1023 bytes and
rejects files that are world-readable or writable by group/others.

```sh
sudo install -d -m 0750 -o root -g shortener /etc/shortener
sudo install -m 0640 -o root -g shortener /dev/null /etc/shortener/api_key
sudo install -m 0640 -o root -g shortener /dev/null /etc/shortener/analytics_hmac_key
sudoedit /etc/shortener/api_key
sudoedit /etc/shortener/analytics_hmac_key
sudo install -m 0640 -o root -g shortener deploy/systemd/shortener.env.example /etc/shortener/shortener.env
```

Create a dedicated `shortener` service user and install the binary at
`/opt/shortener/bin/shortener`. The unit template keeps mutable SQLite data in
`/var/lib/shortener`, outside the release directory.

For the current checkout-based deployment, use
`deploy/systemd/shortener.in-place.service` first. It removes secrets from the
unit while retaining the existing user and paths; move to the dedicated-service
template on the next release migration.

## Rollout checklist

1. Run `make check && make sanitize && make` from the release checkout.
2. Make a consistent SQLite backup before restarting; use SQLite's backup
   command, not a raw file copy while WAL is active.
3. Install the systemd and NGINX templates. Install
   `deploy/nginx/shortener-rate-limit.conf` in NGINX's `http` context and
   `deploy/nginx/shortener-proxy.conf` as `/etc/nginx/snippets/shortener-proxy.conf`.
4. Run `sudo nginx -t` and `sudo systemctl daemon-reload` before any reload.
5. Restart `shortener`, then validate both `http://127.0.0.1:8086/health` and
   `https://c.micutu.com/health`.
6. Confirm the service listens only on loopback with `ss -lntp` and verify
   there is exactly one value for each security response header.

The API key previously lived in a readable unit file. Rotate it during the
rollout, then update every legitimate client key once. Do not place the new key
in a URL, shell history, Git, or frontend storage.

`ANALYTICS_HMAC_KEY_FILE` must be a separate, stable random secret. It turns a
visitor address into an irreversible keyed identifier before storage, allowing
the unique-visitor counter to work without retaining raw addresses. If it is
missing, visits are counted but unique-visitor counts are intentionally omitted.

## Backups and recovery

Keep encrypted, access-controlled backups and test restoration periodically.
For this service, back up the SQLite database with `sqlite3`'s `.backup`
operation and retain enough history to recover accidental link deletion. The
database is intentionally excluded from Git.
