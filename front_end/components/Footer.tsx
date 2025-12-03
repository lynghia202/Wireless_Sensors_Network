
import React from 'react';
import { ShieldCheck, GraduationCap } from 'lucide-react';

const Footer: React.FC = () => {
  return (
    <footer className="mt-12 border-t border-slate-200 bg-white py-8">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col md:flex-row items-center justify-between gap-4">
        <div className="flex items-center gap-4">
            <div className="h-12 w-12 bg-indigo-600 rounded-xl flex items-center justify-center shadow-lg shadow-indigo-200 transform rotate-3 hover:rotate-0 transition-all duration-300">
              <span className="text-white font-bold text-xl">N6</span>
            </div>
            <div className="flex flex-col">
              <h4 className="font-bold text-slate-900 flex items-center gap-2">
                Mạng Cảm Biến - Nhóm 6 <ShieldCheck className="h-4 w-4 text-indigo-500" />
              </h4>
              <p className="text-xs text-slate-500 flex items-center gap-1">
                <GraduationCap className="h-3 w-3" /> Học viện Công nghệ Bưu chính Viễn thông
              </p>
            </div>
        </div>
        
        <div className="text-center md:text-right">
          <p className="text-sm font-medium text-slate-700">Báo cáo Bài tập lớn</p>
          <p className="text-xs text-slate-400 mt-1">
            © {new Date().getFullYear()} SafeGuard IoT System. All rights reserved.
          </p>
        </div>
      </div>
    </footer>
  );
};

export default Footer;
