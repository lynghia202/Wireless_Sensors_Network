
import React from 'react';
import { WifiOff } from 'lucide-react';
import { 
  XAxis, 
  YAxis, 
  CartesianGrid, 
  Tooltip, 
  ResponsiveContainer, 
  AreaChart, 
  Area 
} from 'recharts';

interface LiveChartProps {
  data: { time: string; temp: number; gas: number }[];
  isConnected: boolean;
}

const LiveChart: React.FC<LiveChartProps> = ({ data, isConnected }) => {
  return (
    <div className="lg:col-span-2 bg-white rounded-2xl shadow-sm border border-slate-100 p-6">
      <h3 className="text-lg font-semibold mb-6 text-slate-800 flex items-center justify-between">
        <span>Biểu Đồ Trực Tiếp</span>
        {!isConnected && <span className="text-xs font-normal text-rose-500 bg-rose-50 px-2 py-1 rounded flex items-center gap-1"><WifiOff size={12}/> Gián đoạn</span>}
      </h3>
      <div className="h-[300px] w-full">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data}>
            <defs>
              <linearGradient id="colorTemp" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#f97316" stopOpacity={0.2}/>
                <stop offset="95%" stopColor="#f97316" stopOpacity={0}/>
              </linearGradient>
              <linearGradient id="colorGas" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#6366f1" stopOpacity={0.2}/>
                <stop offset="95%" stopColor="#6366f1" stopOpacity={0}/>
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" />
            <XAxis dataKey="time" hide />
            <YAxis yAxisId="left" orientation="left" stroke="#94a3b8" fontSize={12} tickFormatter={(val) => `${val}°C`}/>
            <YAxis yAxisId="right" orientation="right" stroke="#94a3b8" fontSize={12} />
            <Tooltip 
              contentStyle={{ backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.1)' }}
              itemStyle={{ fontSize: '12px' }}
              formatter={(value, name) => [value, name === 'Temp' ? 'Nhiệt độ' : 'Khí Gas']}
            />
            <Area 
              yAxisId="left" 
              type="monotone" 
              dataKey="temp" 
              stroke="#f97316" 
              strokeWidth={2} 
              fillOpacity={1} 
              fill="url(#colorTemp)" 
              name="Temp"
              isAnimationActive={isConnected} 
            />
            <Area 
              yAxisId="right" 
              type="monotone" 
              dataKey="gas" 
              stroke="#6366f1" 
              strokeWidth={2} 
              fillOpacity={1} 
              fill="url(#colorGas)" 
              name="Gas"
              isAnimationActive={isConnected}
            />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};

export default LiveChart;
