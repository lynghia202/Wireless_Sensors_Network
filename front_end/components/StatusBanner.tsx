
import React from 'react';
import { AlertTriangle, CheckCircle2, WifiOff } from 'lucide-react';

interface StatusBannerProps {
  isConnected: boolean;
  isDanger: boolean;
}

const StatusBanner: React.FC<StatusBannerProps> = ({ isConnected, isDanger }) => {
  if (!isConnected) {
    return (
      <div className="rounded-2xl p-6 flex items-center justify-between shadow-sm bg-slate-100 border border-slate-300 text-slate-500">
          <div>
          <h2 className="text-2xl font-bold flex items-center gap-2">
              <WifiOff className="h-6 w-6" /> Mất kết nối
          </h2>
          <p className="mt-1">Không thể liên lạc với ESP32. Vui lòng kiểm tra nguồn điện hoặc mạng WiFi.</p>
        </div>
      </div>
    );
  }

  return (
    <div className={`rounded-2xl p-6 flex items-center justify-between shadow-sm transition-all duration-500 ${isDanger ? 'bg-rose-500 text-white ring-4 ring-rose-200' : 'bg-white border border-slate-200'}`}>
      <div>
        <h2 className={`text-2xl font-bold flex items-center gap-2 ${isDanger ? 'text-white' : 'text-slate-800'}`}>
          {isDanger ? (
            <>CẢNH BÁO NGUY HIỂM <span className="animate-ping absolute inline-flex h-3 w-3 rounded-full bg-white opacity-75"></span></>
          ) : 'Hệ thống Bình thường'}
        </h2>
        <p className={`mt-1 ${isDanger ? 'text-rose-100' : 'text-slate-500'}`}>
          {isDanger ? 'Cảm biến phát hiện mức nguy hiểm. Đã kích hoạt thiết bị.' : 'Các chỉ số môi trường đang ở mức an toàn.'}
        </p>
      </div>
      <div className={`p-4 rounded-full ${isDanger ? 'bg-white/20' : 'bg-emerald-100'}`}>
        {isDanger ? <AlertTriangle className="h-8 w-8 text-white" /> : <CheckCircle2 className="h-8 w-8 text-emerald-600" />}
      </div>
    </div>
  );
};

export default StatusBanner;
