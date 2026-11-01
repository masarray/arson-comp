#!/usr/bin/env python3
"""Offline checks for the Cloudflare/GitHub Pages static download site."""
from __future__ import annotations

import hashlib
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlparse, unquote
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
RELEASE = "https://github.com/masarray/arson-comp/releases/download/v0.2.0-rc.1/"
DOWNLOADS = (
    "ArSonComp-0.2.0-Windows-x64-Setup.exe",
    "ArSonComp-0.2.0-macOS-Universal-AdHoc.pkg",
    "ArSonComp-0.2.0-Ubuntu-Debian-amd64.deb",
    "ArSonComp-0.2.0-Linux-x86_64-Portable.tar.gz",
    "ArSonComp-0.2.0-macOS-VST3-Universal.zip",
)
SCREENS = {
    "arsoncomp-full.png": (612, 580),
    "arsoncomp-sidechain.png": (580, 540),
    "arsoncomp-minimum.png": (580, 540),
    "arsoncomp-mini.png": (612, 320),
    # browser screenshot dimensions are permitted to differ; source is checked by hash.
    "arsoncomp-presets.png": None,
}


class Page(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.urls: list[tuple[str, str]] = []
        self.ids: set[str] = set()
        self.alt_count = 0
        self.images = 0
        self.has_title = False
        self.downloads: list[str] = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if a.get("id"):
            assert a["id"] not in self.ids, "duplicate id " + a["id"]
            self.ids.add(a["id"])
        if tag == "title":
            self.has_title = True
        if tag == "img":
            self.images += 1
            if not a.get("alt"):
                raise AssertionError("image missing alt text: " + str(a))
            self.alt_count += 1
            if not a.get("width") or not a.get("height"):
                raise AssertionError("image missing dimension: " + str(a))
        for field in ("href", "src"):
            if a.get(field):
                self.urls.append((field, a[field]))
        if a.get("data-download"):
            self.downloads.append(a["data-download"])


def check() -> None:
    page_file = ROOT / "index.html"
    page = Page()
    page.feed(page_file.read_text(encoding="utf-8"))
    assert page.has_title and page.images >= 2, "missing title or preview screenshots"
    assert len(page.ids) >= 12, "missing core sections/anchors"
    assert set(page.downloads) == {"windows", "macos", "linux"}, "missing OS CTA"
    assert set(["top", "features", "interface", "downloads", "getting-started"]).issubset(page.ids)
    for attr, value in page.urls:
        u = urlparse(value)
        if u.scheme in ("https", "http", "mailto"):
            if u.scheme == "http":
                raise AssertionError("insecure HTTP URL: " + value)
            continue
        if value.startswith("//"):
            raise AssertionError("scheme-relative URL forbidden: " + value)
        if value.startswith("#"):
            if value[1:] and value[1:] not in page.ids:
                raise AssertionError("unresolved anchor: " + value)
            continue
        assert not value.startswith("/"), "root-absolute asset breaks GitHub project Pages: " + value
        file_path = (ROOT / unquote(u.path)).resolve()
        if not file_path.is_relative_to(ROOT) or not file_path.is_file():
            raise AssertionError("unresolved local " + attr + ": " + value)

    text = page_file.read_text(encoding="utf-8")
    js = (ROOT / "app.js").read_text(encoding="utf-8")
    assert "v0.2.0-rc.1" in text and "v0.2.0-rc.1" in js, "release mismatch"
    assert "Release Candidate" in text and "not Apple notarized" in text, "honest RC/macOS messaging missing"
    for item in DOWNLOADS:
        assert RELEASE + item in text, "missing official release asset: " + item
    for item in DOWNLOADS[:2]:
        assert item in js, "OS-aware CTA differs from download list: " + item

    for filename, expected in SCREENS.items():
        local = ROOT / "assets" / filename
        original = REPO / "docs" / "images" / filename
        assert local.is_file() and original.is_file(), "missing real screenshot " + filename
        data = local.read_bytes()
        assert data[:8] == b"\x89PNG\r\n\x1a\n", "not PNG: " + filename
        w, h = struct.unpack(">II", data[16:24])
        if expected and (w, h) != expected:
            raise AssertionError(f"wrong dimensions {filename}: {(w, h)} != {expected}")
        if hashlib.sha256(data).digest() != hashlib.sha256(original.read_bytes()).digest():
            raise AssertionError("screenshot was modified: " + filename)
        print(f"Verified genuine image: {filename} {w}x{h}")

    # Production SEO and website privacy must be internally consistent.
    import xml.etree.ElementTree as ET
    canonical = "https://masarray.github.io/arson-comp/"
    assert '<link rel="canonical" href="' + canonical + '">' in text, "canonical URL missing"
    assert '<meta property="og:url" content="' + canonical + '">' in text, "OpenGraph origin drift"
    assert '<meta name="twitter:image"' in text, "social preview missing"
    assert 'href="./privacy.html"' in text and 'id="faq"' in text, "support/FAQ links missing"
    policy = ROOT / "privacy.html"
    assert policy.is_file(), "privacy statement missing"
    assert (ROOT / "styles.css").is_file()
    policy_page = Page()
    policy_page.feed(policy.read_text(encoding="utf-8"))
    assert policy_page.has_title, "privacy page missing title"
    assert 'does not embed advertising trackers' in policy.read_text(encoding="utf-8")
    sitemap = ROOT / "sitemap.xml"
    sitemap_root = ET.parse(sitemap).getroot()
    locations = {e.text for e in sitemap_root.iter() if e.tag.endswith("loc")}
    assert locations == {canonical, canonical + "privacy.html"}, "sitemap URL drift"
    robots = (ROOT / "robots.txt").read_text(encoding="utf-8")
    assert "Sitemap: " + canonical + "sitemap.xml" in robots, "robots/sitemap mismatch"

    assert not (ROOT / "CNAME").exists(), "unknown domain must not be guessed"
    assert (ROOT / "_headers").is_file(), "missing Cloudflare security headers"
    assert (ROOT / ".nojekyll").exists(), "missing GitHub Pages .nojekyll"
    for file in ("index.html", "styles.css", "app.js"):
        size = (ROOT / file).stat().st_size
        assert size < 100_000, f"oversized static payload: {file} ({size})"
    print("PASS: offline URLs, accessible UI structure, release integrity, screenshots and static host compatibility")


if __name__ == "__main__":
    try:
        check()
    except (AssertionError, OSError, ValueError) as exc:
        print("SITE VALIDATION FAILED:", exc, file=sys.stderr)
        raise SystemExit(1) from exc
