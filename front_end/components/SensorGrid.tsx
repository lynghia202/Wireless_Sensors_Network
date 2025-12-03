import React, { useState } from 'react';
import { Thermometer, Droplets, Wind, Flame, Info } from 'lucide-react';
import { SensorData, SettingsPayload, HistoryEvent } from '../types';
import SensorDetailModal, { SensorType } from './SensorDetailModal';

interface SensorGridProps {
  data: SensorData;
  settings: SettingsPayload;
  isConnected: boolean;
  chartData: any[];
  history: HistoryEvent[];
}

const SensorGrid: React.FC<SensorGridProps> = ({ data, settings, isConnected, chartData, history }) => {
  const [selectedSensor, setSelectedSensor] = useState<SensorType | null>(null);

  const isTempHigh = data.temp > settings.tempThreshold;
  const isGasHigh = data.co2 > settings.co2Threshold;
  const isFireDetected = data.fire > settings.fireThreshold;

  return (
    <>
      <div className={`grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 transition-opacity duration-500 ${!isConnected ? 'opacity-60 grayscale' : ''}`}>
        {/* Temperature */}
        <div 
          onClick={() => setSelectedSensor('temp')}
          className="group cursor-pointer bg-white p-6 rounded-2xl shadow-sm border border-slate-100 hover:shadow-lg hover:border-orange-200 transition-all duration-300 relative"
        >
          <div className="absolute top-4 right-4 opacity-0 group-hover:opacity-100 transition-opacity">
            <Info className="h-4 w-4 text-slate-300" />
          </div>
          <div className="flex items-center justify-between mb-4">
            <div className="p-3 bg-orange-100 text-orange-600 rounded-xl">
              <Thermometer className="h-6 w-6" />
            </div>
            <span className={`text-xs font-bold px-2 py-1 rounded-full ${isTempHigh ? 'bg-red-100 text-red-600' : 'bg-green-100 text-green-600'}`}>
              {isTempHigh ? 'CAO' : 'BÌNH THƯỜNG'}
            </span>
          </div>
          <h3 className="text-slate-500 text-sm font-medium">Nhiệt độ</h3>
          <div className="flex items-baseline gap-1">
            <span className="text-3xl font-bold text-slate-900">{data.temp.toFixed(1)}</span>
            <span className="text-sm text-slate-500">°C</span>
          </div>
          <p className="text-[10px] text-slate-400 mt-2">Ngưỡng báo động: &gt; {settings.tempThreshold}°C</p>
        </div>

        {/* Humidity */}
        <div 
          onClick={() => setSelectedSensor('hum')}
          className="group cursor-pointer bg-white p-6 rounded-2xl shadow-sm border border-slate-100 hover:shadow-lg hover:border-blue-200 transition-all duration-300 relative"
        >
          <div className="absolute top-4 right-4 opacity-0 group-hover:opacity-100 transition-opacity">
             <Info className="h-4 w-4 text-slate-300" />
          </div>
          <div className="flex items-center justify-between mb-4">
            <div className="p-3 bg-blue-100 text-blue-600 rounded-xl">
              <Droplets className="h-6 w-6" />
            </div>
            <span className="text-xs font-bold px-2 py-1 rounded-full bg-blue-50 text-blue-600">
              TRỰC TIẾP
            </span>
          </div>
          <h3 className="text-slate-500 text-sm font-medium">Độ ẩm</h3>
          <div className="flex items-baseline gap-1">
            <span className="text-3xl font-bold text-slate-900">{data.hum.toFixed(1)}</span>
            <span className="text-sm text-slate-500">%</span>
          </div>
          <p className="text-[10px] text-slate-400 mt-2">Độ ẩm không khí hiện tại</p>
        </div>

        {/* Gas/Smoke */}
        <div 
          onClick={() => setSelectedSensor('gas')}
          className="group cursor-pointer bg-white p-6 rounded-2xl shadow-sm border border-slate-100 hover:shadow-lg hover:border-indigo-200 transition-all duration-300 relative"
        >
          <div className="absolute top-4 right-4 opacity-0 group-hover:opacity-100 transition-opacity">
            <Info className="h-4 w-4 text-slate-300" />
          </div>
          <div className="flex items-center justify-between mb-4">
            <div className="p-3 bg-indigo-50 text-indigo-600 rounded-xl">
              <Wind className="h-6 w-6" />
            </div>
            <span className={`text-xs font-bold px-2 py-1 rounded-full ${isGasHigh ? 'bg-red-100 text-red-600' : 'bg-green-100 text-green-600'}`}>
              {data.co2} PPM
            </span>
          </div>
          <h3 className="text-slate-500 text-sm font-medium">Khí Gas / Khói</h3>
          <div className="flex items-baseline gap-1">
            <div className="w-full bg-slate-100 rounded-full h-2.5 mt-3 overflow-hidden">
              <div 
                className={`h-2.5 rounded-full transition-all duration-1000 ${isGasHigh ? 'bg-rose-500' : 'bg-emerald-500'}`} 
                style={{width: `${Math.min((data.co2 / 10000) * 100, 100)}%`}}
              ></div>
            </div>
          </div>
          <p className="text-[10px] text-slate-400 mt-2">Ngưỡng báo động: &gt; {settings.co2Threshold} PPM</p>
        </div>

        {/* Fire (IR) */}
        <div 
          onClick={() => setSelectedSensor('fire')}
          className="group cursor-pointer bg-white p-6 rounded-2xl shadow-sm border border-slate-100 hover:shadow-lg hover:border-rose-200 transition-all duration-300 relative"
        >
          <div className="absolute top-4 right-4 opacity-0 group-hover:opacity-100 transition-opacity">
            <Info className="h-4 w-4 text-slate-300" />
          </div>
          <div className="flex items-center justify-between mb-4">
            <div className="p-3 bg-rose-100 text-rose-600 rounded-xl">
              <Flame className="h-6 w-6" />
            </div>
            <span className={`text-xs font-bold px-2 py-1 rounded-full ${isFireDetected ? 'bg-red-100 text-red-600' : 'bg-green-100 text-green-600'}`}>
              {isFireDetected ? 'PHÁT HIỆN' : 'AN TOÀN'}
            </span>
          </div>
          <h3 className="text-slate-500 text-sm font-medium">Cảm biến Lửa (IR)</h3>
          <div className="flex items-baseline gap-1">
            <span className="text-3xl font-bold text-slate-900">{data.fire}</span>
            <span className="text-xs text-slate-400 ml-2">Giá trị đo</span>
          </div>
          <div className="w-full bg-slate-100 rounded-full h-2.5 mt-3 overflow-hidden">
              <div 
                className={`h-2.5 rounded-full transition-all duration-1000 ${isFireDetected ? 'bg-rose-500' : 'bg-emerald-500'}`} 
                style={{width: `${Math.min((data.fire / 4095) * 100, 100)}%`}}
              ></div>
          </div>
          <p className="text-[10px] text-slate-400 mt-2">Ngưỡng báo động: &gt; {settings.fireThreshold}</p>
        </div>
      </div>

      {/* Modal */}
      {selectedSensor && (
        <SensorDetailModal 
          isOpen={!!selectedSensor}
          onClose={() => setSelectedSensor(null)}
          sensorType={selectedSensor}
          currentData={data}
          chartData={chartData}
          history={history}
        />
      )}
    </>
  );
};

export default SensorGrid;