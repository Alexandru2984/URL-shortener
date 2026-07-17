# Product and hardening roadmap

## Next security milestones

1. Deploy the committed network, API-key and password fixes using the rollout
   checklist; rotate the existing API key.
2. Replace the remaining inline browser assets with same-origin static assets
   served by the C binary, then remove `'unsafe-inline'` from CSP.
3. Add configurable analytics retention, a privacy notice and a safe aggregate
   migration/rotation procedure for the dedicated HMAC key.
4. Add a separate, scoped creator-token model so the global admin key is never
   used from the public shortening form.
5. Add alerting for failed health checks, restart loops, disk growth and
   repeated authentication/rate-limit failures.

## Product milestones

1. Paginated admin search/filtering, explicit link edit/disable actions and
   CSV export for administrators.
2. Link-level controls: max clicks, one-time links, scheduled activation,
   expiry editing and optional password change.
3. Privacy-preserving daily analytics, referrer categories and retention-aware
   aggregate reports.
4. A locally generated SVG QR endpoint implemented in C, avoiding third-party
   QR requests and the related URL disclosure.
5. Versioned API documentation, idempotency keys, scoped creator tokens,
   audit events and webhook delivery with signed payloads.
6. Import/export, bulk expiry operations, link tags and a lightweight public
   API client example in C.
