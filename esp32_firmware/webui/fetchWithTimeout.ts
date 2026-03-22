/**
 * Compatibility wrapper for fetch with timeout.
 *
 * Uses AbortController + setTimeout instead of AbortSignal.timeout() to
 * support older mobile webviews (Android System WebView < 105, iOS < 15.4,
 * etc.) that do not implement AbortSignal.timeout().
 *
 * Note: the `signal` field is excluded from `init` because this function
 * manages its own AbortController. Any signal you need to combine with the
 * timeout should be composed externally via AbortSignal.any() (where
 * available) before calling this helper.
 *
 * @param input  - URL or Request to fetch
 * @param init   - Optional RequestInit options (must NOT include `signal`)
 * @param timeoutMs - Abort timeout in milliseconds (default: 5000)
 */
export function fetchWithTimeout(
  input: RequestInfo | URL,
  init?: Omit<RequestInit, 'signal'>,
  timeoutMs = 5000,
): Promise<Response> {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), timeoutMs);
  return fetch(input, { ...init, signal: controller.signal }).finally(() => {
    clearTimeout(timeoutId);
  });
}
