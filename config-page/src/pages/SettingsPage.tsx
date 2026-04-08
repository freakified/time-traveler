import React from 'react';
import { useConfig } from '../context/PebbleConfigContext';
import { Page, Section, CityList, FormItemLabel, CustomCitiesSection } from '../components';
import { CITIES } from '../data/cities';
import type { CustomCity } from '../data/cities';
import { LocationArrowIcon } from '../components/icons';

const MAX_CITIES = 50;

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
    const isCurrentlyPinned = pinnedCities.includes(cityName);
    if (!isCurrentlyPinned && 1 + pinnedCities.length + customCities.length >= MAX_CITIES) return;
    const newPinnedCities = isCurrentlyPinned
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
    if (1 + pinnedCities.length + customCities.length >= MAX_CITIES) return;
    updateSetting('SETTING_CUSTOM_CITIES', JSON.stringify([...customCities, city]));
  };

  const handleDeleteCustomCity = (index: number) => {
    updateSetting('SETTING_CUSTOM_CITIES', JSON.stringify(customCities.filter((_, i) => i !== index)));
  };

  // Current location always occupies one slot
  const totalCities = 1 + pinnedCities.length + customCities.length;
  const atLimit = totalCities >= MAX_CITIES;
  const remainingSlots = MAX_CITIES - totalCities;

  const pinnedCityList = CITIES.filter(city => pinnedCities.includes(city.name));
  const unpinnedCityList = CITIES.filter(city => !pinnedCities.includes(city.name));

  return (
    <Page title="Time Traveler Settings">
      <Section>
        <CityList
          title="Your Cities"
          titleCount={`${totalCities}/${MAX_CITIES}`}
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
              disableAdd={atLimit}
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
          disablePinning={atLimit}
          actionLabel="Pin All"
          onAction={unpinnedCityList.length > 0 && unpinnedCityList.length <= remainingSlots ? handlePinAll : undefined}
          emptyStateMessage="All available cities pinned; the true Casio™ World Time experience!"
        />
      </Section>
    </Page>
  );
};
