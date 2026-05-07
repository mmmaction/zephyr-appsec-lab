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

> **Known gap (Zephyr v3.2.x):** `west spdx` in v3.2.x captures file hashes only — no component names, no versions, no license metadata. CVE matching via SBOM is not possible. Documented in `sbom-gaps.md` (archived with each build). **Mitigation:** `cve-bin-tool` scans the compiled `zephyr.elf` binary directly. Upgrade to Zephyr v3.4+ for complete SPDX output.

### SBOM Tool Evaluation

| Tool | Understands Zephyr? | Output | Verdict |
|---|---|---|---|
| `trivy fs` | ❌ No | Empty SBOM | Does not understand west/CMake |
| `west spdx` | ✅ Yes | SPDX 2.3 | **Primary** — walks actual build graph; incomplete in v3.2.x |
| `syft convert` | ✅ (converter) | CycloneDX | Converts SPDX → CDX for downstream tools |
| `cve-bin-tool` | ✅ Yes | JSON CVE report | **Mitigation** — binary scan, works regardless of SBOM quality |

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
| **Build + SBOM** | Cross-compile firmware for nRF5340, generate SBOM + CVE scan | `west build`, Zephyr SDK v0.15.1, `west spdx` → `syft` (CycloneDX), `cve-bin-tool` |
| **Unit Test** | Run tests on `native_posix` with coverage | Zephyr Twister, Ztest, `gcovr` |
| **SAST / SCA** | Static analysis + vulnerability/license/secret scan | `cppcheck`, Trivy SCA (via SBOM), Gitleaks |
| **Package** | Sign firmware, archive SBOM + HBOM | `imgtool` (MCUboot), GitHub Artifacts |

### SBOM Flow

```
Build stage
  └─ west spdx → SPDX 2.3           ← primary (understands Zephyr build graph)
  └─ syft convert → CycloneDX JSON  ← for DependencyTrack + Trivy downstream
  └─ cve-bin-tool → cve-report.json  ← binary CVE scan (mitigation for v3.2.x SBOM gaps)
  └─ sbom-gaps.md                   ← CRA Annex I gap documentation
        │
        ▼
  SAST/SCA stage consumes CycloneDX SBOM
    ├─ Trivy: vulnerability scan (SARIF → Security tab)
    ├─ Trivy: license compliance check
    └─ Gitleaks: secret scanning (full git history)
        │
        ▼
  Package stage archives SBOM (SPDX + CycloneDX) + HBOM alongside signed firmware
```

> **Why not Trivy fs for SBOM generation?** Trivy does not understand west/CMake and produces empty results for C/Zephyr projects. `west spdx` is the correct tool — it walks the actual Zephyr build graph.

### Pipeline Triggers

| Trigger | When |
|---|---|
| Push | Every push to `main` |
| Pull Request | Every PR targeting `main` |
| Schedule | Weekly Monday 07:00 UTC (Trivy vuln DB refresh) |
| Manual | `workflow_dispatch` |

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
