import React, { useState, useEffect } from 'react';
import { ShieldCheck, Lock, User, AlertTriangle, Loader2 } from 'lucide-react';

interface LoginScreenProps {
  onLogin: (u: string, p: string) => Promise<boolean>;
  isLocked: boolean;
  lockoutEndTime: number | null;
  onUnlock: () => void;
  failedAttempts: number;
  maxAttempts: number;
}

const LoginScreen: React.FC<LoginScreenProps> = ({ 
  onLogin, 
  isLocked, 
  lockoutEndTime, 
  onUnlock,
  failedAttempts,
  maxAttempts
}) => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [timeLeft, setTimeLeft] = useState(0);
  const [loading, setLoading] = useState(false);

  // Countdown timer for lockout
  useEffect(() => {
    if (!isLocked || !lockoutEndTime) return;

    const timer = setInterval(() => {
      const remaining = Math.ceil((lockoutEndTime - Date.now()) / 1000);
      if (remaining <= 0) {
        onUnlock();
        setTimeLeft(0);
      } else {
        setTimeLeft(remaining);
      }
    }, 1000);

    return () => clearInterval(timer);
  }, [isLocked, lockoutEndTime, onUnlock]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (isLocked) return;

    setLoading(true);

    try {
      await onLogin(username, password);
      // Không cần xử lý thêm, parent component sẽ lo
    } catch (e) {
      console.error('Login error:', e);
    } finally {
      setLoading(false);
    }
  };

  const remainingAttempts = Math.max(0, maxAttempts - failedAttempts);

  return (
    <div className="min-h-screen bg-slate-100 flex items-center justify-center p-4">
      <div className="max-w-md w-full bg-white rounded-2xl shadow-xl overflow-hidden">
        <div className="bg-indigo-600 p-8 text-center">
          <div className="mx-auto h-16 w-16 bg-white/20 backdrop-blur-md rounded-xl flex items-center justify-center mb-4">
             <ShieldCheck className="h-8 w-8 text-white" />
          </div>
          <h2 className="text-2xl font-bold text-white">SafeGuard IoT</h2>
          <p className="text-indigo-100 text-sm mt-1">Hệ thống giám sát an toàn</p>
        </div>

        <div className="p-8">
          {isLocked ? (
             <div className="bg-rose-50 border border-rose-200 rounded-xl p-6 text-center animate-in fade-in zoom-in">
                <AlertTriangle className="h-10 w-10 text-rose-500 mx-auto mb-3" />
                <h3 className="text-rose-700 font-bold mb-2">Tạm thời bị khóa!</h3>
                <p className="text-rose-600 text-sm mb-4">Bạn đã nhập sai quá nhiều lần.</p>
                <div className="text-3xl font-mono font-bold text-rose-800">
                  {timeLeft}s
                </div>
                <p className="text-xs text-rose-400 mt-2">Vui lòng thử lại sau.</p>
             </div>
          ) : (
            <form onSubmit={handleSubmit} className="space-y-5">
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Tên đăng nhập</label>
                <div className="relative">
                  <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none">
                    <User className="h-5 w-5 text-slate-400" />
                  </div>
                  <input
                    type="text"
                    required
                    value={username}
                    onChange={(e) => setUsername(e.target.value)}
                    className="block w-full pl-10 pr-3 py-2 border border-slate-300 rounded-lg focus:ring-indigo-500 focus:border-indigo-500 outline-none transition-colors"
                    placeholder="Nhập username (mặc định: admin)"
                  />
                </div>
              </div>

              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Mật khẩu</label>
                <div className="relative">
                  <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none">
                    <Lock className="h-5 w-5 text-slate-400" />
                  </div>
                  <input
                    type="password"
                    required
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    className="block w-full pl-10 pr-3 py-2 border border-slate-300 rounded-lg focus:ring-indigo-500 focus:border-indigo-500 outline-none transition-colors"
                    placeholder="Nhập password"
                  />
                </div>
              </div>

              {failedAttempts > 0 && !isLocked && (
                 <div className="text-sm text-orange-600 bg-orange-50 p-3 rounded-lg border border-orange-200 text-center flex items-center justify-center gap-2">
                    <AlertTriangle className="h-4 w-4 flex-shrink-0" />
                    <span>Sai thông tin đăng nhập. Còn lại <b>{remainingAttempts}</b> lần thử.</span>
                 </div>
              )}

              <button
                type="submit"
                disabled={loading}
                className="w-full flex justify-center py-2.5 px-4 border border-transparent rounded-lg shadow-sm text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 disabled:opacity-70 transition-all"
              >
                {loading ? <Loader2 className="h-5 w-5 animate-spin" /> : 'Đăng nhập'}
              </button>

              <div className="mt-4 p-3 bg-blue-50 border border-blue-200 rounded-lg">
                <p className="text-xs text-blue-600 text-center leading-relaxed">
                  💡 <b>Quên mật khẩu?</b> Bấm giữ nút reset trên thiết bị <b>5 giây</b> để khôi phục mật khẩu mặc định.
                </p>
              </div>
            </form>
          )}
        </div>
        <div className="bg-slate-50 px-8 py-4 border-t border-slate-100 text-center">
            <p className="text-xs text-slate-400">© Nhóm 6 - Mạng Cảm Biến</p>
        </div>
      </div>
    </div>
  );
};

export default LoginScreen;