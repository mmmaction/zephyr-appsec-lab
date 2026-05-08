# zephyr-appsec-lab

<!-- CI/CD -->
![Pipeline](https://github.com/mmmaction/zephyr-appsec-lab/actions/workflows/pipeline.yml/badge.svg)
<!-- Coverage -->
[![codecov](https://codecov.io/gh/mmmaction/zephyr-appsec-lab/branch/main/graph/badge.svg)](https://codecov.io/gh/mmmaction/zephyr-appsec-lab)
<!-- Security -->
[![Gitleaks](https://img.shields.io/badge/security-gitleaks-blue)](https://github.com/gitleaks/gitleaks)
[![SBOM](https://img.shields.io/badge/SBOM-CycloneDX-blue)](https://cyclonedx.org/)
<!-- SDK -->
[![Zephyr RTOS](https://img.shields.io/badge/Zephyr%20RTOS-v3.2.0-1a1a2e)](https://zephyrproject.org)
<!-- License -->
[![License](https://img.shields.io/github/license/mmmaction/zephyr-appsec-lab)](LICENSE)

The embedded counterpart to [mobile-appsec-lab](https://github.com/mmmaction/mobile-appsec-lab) — a reference implementation applying the same CI/CD security pipeline to Zephyr RTOS / nRF Connect SDK firmware. The repo contains a minimal hello-world app (`hello_app`) targeting the **u-blox NORA-B106 (nRF5340)** and a GitHub Actions pipeline demonstrating practical security measures for embedded firmware development.

---

## Purpose

- Demonstrate which security tools fit where in an embedded firmware pipeline
- Show how an SBOM (software) and HBOM (hardware) flow from build to verification to archival
- Serve as a reference aligned with the **CRA Annex I** requirement for SBOM delivery alongside software products

---

## Software Bill of Materials (SBOM)

| Component | Version | SBOM-relevant |
|---|---|---|
| Zephyr RTOS | v3.2.0 | ✅ |
| Zephyr SDK (ARM toolchain) | v0.15.1 | ✅ |

SBOM is generated automatically in the Build stage using **`west spdx`** (understands the Zephyr CMake/west build graph) and converted to CycloneDX JSON via **syft** for downstream tooling (DependencyTrack, Trivy).

> **Known gap (Zephyr v3.2.x):** `west spdx` in v3.2.x captures file hashes only — no component names, no versions, no license metadata. CVE matching via SBOM is not possible. Documented in `sbom-gaps.md` (archived with each build). **Mitigation:** `grype` scans the compiled `zephyr.elf` binary directly. Upgrade to Zephyr v3.4+ for complete SPDX output.

### SBOM Tool Evaluation

| Tool | Understands Zephyr? | Output | Verdict |
|---|---|---|---|
| `trivy fs` | ❌ No | Empty SBOM | Does not understand west/CMake |
| `west spdx` | ✅ Yes | SPDX 2.3 | **Primary** — walks actual build graph; incomplete in v3.2.x |
| `syft convert` | ✅ (converter) | CycloneDX | Converts SPDX → CDX for downstream tools |
| `grype` | ✅ Yes | JSON CVE report | **Mitigation** — binary scan via Anchore CDN; reliable from GitHub Actions (see Tool Selection Notes) |

## Hardware Bill of Materials (HBOM)

| Component | Description |
|---|---|
| u-blox NORA-B106 (nRF5340) | Dual-core ARM Cortex-M33 BLE module (application core) |
| STM32L051R8 | Ultra-low-power ARM Cortex-M0+ co-processor |

HBOM is manually maintained in [`hbom.cdx.json`](hbom.cdx.json) (CycloneDX 1.4) and archived alongside the SBOM in the Package stage.

---

## Pipeline Stages

The GitHub Actions pipeline ([`.github/workflows/pipeline.yml`](.github/workflows/pipeline.yml)) implements the **Embedded C++ (Zephyr / Nordic)** row of the internal CI/CD reference pipeline proposal:

| Stage | What it does | Key tools |
|---|---|---|
| **Lint** | Code style & formatting check (fast-fail) | `cpplint`, `clang-format`, `cmake-format` |
| **Build + SBOM** | Cross-compile firmware for nRF5340, generate SBOM | `west build`, Zephyr SDK v0.15.1, `west spdx` → `syft` (CycloneDX) |
| **Unit Test** | Run tests on `native_posix` with coverage | Zephyr Twister, Ztest, `gcovr` |
| **SAST · cppcheck** | Semantic C analysis — null deref, buffer OOB, UB, memory leaks | `cppcheck` + Gitleaks |
| **SAST · Semgrep** | Pattern-based security analysis — C security anti-patterns | Semgrep (`config: auto`) |
| **CVE Scan · grype** | CVE scan via SBOM — **primary scanner** | Anchore `grype` (CPE/NVD) |
| **CVE Scan · trivy** | CVE scan via SBOM + license check — documents limitations | `trivy sbom` |
| **CVE Scan · Dependency-Track** | Upload SBOM for continuous 24/7 re-scanning | OWASP Dependency-Track (local: `docker-compose.yml`) |
| **Package** | Sign firmware, archive SBOM + HBOM | `imgtool` (MCUboot), GitHub Artifacts |

### SBOM Flow

```
Build stage
  └─ west spdx → SPDX 2.3           ← primary (understands Zephyr build graph)
  └─ syft convert → CycloneDX JSON  ← for downstream CVE scanner consumption
  └─ sbom-gaps.md                   ← CRA Annex I gap documentation
        │
        ▼  (all 5 run in parallel after build + test)
  ┌─────────────────────────────────────────────────────────────────────────────┐
  │  sast-cppcheck   cppcheck (C semantic) + Gitleaks (secrets)              │
  │  sast-semgrep    Semgrep pattern-based SAST (C rules)                    │
  │  scan-grype      grype sbom:sbom.cdx.json                                │
  │  scan-trivy      trivy sbom sbom.cdx.json + license check                │
  │  scan-deptrack   DT API v1 PUT /bom upload                               │
  └─────────────────────────────────────────────────────────────────────────────┘
        │
        ▼
  Package stage archives SBOM + HBOM alongside signed firmware
```

### Pipeline Triggers

| Trigger | When |
|---|---|
| Push | Every push to `main` |
| Pull Request | Every PR targeting `main` |
| Schedule | Weekly Monday 07:00 UTC (Trivy vuln DB refresh) |
| Manual | `workflow_dispatch` |

### SAST Comparison (lab results — Zephyr 3.2.0 hello_app)

> **Scan date: 2026-05-08.** Findings reflect the demo hello_app C source (`src/`). Real-world firmware with complex state machines, network stacks, or crypto will produce more findings.

| Tool | Findings | Type | Notes |
|---|---|---|---|
| **cppcheck** | **0** | Semantic SAST | Deep C/C++ analysis — null deref, buffer OOB, undefined behaviour, memory leaks. **Primary C static analysis tool.** 0 findings expected in minimal demo code. |
| **Semgrep** | **0** | Pattern SAST | `config: auto` includes the C ruleset (`p/c`) — format string bugs, dangerous function use, injection patterns. Complements cppcheck. `continue-on-error`. |

**Key finding:** 0 findings is expected for the minimal demo code. Both tools are complementary — cppcheck understands C memory semantics deeply; Semgrep catches known-bad API usage patterns quickly. CodeQL (C/C++) would be the next upgrade but requires compiler instrumentation compatible with Zephyr's west build system.

### CVE Scanner Comparison (lab results — Zephyr 3.2.0 manual SBOM)

Manual SBOM [`sbom.cdx.json`](sbom.cdx.json) uses `pkg:generic` PURLs and CPEs to maximise scanner compatibility.

> **Scan date: 2026-05-08.** CVE counts reflect the vulnerability databases at that date. Counts will increase over time as new CVEs are published — re-run `grype sbom:sbom.cdx.json` or refresh the Dependency-Track project to get current numbers.

| Tool | CVEs found | Critical | High | Medium | Notes |
|---|---|---|---|---|---|
| **grype** | **54** | **13** | **21** | **20** | CPE/NVD matching. **Recommended for CI/CD gate.** |
| **trivy** | 0 | — | — | — | ❌ No database for `pkg:generic` / firmware. Use for license check only. |
| **Dependency-Track** | **55** | **13** | **22** | **20** | NVD + OSS Index + GitHub Advisories. +1 High vs grype. **Recommended for continuous monitoring.** |

**Key finding:** Trivy is an excellent scanner for container/OS/language ecosystems (npm, pip, Alpine, etc.) but **cannot match CVEs for firmware components** — it has no ecosystem DB for `pkg:generic`. It will always report 0 CVEs for Zephyr projects. Do **not** use trivy as a security gate for embedded firmware.

**Dependency-Track** is fully open source (OWASP, Apache 2.0). It queries NVD, OSS Index, and GitHub Advisory Database — which is why it found **55 CVEs (13C/22H/20M)**, one more High than grype's NVD-only scan. The key advantage is continuous re-scanning: if a new CVE is published tomorrow for Zephyr 3.2.0, DT alerts without needing a new build. Activate CI integration by adding `DT_URL` + `DT_API_KEY` secrets (see `docker-compose.yml` for local setup).

#### Notable grype findings (Zephyr 3.2.0)

| CVE | Severity | Fixed in | Description |
|---|---|---|---|
| CVE-2023-5055 | Critical | — | Zephyr Bluetooth stack buffer overflow |
| CVE-2023-3725 | Critical | — | Zephyr Bluetooth HCI buffer overflow |
| CVE-2023-6249 | Critical | 3.5.0 | Zephyr memory corruption |
| CVE-2023-4260 | Critical | — | Zephyr POSIX buffer overflow |
| CVE-2025-1675 | Critical | — | Zephyr network stack vulnerability |
| CVE-2023-7060 | High | 3.6.0 | Zephyr net/ipv6 OOB write |
| CVE-2023-4258 | Medium | 3.4.0 | Zephyr Bluetooth MESH |

> **Action required:** Upgrade to Zephyr ≥ 3.6.0 to resolve all CVEs with known fixes (CVE-2023-7060, CVE-2023-6249, CVE-2023-4258, CVE-2024-5754, CVE-2024-4785, CVE-2024-6258).

> **Why not Trivy fs for SBOM generation?** Trivy does not understand west/CMake and produces empty results for C/Zephyr projects. `west spdx` is the correct tool — it walks the actual Zephyr build graph.

---

## Tool Selection Notes

Obstacles encountered during pipeline development.

### CVE scanning: cve-bin-tool → grype (manual SBOM + pkg:generic)

**Key distinction:** `cve-bin-tool` and `grype` are fundamentally different tools:

| Tool | Approach | Input |
|---|---|---|
| `cve-bin-tool` | Binary string matching — scans ELF byte strings for version markers | Raw binary (`.elf`, `.hex`) |
| `grype` | SCA — matches component names/versions against CVE DB via CPE | SBOM (CycloneDX/SPDX) |

`cve-bin-tool` is the **only tool that produces real CVE results** from the compiled binary in Zephyr v3.2.x — it does not depend on SBOM quality. However, it cannot run in CI:

| Attempt | Problem |
|---|---|
| `cve-bin-tool` with NVD API 2.0 key | NVD API returns **HTTP 403** for GitHub Actions runner IPs — blocked at network level regardless of key validity |
| `cve-bin-tool -n json-mirror` | Downloads ~1 GB NVD JSON mirror; OSV source crashes with `FileNotFoundError: gsutil` (cve-bin-tool 3.4 bug on Python 3.14 — `--disable-data-source OSV` flag ignored) |

**Local workaround:** Install `google-cloud-sdk` (provides `gsutil`) and run `cve-bin-tool` locally:
```bash
brew install --cask google-cloud-sdk
cve-bin-tool --disable-data-source REDHAT --disable-data-source PURL2CPE \
  --format json --output cve-report.json path/to/zephyr.elf
```
Download `zephyr.elf` from the `firmware` CI artifact: `gh run download --repo mmmaction/zephyr-appsec-lab --name firmware -D firmware/`

**CI resolution:** `grype` scans the manual [`sbom.cdx.json`](sbom.cdx.json) using `pkg:generic` PURLs and CPE entries. With CPE matching enabled grype resolves CVEs via NVD and found **54 CVEs** (13 critical) for Zephyr 3.2.0 — making it the **primary CVE gate** in CI. The auto-generated SBOM from `west spdx` v3.2.x (file hashes only) remains the authoritative SBOM for CRA Annex I; the manual SBOM bridges the gap for scanning until Zephyr v3.4+ is adopted.

### Trivy: CVE scanning limitation for firmware

Trivy is kept in the pipeline for **license compliance only**. It has no CVE database for `pkg:generic` ecosystem components and will always return 0 CVEs for firmware/C projects. This is intentional and documented — the `scan-trivy` job has `exit-code: 0` and explicit comments explaining it cannot be used as a security gate for embedded firmware.

**Lab validation result:** `trivy sbom sbom.cdx.json` → 0 CVEs (confirmed false negative against same SBOM where grype finds 54 CVEs).

### Coverage reporting: gcovr auto-config

`gcovr` auto-discovers a `gcovr.cfg` file in the repo root. The `[gcovr]` section header in that file is not supported by the gcovr version shipped with `ubuntu-22.04`, causing the coverage step to fail.

**Resolution:** Deleted `gcovr.cfg` — all options are passed explicitly via CLI flags in the pipeline, making the config file redundant.

---

## Local Development

### Prerequisites

- Zephyr SDK v0.15.1 (ARM toolchain) — [install guide](https://docs.zephyrproject.org/3.2.0/develop/toolchains/zephyr_sdk.html)
- `west` (`pip install west`)

### Build

```bash
# Initialize west workspace (first time only)
west init -l hello_app
west update

# Cross-compile for nRF5340 DK (nRF5340 / NORA-B106)
west build -b nrf5340dk_nrf5340_cpuapp hello_app
```

### Test (no hardware needed)

```bash
# Run Ztest suite on native_posix (Linux host, no SDK needed)
west twister -p native_posix -T hello_app/tests --inline-logs
```

### Flash (requires board connected via J-Link)

```bash
west flash --build-dir build/nrf5340
```

---

## Security Configuration

### Required Secrets

| Secret | Required | Purpose |
|---|---|---|
| `CODECOV_TOKEN` | Optional | Upload coverage reports to [codecov.io](https://codecov.io) |
| `MCUBOOT_SIGNING_KEY` | Optional | Base64-encoded PEM for MCUboot firmware signing. Without it, pipeline warns and skips signing. |

> **Production note:** Use an HSM (e.g. AWS CloudHSM, Azure Dedicated HSM) for signing key storage instead of a GitHub secret.

### Enabling Blocking Security Gates

| Check | How to enable blocking |
|---|---|
| Vulnerability scan | Set `exit-code: "1"` in Trivy vulnerability step |
| License violation | Set `exit-code: "1"` in Trivy license step |
| cpplint | Remove `continue-on-error: true` from cpplint step |
| cmake-format | Remove `continue-on-error: true` from cmake-format step |
