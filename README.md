# CivicFix security-hardening update

This update is based on the project files supplied in ChatGPT. It does not claim that Google Safe Browsing has cleared the hosted domain; only Google can remove that warning after reviewing the live site.

## Changes included
- Public registration creates citizen accounts only. Existing engineer accounts remain usable. Do not set `ALLOW_ENGINEER_SIGNUP=1` on a public site.
- New passwords must be at least 10 characters; existing accounts are not modified.
- Removed the predictable Flask secret-key fallback in Render: startup fails if `SECRET_KEY` is missing. Local development generates a temporary random key.
- Added safer session-cookie settings and standard security headers.
- Added a privacy notice describing account data, complaint content, optional browser location, photos, ESP32 telemetry, and Google Maps links.
- Fixed the malformed/duplicated notification script placement in `engineer_dashboard.html`.
- Excluded databases, environment files, bytecode, and ZIPs from version control.

## Deploy safely
1. Back up your current database and uploaded photos first. This bundle intentionally does not include `complaints.db` or user data.
2. Review the diff, then copy these project files into your existing GitHub repository and commit/push. Do not delete the Render service or its database. If your repository currently tracks `static.zip` or `__pycache__`, remove those generated artifacts from GitHub; the CSS and JavaScript should be committed as normal files under `static/`.
3. In Render Environment, verify `SECRET_KEY` exists and is a strong private value. Do not paste it into GitHub or chat. `render.yaml` defines a generated secret for new Blueprint-managed services; do not rotate an existing production secret without planning for user sessions.
4. Keep `ALLOW_ENGINEER_SIGNUP=0`. Existing engineer accounts are not removed by this code change.
5. Confirm the deployment is healthy at `/healthz`, then test citizen login and complaint tracking.

## Google Safe Browsing warning
A code change alone cannot guarantee removal of the Chrome `Dangerous site` interstitial. After reviewing and cleaning the live site:
1. Verify `civicfix-smart-monitor-1.onrender.com` in [Google Search Console](https://search.google.com/search-console/).
2. Open **Security issues** and inspect any example URLs/categories.
3. Fix every listed issue and request a security review. Google says review/propagation may take days or longer.
4. Check the domain in [Google Safe Browsing Site Status](https://transparencyreport.google.com/safe-browsing/search?url=civicfix-smart-monitor-1.onrender.com).

## Important security notes
- Do not disable Chrome Safe Browsing or ask citizens to bypass the warning.
- The project should receive a proper security review before production use. This patch does not add CSRF protection or a persistent database migration.
- Never commit Wi-Fi passwords, device tokens, `SECRET_KEY`, user databases, or uploaded citizen photos. The included firmware intentionally uses placeholders. Because the files shared for review contained credential-like values, assume those values are exposed: change the Wi-Fi password and rotate the ESP32 token in Render, then update the ESP32 locally. Removing a secret from the latest commit does not erase it from Git history; rotate it even if you remove old commits.
