import React from 'react';
import { Switch } from 'react-aria-components';
import { useConfig } from '../context/PebbleConfigContext';
import { FormItemLabel } from './FormItem';

export const CityList: React.FC<{
  title: string;
  cities: { name: string; timezone: string }[];
  pinnedCities: string[];
  onToggle: (cityName: string) => void;
}> = ({ title, cities, pinnedCities, onToggle }) => {
  return (
    <div className="city-list-section">
      <h3>{title}</h3>
      <div className="city-list">
        {cities.map((city) => (
          <div key={city.name} className="city-item">
            <Switch
              isSelected={pinnedCities.includes(city.name)}
              onChange={() => onToggle(city.name)}
              className="city-toggle"
            >
              <FormItemLabel 
                label={`${pinnedCities.includes(city.name) ? '[unpin] ' : '[pin] '}${city.name}, ${city.timezone}`}
              />
            </Switch>
          </div>
        ))}
      </div>
    </div>
  );
};