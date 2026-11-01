# Security policy

## Supported builds

ArSonComp is an early public Release Candidate, not a production-certified audio tool.

| Release line | Status |
| --- | --- |
| v0.2.0-rc.1 | Current publicly available release candidate; security fixes are best effort |
| Older development builds | No guaranteed maintenance |

## Reporting a security vulnerability

**Please do not disclose an exploitable issue in a public GitHub issue or discussion.**

If GitHub private vulnerability reporting is enabled for this repository, use [Report a vulnerability](https://github.com/masarray/arson-comp/security/advisories/new) on the Security tab. If it is not available, request a secure reporting route from the maintainer through a trusted private communication channel; do not send exploit details to a public issue. No security contact email has been published or assumed here.

Include the affected version/commit, operating system, reproducible impact, minimal proof-of-concept details, and a proposed mitigation where possible. Avoid including real user data, passwords or API tokens. Response timelines and bounties are **not guaranteed**.

## Scope and release limitations

Security reports may involve VST3/Standalone loading, malformed preset/state parsing, installer/update paths, privileges, dependencies or website distribution. The site is static and links to official release binaries. Distribution integrity can be verified with the [release SHA-256 checksums](https://github.com/masarray/arson-comp/releases/tag/v0.2.0-rc.1).

Avoid claiming a program is security-certified or physically glitch-free based solely on passing CI. Refer to [release status](docs/RELEASE-STATUS.md) for remaining validation work.
