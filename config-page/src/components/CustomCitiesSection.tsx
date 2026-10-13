import React from 'react';
import { Button } from 'react-aria-components';
import type { CustomCity } from '../data/cities';
import { formatOffset } from '../utils/string';
import { customCityZone, offsetMinutes } from '../utils/timezone';
import { FormItemLabel } from './FormItem';
import { AddCityModal } from './AddCityModal';
import { TrashIcon, PlusIcon } from './icons';

interface CustomCitiesSectionProps {
  customCities: CustomCity[];
  onAdd: (city: CustomCity) => void;
  onDelete: (index: number) => void;
  disableAdd?: boolean;
}

export const CustomCitiesSection: React.FC<CustomCitiesSectionProps> = ({
  customCities,
  onAdd,
  onDelete,
  disableAdd,
}) => {
  const [addModalOpen, setAddModalOpen] = React.useState(false);

  return (
    <>
      {customCities.map((city, index) => {
        const offset = formatOffset(offsetMinutes(customCityZone(city)));
        return (
          <div key={`custom-${index}`} className="city-item">
            <div className="city-toggle-content">
              <Button
                className="city-delete-btn"
                aria-label={`Remove ${city.displayName}`}
                onPress={() => onDelete(index)}
              >
                <TrashIcon />
              </Button>
              <FormItemLabel
                label={city.displayName}
                description={`Custom city · ${offset}`}
              />
            </div>
          </div>
        );
      })}
      <div className="city-item city-item-add">
        <Button
          className="city-add-custom-btn"
          isDisabled={disableAdd}
          onPress={() => setAddModalOpen(true)}
        >
          <PlusIcon />
          <span>
            Add custom city
            {disableAdd && (
              <span className="city-add-limit-note">
                Already at limit; unpin some cities to add a custom one.
              </span>
            )}
          </span>
        </Button>
      </div>
      <AddCityModal
        isOpen={addModalOpen}
        onOpenChange={setAddModalOpen}
        onAdd={onAdd}
      />
    </>
  );
};
