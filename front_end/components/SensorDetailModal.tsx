import React from 'react';
import { X, Thermometer, Droplets, Wind, Flame, AlertTriangle } from 'lucide-react';
import { SensorData, HistoryEvent } from '../types';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

export type SensorType = 'temp' | 'hum' | 'gas' | 'fire';

interface SensorDetailModalProps {
  isOpen: boolean;
  onClose: () => void;
  sensorType: SensorType;
  currentData: SensorData;
  chartData: any[];
  history: HistoryEvent[];
}

const SensorDetailModal: React.FC<SensorDetailModalProps> = ({ 
  isOpen, 
  onClose, 
  sensorType, 
  currentData, 
  chartData, 
  history 
}) => {
  if (!isOpen) return null;

  // Configurations based on sensor type
  const getConfig = () => {
    switch(sensorType) {
      case 'temp':
        return {
          title: 'Chi tiết Nhiệt độ',
          icon: <Thermometer className="h-6 w-6" />,
          color: '#f97316', // Orange
          bgClass: 'bg-orange-100 text-orange-600',
          dataKey: 'temp',
          value: `${currentData.temp.toFixed(1)}°C`,
          desc: 'Biến động nhiệt độ theo thời gian thực.',
          keyword: 'Nhiệt độ'
        };
      case 'hum':
        return {
          title: 'Chi tiết Độ ẩm',
          icon: <Droplets className="h-6 w-6" />,
          color: '#3b82f6', // Blue
          bgClass: 'bg-blue-100 text-blue-600',
          dataKey: 'hum',
          value: `${currentData.hum.toFixed(1)}%`,
          desc: 'Độ ẩm tương đối trong không khí.',
          keyword: 'Độ ẩm'
        };
      case 'gas':
        return {
          title: 'Chi tiết Khí Gas',
          icon: <Wind className="h-6 w-6" />,
          color: '#6366f1', // Indigo
          bgClass: 'bg-indigo-100 text-indigo-600',
          dataKey: 'gas',
          value: `${currentData.co2} PPM`,
          desc: 'Nồng độ khí dễ cháy hoặc khói.',
          keyword: 'Gas'
        };
      case 'fire':
        return {
          title: 'Cảm biến Lửa',
          icon: <Flame className="h-6 w-6" />,
          color: '#f43f5e', // Rose
          bgClass: 'bg-rose-100 text-rose-600',
          dataKey: 'fire',
          value: currentData.fire.toString(),
          desc: 'Giá trị Analog từ cảm biến hồng ngoại.',
          keyword: 'Lửa'
        };
    }
  };

  const config = getConfig();

  // Filter relevant history logs (simple keyword matching)
  const relevantHistory = history.filter(h => 
    h.message.toLowerCase().includes(config.keyword.toLowerCase()) || 
    (sensorType === 'fire' && h.message.includes('CHÁY')) ||
    (sensorType === 'gas' && h.message.includes('khói'))
  ).slice(0, 5);

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4">
      {/* Backdrop */}
      <div 
        className="absolute inset-0 bg-slate-900/40 backdrop-blur-sm transition-opacity"
        onClick={onClose}
      ></div>

      {/* Modal Content */}
      <div className="relative bg-white rounded-2xl shadow-2xl w-full max-w-2xl overflow-hidden animate-in zoom-in-95 duration-200">
        
        {/* Header */}
        <div className="px-6 py-4 border-b border-slate-100 flex items-center justify-between bg-slate-50/50">
          <div className="flex items-center gap-3">
            <div className={`p-2 rounded-lg ${config.bgClass}`}>
              {config.icon}
            </div>
            <div>
              <h3 className="text-lg font-bold text-slate-800">{config.title}</h3>
              <p className="text-xs text-slate-500">{config.desc}</p>
            </div>
          </div>
          <button 
            onClick={onClose}
            className="p-2 rounded-full hover:bg-slate-200 text-slate-400 hover:text-slate-600 transition-colors"
          >
            <X className="h-5 w-5" />
          </button>
        </div>

        {/* Body */}
        <div className="p-6">
          <div className="flex items-baseline gap-2 mb-6">
            <span className="text-4xl font-bold text-slate-900">{config.value}</span>
            <span className="text-sm font-medium text-slate-500 uppercase tracking-wide">Giá trị hiện tại</span>
          </div>

          {/* Chart */}
          <div className="h-[200px] w-full bg-slate-50 rounded-xl border border-slate-100 p-4 mb-6">
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart data={chartData}>
                <defs>
                  <linearGradient id={`color${config.dataKey}`} x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor={config.color} stopOpacity={0.2}/>
                    <stop offset="95%" stopColor={config.color} stopOpacity={0}/>
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#e2e8f0" />
                <XAxis dataKey="time" hide />
                <YAxis hide domain={['auto', 'auto']} />
                <Tooltip 
                  contentStyle={{ borderRadius: '8px', border: 'none', boxShadow: '0 4px 6px -1px rgba(0,0,0,0.1)' }}
                  formatter={(val: number) => [val, config.title]}
                  labelStyle={{ color: '#64748b' }}
                />
                <Area 
                  type="monotone" 
                  dataKey={config.dataKey} 
                  stroke={config.color} 
                  fill={`url(#color${config.dataKey})`} 
                  strokeWidth={2}
                />
              </AreaChart>
            </ResponsiveContainer>
          </div>

          {/* Mini History */}
          <div>
            <h4 className="text-sm font-semibold text-slate-700 mb-3 flex items-center gap-2">
              <AlertTriangle className="h-4 w-4" /> Sự kiện gần đây
            </h4>
            {relevantHistory.length > 0 ? (
              <div className="space-y-2">
                {relevantHistory.map((event, idx) => (
                  <div key={idx} className="flex items-start gap-3 p-3 rounded-lg bg-slate-50 border border-slate-100 text-sm">
                    <span className="text-xs text-slate-400 min-w-[60px]">{new Date(event.timestamp * 1000).toLocaleTimeString('vi-VN')}</span>
                    <span className={event.isDanger ? 'text-rose-600 font-medium' : 'text-slate-700'}>{event.message}</span>
                  </div>
                ))}
              </div>
            ) : (
              <p className="text-sm text-slate-400 italic">Không có sự kiện liên quan gần đây.</p>
            )}
          </div>
        </div>

      </div>
    </div>
  );
};

export default SensorDetailModal;