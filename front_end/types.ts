export interface SensorData {
  fire: number;
  co2: number;
  temp: number;
  hum: number;
  buzzer: boolean;
  pump: boolean;
  fan: boolean;
  recipientEmail: string;
  ssid: string;
  // Optional fields for syncing settings from Backend
  fireThresh?: number;
  co2Thresh?: number;
  tempThresh?: number;
}

export interface HistoryEvent {
  id: number;
  timestamp: number;
  message: string;
  isDanger: boolean;
}

export interface SettingsPayload {
  fireThreshold: number;
  co2Threshold: number;
  tempThreshold: number;
  recipientEmail: string;
  password?: string;
}

export enum ConnectionState {
  DISCONNECTED,
  CONNECTING,
  CONNECTED
}