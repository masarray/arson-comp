# ArSonComp landing page — GitHub Pages and Cloudflare Pages

This is the production-ready, framework-free marketing/download site under site/.
There is no npm install, bundler, Google Fonts request, analytics SDK or web API.
It uses **real original M7 JUCE screenshots**, copied by their Git blob SHA from docs/images/ to site/assets/. The source release is **v0.2.0-rc.1** and must not be called stable before actual host/hardware gates.

## GitHub Pages — automatic deployment
1. Repository: Settings → Pages → Build and deployment → **Source: GitHub Actions**.
2. The workflow .github/workflows/pages.yml runs on pushes to main that change site/, and on manual dispatch; it validates files and publishes only site/.
3. Project URL: https://masarray.github.io/arson-comp/ (once Pages deployment succeeds).
4. A pull request runs light static validation only; it does not publish the draft. It does not trigger heavy audio engine tests.
5. Relative assets (./styles.css, ./assets/...) work at both /arson-comp/ and a custom-domain root. Do not put a Cloudflare custom domain into a GitHub Pages CNAME unless intentionally changing GitHub Pages itself.

## Cloudflare Pages — preferred primary site
1. Cloudflare dashboard → Workers & Pages → Create → **Pages** → Connect to Git.
2. Select GitHub masarray/arson-comp and set **production branch = main**.
3. Framework preset = **None**; Build command = **leave blank (no build)**; Build output directory = **site**; Root directory = repository root.
4. Deploy. A *.pages.dev preview/production URL is created; connect your own domain under Pages → Custom domains when the domain is chosen.
5. Cloudflare automatically previews PR branches and redeploys main after merges.
6. site/_headers ships security/cache headers for Cloudflare Pages. GitHub Pages ignores _headers; both hosts still use the same static assets.

Search indexing and social cards currently use the public GitHub Pages URL as the canonical origin. **When a verified Cloudflare custom domain becomes the primary public URL**, update the canonical and OpenGraph URLs in `site/index.html`, `site/privacy.html`, `site/sitemap.xml`, and `site/robots.txt` together, then verify both hosts. Do not guess a domain or configure redirects before the owner selects it.

Do not add a guessed CNAME or domain. Cloudflare Pages project/domain connection requires the user's Cloudflare account authorization and DNS control. The Git repository is ready, but it is not evidence that a Cloudflare deployment has been performed.

## Content maintenance
- Official release assets and names are pinned in site/index.html. The OS-aware main CTA in site/app.js uses the same Windows/macOS release files. **When a new public release is published, update the links in both files**, then run site/scripts/validate_site.py. This prevents silent downloads pointing to a nonexistent "latest" installer.
- Keep the Linux DEB and portable paths distinct; autodetection must not force Ubuntu DEB onto arbitrary Linux.
- v0.2.0-rc.1 macOS PKG is ad-hoc signed and not notarized. Do not remove this caveat.
- Keep all 5 actual product snapshots in site/assets, not AI renderings.
- No claims of zero *hardware* latency, zero xruns or parity with other commercial compressors.

## Lightweight tests
Run: python3 site/scripts/validate_site.py
Run: node --check site/app.js
No audio plugin compilation is needed for website-only commits.

## Deployment credentials and custom domains
GitHub Pages uses built-in GITHUB_TOKEN with pages:write/id-token:write. Cloudflare native Git integration uses the Cloudflare app connection (no API token stored in repo). Configure a primary custom domain via Cloudflare Pages dashboard later; do not invent DNS values in source.
