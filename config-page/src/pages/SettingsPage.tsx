import React from 'react';
import { useConfig } from '../context/PebbleConfigContext';
import { Page, Section, CityList, FormItemLabel } from '../components';
import { CITIES } from '../data/cities';


export const SettingsPage: React.FC = () => {
  const { settings, updateSetting } = useConfig();

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

  const handlePinAll = () => {
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify(CITIES.map(c => c.name)));
  };

  const handleUnpinAll = () => {
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify([]));
  };

  // Separate cities into pinned and unpinned
  const pinnedCityList = CITIES.filter(city => pinnedCities.includes(city.name));
  const unpinnedCityList = CITIES.filter(city => !pinnedCities.includes(city.name));

  return (
    <Page title="Time Traveler Settings">
      <Section>
        <CityList
          title="Your Cities"
          cities={pinnedCityList}
          pinnedCities={pinnedCities}
          onToggle={handleToggleCity}
          actionLabel="Unpin All"
          onAction={pinnedCityList.length > 0 ? handleUnpinAll : undefined}
        >
          <div className="city-item">
            <div className="city-toggle-content">
              <div className="city-pin-icon neutral">
                <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 15 17" fill="currentColor">
                  <path d="m0 9 13-9-4 15-3-6Z" />
                </svg>
              </div>
              <FormItemLabel label="Current Location" description="Always shown" />
            </div>
          </div>
        </CityList>
      </Section>

      <Section>
        <CityList
          title="Available Cities"
          cities={unpinnedCityList}
          pinnedCities={pinnedCities}
          onToggle={handleToggleCity}
          actionLabel="Pin All"
          onAction={unpinnedCityList.length > 0 ? handlePinAll : undefined}
          emptyStateMessage="All available cities pinned; the true Casio™ World Time experience!"
        />
      </Section>
    </Page >
  );
};
