import React from 'react';
import { useConfig } from '../context/PebbleConfigContext';
import { Page, Section, CityList, FormItemLabel, CustomCitiesSection } from '../components';
import { CITIES } from '../data/cities';
import type { CustomCity } from '../data/cities';
import { formatOffset } from '../utils/string';
import { LocationArrowIcon } from '../components/icons';

export const SettingsPage: React.FC = () => {
  const { settings, updateSetting } = useConfig();

  const pinnedCities = React.useMemo(() => {
    try {
      return JSON.parse(settings.SETTING_PINNED_CITIES || '[]');
    } catch {
      return [];
    }
  }, [settings.SETTING_PINNED_CITIES]);

  const customCities: CustomCity[] = React.useMemo(() => {
    try {
      return JSON.parse(settings.SETTING_CUSTOM_CITIES || '[]');
    } catch {
      return [];
    }
  }, [settings.SETTING_CUSTOM_CITIES]);

  const handleToggleCity = (cityName: string) => {
    const newPinnedCities = pinnedCities.includes(cityName)
      ? pinnedCities.filter((name: string) => name !== cityName)
      : [...pinnedCities, cityName];
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify(newPinnedCities));
  };

  const handlePinAll = () => {
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify(CITIES.map(c => c.name)));
  };

  const handleUnpinAll = () => {
    updateSetting('SETTING_PINNED_CITIES', JSON.stringify([]));
  };

  const handleAddCustomCity = (city: CustomCity) => {
    updateSetting('SETTING_CUSTOM_CITIES', JSON.stringify([...customCities, city]));
  };

  const handleDeleteCustomCity = (index: number) => {
    updateSetting('SETTING_CUSTOM_CITIES', JSON.stringify(customCities.filter((_, i) => i !== index)));
  };

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
          footer={
            <CustomCitiesSection
              customCities={customCities}
              onAdd={handleAddCustomCity}
              onDelete={handleDeleteCustomCity}
            />
          }
        >
          <div className="city-item">
            <div className="city-toggle-content">
              <div className="city-pin-icon neutral">
                <LocationArrowIcon />
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
    </Page>
  );
};
