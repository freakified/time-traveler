export interface City {
  name: string;
  displayName: string;
  timezone: string;
}

export interface CustomCity {
  displayName: string; // What appears on the watch (max 15 chars)
  lat: number;
  lon: number;
  tz?: string; // IANA zone
  std?: number; // Standard offset in minutes, used by the watch only when Intl is unavailable
  tzCityName?: string; // References CITIES[].name; only read by app versions before tz existed
}

export const CITIES: City[] = [
  { name: "PAGO PAGO", displayName: "Pago Pago", timezone: "Pacific/Pago_Pago" },
  { name: "HONOLULU", displayName: "Honolulu", timezone: "Pacific/Honolulu" },
  { name: "ANCHORAGE", displayName: "Anchorage", timezone: "America/Anchorage" },
  { name: "VANCOUVER", displayName: "Vancouver", timezone: "America/Vancouver" },
  { name: "SAN FRANCISCO", displayName: "San Francisco", timezone: "America/Los_Angeles" },
  { name: "EDMONTON", displayName: "Edmonton", timezone: "America/Edmonton" },
  { name: "DENVER", displayName: "Denver", timezone: "America/Denver" },
  { name: "MEXICO CITY", displayName: "Mexico City", timezone: "America/Mexico_City" },
  { name: "CHICAGO", displayName: "Chicago", timezone: "America/Chicago" },
  { name: "NEW YORK", displayName: "New York", timezone: "America/New_York" },
  { name: "SANTIAGO", displayName: "Santiago", timezone: "America/Santiago" },
  { name: "HALIFAX", displayName: "Halifax", timezone: "America/Halifax" },
  { name: "ST. JOHNS", displayName: "St. John's", timezone: "America/St_Johns" },
  { name: "RIO DE JANEIRO", displayName: "Rio de Janeiro", timezone: "America/Sao_Paulo" },
  { name: "F. DE NORONHA", displayName: "F. de Noronha", timezone: "America/Noronha" },
  { name: "PRAIA", displayName: "Praia", timezone: "Atlantic/Cape_Verde" },
  { name: "UTC", displayName: "UTC", timezone: "UTC" },
  { name: "LISBON", displayName: "Lisbon", timezone: "Europe/Lisbon" },
  { name: "LONDON", displayName: "London", timezone: "Europe/London" },
  { name: "MADRID", displayName: "Madrid", timezone: "Europe/Madrid" },
  { name: "PARIS", displayName: "Paris", timezone: "Europe/Paris" },
  { name: "ROME", displayName: "Rome", timezone: "Europe/Rome" },
  { name: "BERLIN", displayName: "Berlin", timezone: "Europe/Berlin" },
  { name: "STOCKHOLM", displayName: "Stockholm", timezone: "Europe/Stockholm" },
  { name: "ATHENS", displayName: "Athens", timezone: "Europe/Athens" },
  { name: "CAIRO", displayName: "Cairo", timezone: "Africa/Cairo" },
  { name: "JERUSALEM", displayName: "Jerusalem", timezone: "Asia/Jerusalem" },
  { name: "MOSCOW", displayName: "Moscow", timezone: "Europe/Moscow" },
  { name: "JEDDAH", displayName: "Jeddah", timezone: "Asia/Riyadh" },
  { name: "TEHRAN", displayName: "Tehran", timezone: "Asia/Tehran" },
  { name: "DUBAI", displayName: "Dubai", timezone: "Asia/Dubai" },
  { name: "KABUL", displayName: "Kabul", timezone: "Asia/Kabul" },
  { name: "KARACHI", displayName: "Karachi", timezone: "Asia/Karachi" },
  { name: "DELHI", displayName: "Delhi", timezone: "Asia/Kolkata" },
  { name: "KATHMANDU", displayName: "Kathmandu", timezone: "Asia/Kathmandu" },
  { name: "DHAKA", displayName: "Dhaka", timezone: "Asia/Dhaka" },
  { name: "YANGON", displayName: "Yangon", timezone: "Asia/Yangon" },
  { name: "BANGKOK", displayName: "Bangkok", timezone: "Asia/Bangkok" },
  { name: "SINGAPORE", displayName: "Singapore", timezone: "Asia/Singapore" },
  { name: "HONG KONG", displayName: "Hong Kong", timezone: "Asia/Hong_Kong" },
  { name: "BEIJING", displayName: "Beijing", timezone: "Asia/Shanghai" },
  { name: "TAIPEI", displayName: "Taipei", timezone: "Asia/Taipei" },
  { name: "SEOUL", displayName: "Seoul", timezone: "Asia/Seoul" },
  { name: "TOKYO", displayName: "Tokyo", timezone: "Asia/Tokyo" },
  { name: "ADELAIDE", displayName: "Adelaide", timezone: "Australia/Adelaide" },
  { name: "GUAM", displayName: "Guam", timezone: "Pacific/Guam" },
  { name: "SYDNEY", displayName: "Sydney", timezone: "Australia/Sydney" },
  { name: "NOUMEA", displayName: "Noumea", timezone: "Pacific/Noumea" },
  { name: "WELLINGTON", displayName: "Wellington", timezone: "Pacific/Auckland" }
];
