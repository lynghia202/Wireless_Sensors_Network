
import React, { useState, useRef, useEffect } from 'react';
import { Activity, Wifi, WifiOff, Settings, User, LogOut, KeyRound, ChevronDown } from 'lucide-react';

interface HeaderProps {
  isConnected: boolean;
  ssid: string;
  isDanger: boolean;
  showSettings: boolean;
  onToggleSettings: () => void;
  onLogout: () => void;
  onChangePassword: () => void;
}

const Header: React.FC<HeaderProps> = ({ 
  isConnected, 
  ssid, 
  isDanger, 
  showSettings, 
  onToggleSettings,
  onLogout,
  onChangePassword
}) => {
  const [showUserMenu, setShowUserMenu] = useState(false);
  const menuRef = useRef<HTMLDivElement>(null);

  // Close menu when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(event.target as Node)) {
        setShowUserMenu(false);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  return (
    <header className="bg-white shadow-sm sticky top-0 z-30">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        {/* Brand */}
        <div className="flex items-center space-x-3">
          <div className={`p-2 rounded-lg transition-colors duration-300 ${isConnected ? (isDanger ? 'bg-red-100 text-red-600' : 'bg-indigo-100 text-indigo-600') : 'bg-slate-200 text-slate-500'}`}>
            <Activity className="h-6 w-6" />
          </div>
          <div>
            <h1 className="text-xl font-bold tracking-tight text-slate-900 hidden sm:block">SafeGuard IoT</h1>
            <h1 className="text-xl font-bold tracking-tight text-slate-900 sm:hidden">SafeGuard</h1>
            <div className="text-xs flex items-center gap-2">
              {isConnected ? (
                <span className="flex items-center text-emerald-600 font-medium animate-pulse"><Wifi className="h-3 w-3 mr-1" /> {ssid}</span>
              ) : (
                <span className="flex items-center text-slate-400 font-medium"><WifiOff className="h-3 w-3 mr-1" /> Mất kết nối</span>
              )}
            </div>
          </div>
        </div>
        
        {/* Actions */}
        <div className="flex items-center gap-2">
          <button 
            onClick={onToggleSettings}
            className={`p-2 rounded-full transition-all duration-200 ${showSettings ? 'bg-indigo-50 text-indigo-600' : 'text-slate-500 hover:bg-slate-100'}`}
            title="Cấu hình"
          >
            <Settings className="h-6 w-6" />
          </button>

          {/* User Menu */}
          <div className="relative" ref={menuRef}>
            <button 
              onClick={() => setShowUserMenu(!showUserMenu)}
              className="flex items-center gap-2 pl-2 pr-1 py-1 rounded-full hover:bg-slate-50 border border-transparent hover:border-slate-200 transition-all"
            >
               <div className="bg-indigo-100 p-1.5 rounded-full text-indigo-600">
                 <User className="h-5 w-5" />
               </div>
               <ChevronDown className="h-4 w-4 text-slate-400" />
            </button>

            {showUserMenu && (
              <div className="absolute right-0 mt-2 w-48 bg-white rounded-xl shadow-lg border border-slate-100 py-1 animate-in fade-in slide-in-from-top-2">
                <div className="px-4 py-2 border-b border-slate-50">
                  <p className="text-xs font-semibold text-slate-500 uppercase">Tài khoản</p>
                  <p className="text-sm font-medium text-slate-900 truncate">Admin</p>
                </div>
                <button 
                  onClick={() => { onChangePassword(); setShowUserMenu(false); }}
                  className="w-full text-left px-4 py-2 text-sm text-slate-700 hover:bg-slate-50 flex items-center gap-2"
                >
                  <KeyRound className="h-4 w-4" /> Đổi mật khẩu
                </button>
                <button 
                  onClick={onLogout}
                  className="w-full text-left px-4 py-2 text-sm text-rose-600 hover:bg-rose-50 flex items-center gap-2"
                >
                  <LogOut className="h-4 w-4" /> Đăng xuất
                </button>
              </div>
            )}
          </div>
        </div>
      </div>
    </header>
  );
};

export default Header;
