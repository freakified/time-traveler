export const formatOffset = (minutes: number): string => {
  const hours = Math.floor(Math.abs(minutes) / 60);
  const mins = Math.abs(minutes) % 60;
  const sign = minutes >= 0 ? '+' : '-';
  const timeStr = mins === 0 ? `${hours}` : `${hours}:${mins.toString().padStart(2, '0')}`;
  return `UTC${sign}${timeStr}`;
};

// City names are sent to the watch as UTF-8 with a byte limit, so accented
// letters count double. Truncates on a character boundary.
export const utf8Length = (str: string): number => new TextEncoder().encode(str).length;

export const truncateUtf8 = (str: string, maxBytes: number): string => {
  let out = '';
  for (const ch of str) {
    if (utf8Length(out + ch) > maxBytes) break;
    out += ch;
  }
  return out;
};
