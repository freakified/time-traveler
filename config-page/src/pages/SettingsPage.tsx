import React from 'react';
import { Page, Section, CityList, FormItemLabel, CustomCitiesSection } from '../components';
import { LocationArrowIcon } from '../components/icons';
import { useCitySettings } from '../hooks/useCitySettings';

export const SettingsPage: React.FC = () => {
  const {
    pinnedCities,
    customCities,
    pinnedCityList,
    unpinnedCityList,
    totalCities,
    maxCities,
    atLimit,
    remainingSlots,
    toggleCity,
    pinAll,
    unpinAll,
    addCustomCity,
    deleteCustomCity,
  } = useCitySettings();

  return (
    <Page title="Time Traveler Settings">
      <Section>
        <CityList
          title="Your Cities"
          titleCount={`${totalCities}/${maxCities}`}
          cities={pinnedCityList}
          pinnedCities={pinnedCities}
          onToggle={toggleCity}
          actionLabel="Unpin All"
          onAction={pinnedCityList.length > 0 ? unpinAll : undefined}
          footer={
            <CustomCitiesSection
              customCities={customCities}
              onAdd={addCustomCity}
              onDelete={deleteCustomCity}
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
          onToggle={toggleCity}
          disablePinning={atLimit}
          actionLabel="Pin All"
          onAction={unpinnedCityList.length > 0 && unpinnedCityList.length <= remainingSlots ? pinAll : undefined}
          emptyStateMessage="All available cities pinned; the true Casio™ World Time experience!"
        />
      </Section>
    </Page>
  );
};
