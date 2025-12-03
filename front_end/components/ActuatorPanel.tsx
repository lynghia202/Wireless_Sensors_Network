import React from 'react';
import { Fan, Droplets, Bell } from 'lucide-react';
import { SensorData } from '../types';

interface ActuatorPanelProps {
  data: SensorData;
}

const ActuatorPanel: React.FC<ActuatorPanelProps> = ({ data }) => {
  return (
    <div className="lg:col-span-1 space-y-6">
       {/* Fan Status */}
       <div className="bg-white p-5 rounded-xl shadow-sm border border-slate-100 flex items-center justify-between">
          <div className="flex items-center gap-4">
            <div className={`p-3 rounded-full ${data.fan ? 'bg-indigo-100 text-indigo-600 animate-spin-slow' : 'bg-slate-100 text-slate-400'}`}>
              <Fan className="h-6 w-6" />
            </div>
            <div>
              <h4 className="font-semibold text-slate-900">Quạt Hút</h4>
              <p className="text-xs text-slate-500">Điều khiển tự động</p>
            </div>
          </div>
          <div className={`h-3 w-3 rounded-full transition-all duration-300 ${data.fan ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]' : 'bg-slate-300'}`}></div>
       </div>

       {/* Pump Status */}
       <div className="bg-white p-5 rounded-xl shadow-sm border border-slate-100 flex items-center justify-between">
          <div className="flex items-center gap-4">
            <div className={`p-3 rounded-full ${data.pump ? 'bg-blue-100 text-blue-600' : 'bg-slate-100 text-slate-400'}`}>
              <Droplets className="h-6 w-6" />
            </div>
            <div>
              <h4 className="font-semibold text-slate-900">Máy Bơm</h4>
              <p className="text-xs text-slate-500">Hệ thống chữa cháy</p>
            </div>
          </div>
          <div className={`h-3 w-3 rounded-full transition-all duration-300 ${data.pump ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]' : 'bg-slate-300'}`}></div>
       </div>

       {/* Buzzer Status */}
       <div className="bg-white p-5 rounded-xl shadow-sm border border-slate-100 flex items-center justify-between">
          <div className="flex items-center gap-4">
            <div className={`p-3 rounded-full ${data.buzzer ? 'bg-rose-100 text-rose-600 animate-pulse' : 'bg-slate-100 text-slate-400'}`}>
              <Bell className="h-6 w-6" />
            </div>
            <div>
              <h4 className="font-semibold text-slate-900">Còi Báo Động</h4>
              <p className="text-xs text-slate-500">Cảnh báo âm thanh</p>
            </div>
          </div>
          <div className={`h-3 w-3 rounded-full transition-all duration-300 ${data.buzzer ? 'bg-rose-500 shadow-[0_0_8px_rgba(244,63,94,0.6)]' : 'bg-slate-300'}`}></div>
       </div>
    </div>
  );
};

export default ActuatorPanel;