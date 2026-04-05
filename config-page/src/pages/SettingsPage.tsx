import React from 'react';
import { useConfig, useCapabilities, useWatchInfo } from '../context/PebbleConfigContext';
import { Page, Section, CityList } from '../components';
import { CITIES } from '../data/cities';


export const SettingsPage: React.FC = () => {
  const { settings, updateSetting } = useConfig();
  const capabilities = useCapabilities();
  const watchInfo = useWatchInfo();

  // Parse pinned cities from settings
  const pinnedCities = React.useMemo(() => {
    try {
      return JSON.parse(settings.SETTING_PINNED_CITIES || '[]');
    } catch {
      return [];
    }
  }, [settings.SETTING_PINNED_CITIES]);

  const handleToggleCity = (cityName: string) => {
    const newPinnedCities = pinnedCities.includes(cityName)
      ? pinnedCities.filter(name => name !== cityName)
      : [...pinnedCities, cityName];
    
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify(newPinnedCities));
  };

  // Separate cities into pinned and unpinned
  const pinnedCityList = CITIES.filter(city => pinnedCities.includes(city.name));
  const unpinnedCityList = CITIES.filter(city => !pinnedCities.includes(city.name));

  return (
    <Page title="Time Traveler Settings">
      <Section title="Your Cities">
        <CityList
          title="Pinned Cities"
          cities={pinnedCityList}
          pinnedCities={pinnedCities}
          onToggle={handleToggleCity}
        />
      </Section>
      
      <Section title="All Cities">
        <CityList
          title="Available Cities"
          cities={unpinnedCityList}
          pinnedCities={pinnedCities}
          onToggle={handleToggleCity}
        />
      </Section>
    </Page >
  );
};
