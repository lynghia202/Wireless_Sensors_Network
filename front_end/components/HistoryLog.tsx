
import React from 'react';
import { History } from 'lucide-react';
import { HistoryEvent } from '../types';

interface HistoryLogProps {
  history: HistoryEvent[];
}

const HistoryLog: React.FC<HistoryLogProps> = ({ history }) => {
  return (
    <div className="bg-white rounded-2xl shadow-sm border border-slate-100 overflow-hidden">
      <div className="p-6 border-b border-slate-100 bg-slate-50/50">
        <h3 className="text-lg font-semibold flex items-center gap-2 text-slate-800">
          <History className="h-5 w-5 text-slate-500" /> Lịch Sử Sự Kiện
        </h3>
      </div>
      <div className="max-h-[400px] overflow-y-auto">
        {history.length === 0 ? (
          <div className="p-8 text-center text-slate-400">Chưa có sự kiện nào được ghi nhận.</div>
        ) : (
          <table className="w-full text-sm text-left">
            <thead className="text-xs text-slate-500 uppercase bg-slate-50 sticky top-0">
              <tr>
                <th className="px-6 py-3 font-medium">Thời gian</th>
                <th className="px-6 py-3 font-medium">Sự kiện</th>
                <th className="px-6 py-3 font-medium text-right">Trạng thái</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100">
              {history.map((event, index) => (
                <tr key={index} className="hover:bg-slate-50 transition-colors">
                  <td className="px-6 py-4 text-slate-500">
                    {new Date(event.timestamp * 1000).toLocaleString('vi-VN')}
                  </td>
                  <td className="px-6 py-4 font-medium text-slate-900">
                    {event.message}
                  </td>
                  <td className="px-6 py-4 text-right">
                    <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium ${event.isDanger ? 'bg-red-100 text-red-800' : 'bg-emerald-100 text-emerald-800'}`}>
                      {event.isDanger ? 'CẢNH BÁO' : 'THÔNG TIN'}
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
};

export default HistoryLog;
