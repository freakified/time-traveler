import tzlookup from '@photostructure/tz-lookup';
import { CITIES } from '../data/cities';
import type { CustomCity } from '../data/cities';

const formatters: Record<string, Intl.DateTimeFormat> = {};

const formatterFor = (tz: string): Intl.DateTimeFormat => {
  if (!formatters[tz]) {
    formatters[tz] = new Intl.DateTimeFormat('en-US', {
      timeZone: tz,
      hour12: false,
      year: 'numeric', month: 'numeric', day: 'numeric',
      hour: 'numeric', minute: 'numeric',
    });
  }
  return formatters[tz];
};

// Same approach as intlOffsetMinutes in src/pkjs/cities.js.
export const offsetMinutes = (tz: string, date: Date = new Date()): number => {
  const p: Record<string, string> = {};
  for (const part of formatterFor(tz).formatToParts(date)) p[part.type] = part.value;
  // Some engines render midnight as "24" with hour12:false.
  const wallAsUtc = Date.UTC(+p.year, +p.month - 1, +p.day, +p.hour % 24, +p.minute);
  const subMinuteMs = date.getUTCSeconds() * 1000 + date.getUTCMilliseconds();
  return Math.round((wallAsUtc - (date.getTime() - subMinuteMs)) / 60000);
};

// Standard time is whichever of January/July is earlier; DST always moves clocks forward
// as far as Intl is concerned. The watch only uses this when Intl is unavailable (emulator).
export const standardOffsetMinutes = (tz: string): number => {
  const year = new Date().getUTCFullYear();
  return Math.min(
    offsetMinutes(tz, new Date(Date.UTC(year, 0, 15, 12))),
    offsetMinutes(tz, new Date(Date.UTC(year, 6, 15, 12))),
  );
};

const isUsableZone = (tz: string): boolean => {
  try {
    offsetMinutes(tz);
    return true;
  } catch {
    return false;
  }
};

export const lookupTimeZone = (lat: number, lon: number): string => {
  try {
    const tz = tzlookup(lat, lon);
    if (isUsableZone(tz)) return tz;
  } catch {
    // Invalid coordinates
  }
  return 'UTC';
};

export interface ZoneOption {
  tz: string;
  offset: number;
}

let zoneOptions: ZoneOption[] | null = null;

// Every zone the browser knows, sorted west to east by current offset.
export const allTimeZones = (): ZoneOption[] => {
  if (zoneOptions) return zoneOptions;

  const supportedValuesOf = (Intl as { supportedValuesOf?: (key: string) => string[] }).supportedValuesOf;
  const names = new Set<string>(
    supportedValuesOf ? supportedValuesOf('timeZone') : CITIES.map(c => c.timezone),
  );
  names.add('UTC');

  zoneOptions = [...names]
    .filter(isUsableZone)
    .map(tz => ({ tz, offset: offsetMinutes(tz) }))
    .sort((a, b) => a.offset - b.offset || a.tz.localeCompare(b.tz));
  return zoneOptions;
};

// Custom cities saved before IANA zone support reference a built-in city instead.
export const customCityZone = (city: CustomCity): string =>
  city.tz ?? CITIES.find(c => c.name === city.tzCityName)?.timezone ?? 'UTC';

// Older app versions only read tzCityName and drop custom cities without it, so new
// cities also carry the built-in city whose current offset is closest.
export const closestBuiltInCityName = (tz: string): string => {
  const target = offsetMinutes(tz);
  const exact = CITIES.find(c => c.timezone === tz);
  if (exact) return exact.name;
  return CITIES.reduce((best, city) =>
    Math.abs(offsetMinutes(city.timezone) - target) < Math.abs(offsetMinutes(best.timezone) - target)
      ? city
      : best
  ).name;
};
