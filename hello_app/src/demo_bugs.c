/**
 * demo_bugs.c — Intentional cppcheck findings for lab demonstration.
 *
 * This file is NOT compiled into the firmware (excluded from CMakeLists.txt).
 * It exists solely to produce visible cppcheck findings in the SAST stage,
 * demonstrating that the tool is active and working.
 *
 * Findings expected:
 *   1. bufferAccessOutOfBounds — write past end of fixed-size array
 *   2. nullPointerDereference  — pointer returned by malloc() used without
 *                                NULL check
 *   3. memleak                 — heap allocation never freed
 *
 * Fix for each: see inline comments.
 */

#include <stdlib.h>
#include <string.h>

/* ── Finding 1: Buffer overrun ──────────────────────────────────────────────
 * buf has 8 bytes. The loop writes 9 bytes (indices 0..8).
 * Fix: change loop bound to i < 8, or increase buf size to 9.
 */
void demo_buffer_overrun(void)
{
	char buf[8];

	for (int i = 0; i <= 8; i++) { /* off-by-one: writes buf[8] */
		buf[i] = (char)i;
	}

	(void)buf;
}

/* ── Finding 2: Null pointer dereference + Finding 3: Memory leak ───────────
 * malloc() can return NULL. Dereferencing without a NULL check is UB.
 * The allocated buffer is also never freed — memory leak.
 * Fix: check ptr != NULL before use; call free(ptr) before returning.
 */
void demo_null_deref_and_leak(void)
{
	char *ptr = (char *)malloc(64);

	/* cppcheck: nullPointerDereference — ptr may be NULL */
	memset(ptr, 0, 64);

	/* cppcheck: memleak — ptr is never freed */
}
