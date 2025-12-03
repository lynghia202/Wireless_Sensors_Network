import React, { useState } from 'react';
import { X, Lock, Check, AlertTriangle, Loader2 } from 'lucide-react';

interface ChangePasswordModalProps {
  isOpen: boolean;
  onClose: () => void;
  isForced: boolean;
  onChangePassword: (oldP: string, newP: string) => Promise<boolean>; // Async return
}

const ChangePasswordModal: React.FC<ChangePasswordModalProps> = ({ isOpen, onClose, isForced, onChangePassword }) => {
  const [oldPass, setOldPass] = useState('');
  const [newPass, setNewPass] = useState('');
  const [confirmPass, setConfirmPass] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState(false);
  const [loading, setLoading] = useState(false);

  if (!isOpen) return null;

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    if (newPass.length < 4) {
      setError('Mật khẩu mới phải có ít nhất 4 ký tự.');
      return;
    }

    if (newPass !== confirmPass) {
      setError('Mật khẩu xác nhận không khớp.');
      return;
    }

    if (newPass === oldPass) {
        setError('Mật khẩu mới không được trùng với mật khẩu cũ.');
        return;
    }

    setLoading(true);
    try {
      const result = await onChangePassword(oldPass, newPass);
      if (result) {
        setSuccess(true);
        setOldPass('');
        setNewPass('');
        setConfirmPass('');
        if (isForced) {
          setTimeout(() => onClose(), 1500);
        } else {
          setTimeout(() => {
              setSuccess(false);
              onClose();
          }, 1500);
        }
      } else {
        setError('Mật khẩu cũ không chính xác hoặc lỗi hệ thống.');
      }
    } catch (e) {
      setError('Lỗi kết nối tới thiết bị.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 z-[60] flex items-center justify-center p-4">
      <div className="absolute inset-0 bg-slate-900/60 backdrop-blur-sm"></div>
      
      <div className="relative bg-white rounded-2xl shadow-2xl w-full max-w-md overflow-hidden animate-in zoom-in-95">
        <div className="px-6 py-4 border-b border-slate-100 bg-slate-50 flex items-center justify-between">
          <h3 className="text-lg font-bold text-slate-800 flex items-center gap-2">
            <Lock className="h-5 w-5 text-indigo-600" /> 
            {isForced ? 'Yêu cầu Đổi Mật Khẩu' : 'Đổi Mật Khẩu'}
          </h3>
          {!isForced && (
            <button onClick={onClose} className="p-1 rounded-full hover:bg-slate-200 text-slate-400">
              <X className="h-5 w-5" />
            </button>
          )}
        </div>

        <div className="p-6">
          {isForced && !success && (
            <div className="mb-4 bg-yellow-50 border border-yellow-200 text-yellow-800 text-sm p-3 rounded-lg flex items-start gap-2">
              <AlertTriangle className="h-5 w-5 shrink-0" />
              <p>Vì lý do bảo mật, bạn cần thay đổi mật khẩu mặc định (admin) ngay trong lần đăng nhập đầu tiên.</p>
            </div>
          )}

          {success ? (
            <div className="py-8 text-center">
              <div className="mx-auto h-12 w-12 bg-green-100 text-green-600 rounded-full flex items-center justify-center mb-3">
                <Check className="h-6 w-6" />
              </div>
              <h4 className="text-lg font-bold text-slate-800">Thành công!</h4>
              <p className="text-slate-500">Mật khẩu đã được cập nhật.</p>
            </div>
          ) : (
            <form onSubmit={handleSubmit} className="space-y-4">
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Mật khẩu cũ</label>
                <input 
                  type="password" 
                  value={oldPass}
                  onChange={(e) => setOldPass(e.target.value)}
                  className="w-full p-2 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
                  required
                />
              </div>
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Mật khẩu mới</label>
                <input 
                  type="password" 
                  value={newPass}
                  onChange={(e) => setNewPass(e.target.value)}
                  className="w-full p-2 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
                  required
                />
              </div>
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Xác nhận mật khẩu mới</label>
                <input 
                  type="password" 
                  value={confirmPass}
                  onChange={(e) => setConfirmPass(e.target.value)}
                  className="w-full p-2 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 outline-none"
                  required
                />
              </div>

              {error && <p className="text-sm text-rose-600 font-medium">{error}</p>}

              <button 
                type="submit"
                disabled={loading}
                className="w-full bg-indigo-600 hover:bg-indigo-700 disabled:opacity-70 text-white font-medium py-2 rounded-lg transition-colors flex justify-center items-center gap-2"
              >
                {loading && <Loader2 className="h-4 w-4 animate-spin" />}
                Cập nhật mật khẩu
              </button>
            </form>
          )}
        </div>
      </div>
    </div>
  );
};

export default ChangePasswordModal;