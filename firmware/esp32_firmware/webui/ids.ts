/**
 * Stable id for sequence items.
 * crypto.randomUUID throws on insecure http origins (the poi AP). Fall back to
 * getRandomValues, then Math.random, so Image Lab still works on http://192.168.4.1.
 */
export function createId(): string {
  const cryptoObj = globalThis.crypto;
  if (cryptoObj && typeof cryptoObj.randomUUID === 'function') {
    try {
      return cryptoObj.randomUUID();
    } catch {
      // Insecure context. Continue to getRandomValues.
    }
  }
  if (cryptoObj && typeof cryptoObj.getRandomValues === 'function') {
    const bytes = new Uint8Array(16);
    cryptoObj.getRandomValues(bytes);
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;
    const hex = Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('');
    return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`;
  }
  const rand = () => Math.random().toString(16).slice(2);
  return `id-${Date.now().toString(16)}-${rand()}-${rand()}`;
}
