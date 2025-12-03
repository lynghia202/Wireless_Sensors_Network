
import React from 'react';
import { Settings, Link, Save } from 'lucide-react';
import { SettingsPayload } from '../types';

interface SettingsPanelProps {
  show: boolean;
  isConnected: boolean;
  hostIp: string;
  onHostIpChange: (e: React.ChangeEvent<HTMLInputElement>) => void;
  settingsForm: SettingsPayload;
  setSettingsForm: React.Dispatch<React.SetStateAction<SettingsPayload>>;
  onSave: () => void;
}

const SettingsPanel: React.FC<SettingsPanelProps> = ({ 
  show, 
  isConnected, 
  hostIp, 
  onHostIpChange, 
  settingsForm, 
  setSettingsForm, 
  onSave 
}) => {
  if (!show) return null;

  return (
    <div className="bg-white rounded-2xl shadow-lg border border-slate-200 p-6 animate-in slide-in-from-top-4 fade-in duration-200">
      <h3 className="text-lg font-semibold mb-4 flex items-center gap-2 text-slate-800">
        <Settings className="h-5 w-5 text-indigo-500" /> Cấu hình Hệ thống
      </h3>
      
      {/* IP Config */}
      <div className="mb-6 p-4 bg-indigo-50 rounded-xl border border-indigo-100">
        <div className="flex items-center justify-between mb-2">
          <label className="text-sm font-medium text-indigo-900 flex items-center gap-2">
            <Link className="h-4 w-4" /> Địa chỉ IP Thiết bị
          </label>
          <span className={`text-xs px-2 py-0.5 rounded-full border ${isConnected ? 'text-emerald-700 bg-emerald-50 border-emerald-200' : 'text-rose-700 bg-rose-50 border-rose-200'}`}>
              {isConnected ? "Đã kết nối" : "Không tìm thấy thiết bị"}
          </span>
        </div>
        <input 
          type="text" 
          value={hostIp}
          onChange={onHostIpChange}
          placeholder="Ví dụ: 192.168.1.20 (Để trống nếu chạy trên ESP32)"
          className="w-full p-2 border border-indigo-200 bg-white rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none text-sm"
        />
      </div>

      <div className={`grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 transition-opacity duration-300 ${!isConnected ? 'opacity-50 pointer-events-none' : ''}`}>
        <div className="space-y-2">
          <label className="text-sm font-medium text-slate-600">Ngưỡng Nhiệt độ (°C)</label>
          <input 
            type="number" 
            value={settingsForm.tempThreshold}
            onChange={e => setSettingsForm({...settingsForm, tempThreshold: parseFloat(e.target.value)})}
            className="w-full p-2 border border-slate-200 bg-slate-50 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
          />
        </div>
        <div className="space-y-2">
          <label className="text-sm font-medium text-slate-600">Ngưỡng Khí Gas (PPM)</label>
          <input 
            type="number" 
            value={settingsForm.co2Threshold}
            onChange={e => setSettingsForm({...settingsForm, co2Threshold: parseInt(e.target.value)})}
            className="w-full p-2 border border-slate-200 bg-slate-50 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
          />
        </div>
        <div className="space-y-2">
          <label className="text-sm font-medium text-slate-600">Ngưỡng Lửa (Giá trị đo)</label>
          <input 
            type="number" 
            value={settingsForm.fireThreshold}
            onChange={e => setSettingsForm({...settingsForm, fireThreshold: parseInt(e.target.value)})}
            className="w-full p-2 border border-slate-200 bg-slate-50 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
          />
        </div>
        <div className="space-y-2">
          <label className="text-sm font-medium text-slate-600">Email Nhận Cảnh Báo</label>
          <input 
            type="email" 
            value={settingsForm.recipientEmail}
            onChange={e => setSettingsForm({...settingsForm, recipientEmail: e.target.value})}
            className="w-full p-2 border border-slate-200 bg-slate-50 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
          />
        </div>
      </div>
      <div className="mt-6 flex justify-end items-center gap-4">
        <button 
          onClick={onSave}
          disabled={!isConnected}
          className="flex items-center gap-2 bg-indigo-600 hover:bg-indigo-700 disabled:bg-slate-400 text-white px-6 py-2 rounded-lg font-medium transition-colors shadow-sm active:scale-95 transform"
        >
          <Save className="h-4 w-4" /> Lưu Cấu Hình
        </button>
      </div>
    </div>
  );
};

export default SettingsPanel;
