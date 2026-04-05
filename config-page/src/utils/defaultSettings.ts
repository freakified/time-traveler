import { Settings, Capabilities } from '../context/types';

export const getDefaultSettings = (capabilities: Capabilities): Settings => ({
  SETTING_PINNED_CITIES: JSON.stringify(['SAN FRANCISCO', 'NEW YORK', 'LONDON', 'PARIS', 'TOKYO', 'SYDNEY']),
});
