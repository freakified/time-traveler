import React from 'react';
import { Button } from 'react-aria-components';
import type { CustomCity } from '../data/cities';
import { CITIES } from '../data/cities';
import { formatOffset } from '../utils/string';
import { FormItemLabel } from './FormItem';
import { AddCityModal } from './AddCityModal';
import { TrashIcon, PlusIcon } from './icons';

interface CustomCitiesSectionProps {
  customCities: CustomCity[];
  onAdd: (city: CustomCity) => void;
  onDelete: (index: number) => void;
}

export const CustomCitiesSection: React.FC<CustomCitiesSectionProps> = ({
  customCities,
  onAdd,
  onDelete,
}) => {
  const [addModalOpen, setAddModalOpen] = React.useState(false);

  return (
    <>
      {customCities.map((city, index) => {
        const refCity = CITIES.find(c => c.name === city.tzCityName);
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
                description={refCity ? formatOffset(refCity.offset) : ''}
              />
            </div>
          </div>
        );
      })}
      <div className="city-item city-item-add">
        <Button className="city-add-custom-btn" onPress={() => setAddModalOpen(true)}>
          <PlusIcon />
          Add custom city
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
