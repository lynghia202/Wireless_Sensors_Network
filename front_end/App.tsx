import React, { useState, useEffect, useCallback } from 'react';
import { SensorData, HistoryEvent, SettingsPayload, ConnectionState } from './types';
import Header from './components/Header';
import StatusBanner from './components/StatusBanner';
import SettingsPanel from './components/SettingsPanel';
import SensorGrid from './components/SensorGrid';
import ActuatorPanel from './components/ActuatorPanel';
import LiveChart from './components/LiveChart';
import HistoryLog from './components/HistoryLog';
import Footer from './components/Footer';
import LoginScreen from './components/LoginScreen';
import ChangePasswordModal from './components/ChangePasswordModal';

const DEFAULT_DATA: SensorData = {
  fire: 0,
  co2: 0,
  temp: 0,
  hum: 0,
  buzzer: false,
  pump: false,
  fan: false,
  recipientEmail: "---",
  ssid: "---",
  fireThresh: 3000,
  co2Thresh: 1500,
  tempThresh: 50.0
};
const MAX_ATTEMPTS = 5;
const App: React.FC = () => {
  const [isAuthenticated, setIsAuthenticated] = useState(false);
  const [showPasswordModal, setShowPasswordModal] = useState(false);
  const [mustChangePassword, setMustChangePassword] = useState(false);
  
  const [isLocked, setIsLocked] = useState(false);
  const [lockoutEndTime, setLockoutEndTime] = useState<number | null>(null);
  const [failedAttempts, setFailedAttempts] = useState(0);

  const [data, setData] = useState<SensorData>(DEFAULT_DATA);
  const [history, setHistory] = useState<HistoryEvent[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionState>(ConnectionState.DISCONNECTED);
  const [hostIp, setHostIp] = useState(() => localStorage.getItem('esp32_host_ip') || "");
  
  const [showSettings, setShowSettings] = useState(false);
  const [settingsForm, setSettingsForm] = useState<SettingsPayload>({
    fireThreshold: 3000,
    co2Threshold: 1500,
    tempThreshold: 50.0,
    recipientEmail: ""
  });

  const [chartData, setChartData] = useState<{ time: string; temp: number; gas: number; hum: number; fire: number }[]>([]);

  const getApiUrl = useCallback((endpoint: string) => {
    if (hostIp) {
      let base = hostIp.trim().replace(/\/$/, "");
      if (!base.match(/^https?:\/\//)) {
        base = `http://${base}`;
      }
      return `${base}${endpoint}`;
    }
    return endpoint; 
  }, [hostIp]);

  // ===== KIỂM TRA TRẠNG THÁI BAN ĐẦU =====
  useEffect(() => {
    const checkServerStatus = async () => {
      try {
        const url = getApiUrl('/auth-check');
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 2000);
        
        const res = await fetch(url, { signal: controller.signal });
        clearTimeout(timeoutId);

        if (res.ok) {
          const data = await res.json();

          // Xử lý lockout
          if (data.locked && data.remaining > 0) {
            setIsLocked(true);
            setLockoutEndTime(Date.now() + (data.remaining * 1000));
          }

          // Xử lý session - CHỈ KHI KHÔNG BỊ KHÓA
          if (!data.locked && data.authenticated) {
            setIsAuthenticated(true);
            setConnectionStatus(ConnectionState.CONNECTED);
          }
        }
      } catch (e) {
        console.log("Không thể kết nối đến server.");
      }
    };

    checkServerStatus();
  }, [getApiUrl]);

  // ===== LOGIN HANDLER =====
  const handleLogin = async (u: string, p: string): Promise<boolean> => {
    try {
        const url = getApiUrl('/login');
        const res = await fetch(url, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ password: p })
        });

        const authData = await res.json();
        
        //  CẬP NHẬT failedAttempts từ backend
        if (authData.failedAttempts !== undefined) {
            setFailedAttempts(authData.failedAttempts);
        }
        
        if (authData.locked) {
            setIsLocked(true);
            if (authData.remaining) {
                setLockoutEndTime(Date.now() + (authData.remaining * 1000));
            }
            return false;
        }

        if (authData.authenticated) {
            setIsAuthenticated(true);
            setIsLocked(false);
            setLockoutEndTime(null);
            setFailedAttempts(0); //  RESET khi login thành công
            
            if (authData.isDefault) {
                setMustChangePassword(true);
                setShowPasswordModal(true);
            }
            return true;
        }

        // Login thất bại nhưng chưa bị khóa
        return false;
        
    } catch (e) {
        console.error("Login error:", e);
        return false;
    }
};

  const handleChangePassword = async (oldP: string, newP: string): Promise<boolean> => {
    try {
        const loginUrl = getApiUrl('/login');
        const checkRes = await fetch(loginUrl, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ password: oldP })
        });
        
        if (!checkRes.ok) return false;
        const checkData = await checkRes.json();
        if (!checkData.authenticated) return false;

        const settingsUrl = getApiUrl('/settings');
        const updateRes = await fetch(settingsUrl, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ password: newP })
        });

        if (updateRes.ok) {
            setMustChangePassword(false);
            return true;
        }
        return false;
    } catch (e) {
        return false;
    }
  };

  // ===== LOGOUT HANDLER - ĐÃ SỬA =====
  const handleLogout = async () => {
    try {
      // GỌI API LOGOUT ĐỂ XÓA SESSION Ở BACKEND
      const url = getApiUrl('/logout');
      await fetch(url, { 
        method: 'POST',
        headers: {'Content-Type': 'application/json'}
      });
    } catch (e) {
      console.error("Logout error:", e);
    } finally {
      // Xóa state ở frontend
      setIsAuthenticated(false);
      setShowSettings(false);
      setData(DEFAULT_DATA);
      setConnectionStatus(ConnectionState.DISCONNECTED);
    }
  };

  const handleUnlock = () => {
    setIsLocked(false);
    setLockoutEndTime(null);
  };

  // ===== DATA FETCHING =====
  const fetchData = useCallback(async () => {
    if (!isAuthenticated) return; 

    try {
      const url = getApiUrl('/data');
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 2000); 
      
      const res = await fetch(url, { signal: controller.signal });
      clearTimeout(timeoutId);

      if (!res.ok) throw new Error('Network response was not ok');

      const jsonData: SensorData = await res.json();
      
      setData(jsonData);
      setConnectionStatus(ConnectionState.CONNECTED);

      setChartData(prev => {
        const now = new Date();
        const timeStr = now.toLocaleTimeString('vi-VN', { hour12: false });
        const newPoint = { 
          time: timeStr, 
          temp: jsonData.temp, 
          gas: jsonData.co2,
          hum: jsonData.hum,
          fire: jsonData.fire
        };
        const newBuffer = [...prev, newPoint];
        return newBuffer.slice(-20);
      });

      if (!showSettings) {
        setSettingsForm(prev => ({
          ...prev,
          fireThreshold: jsonData.fireThresh ?? prev.fireThreshold,
          co2Threshold: jsonData.co2Thresh ?? prev.co2Threshold,
          tempThreshold: jsonData.tempThresh ?? prev.tempThreshold,
          recipientEmail: jsonData.recipientEmail || prev.recipientEmail
        }));
      }

    } catch (error) {
      setConnectionStatus(ConnectionState.DISCONNECTED);
    }
  }, [getApiUrl, showSettings, isAuthenticated]);

  useEffect(() => {
    if (!isAuthenticated) return;
    const intervalId = setInterval(fetchData, 1000);
    fetchData();
    return () => clearInterval(intervalId);
  }, [fetchData, isAuthenticated]);

  const fetchHistory = useCallback(async () => {
    if (connectionStatus === ConnectionState.DISCONNECTED || !isAuthenticated) return;
    try {
      const res = await fetch(getApiUrl('/history'));
      if (res.ok) {
        const jsonHistory: HistoryEvent[] = await res.json();
        setHistory(jsonHistory.sort((a, b) => b.timestamp - a.timestamp));
      }
    } catch (e) { }
  }, [connectionStatus, getApiUrl, isAuthenticated]);

  useEffect(() => {
    if (!isAuthenticated) return;
    const intervalId = setInterval(fetchHistory, 5000);
    fetchHistory();
    return () => clearInterval(intervalId);
  }, [fetchHistory, isAuthenticated]);

  const handleHostIpChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = e.target.value;
    setHostIp(val);
    localStorage.setItem('esp32_host_ip', val);
  };

  const handleSaveSettings = async () => {
    if (connectionStatus !== ConnectionState.CONNECTED) {
      alert("Không thể lưu: Mất kết nối với thiết bị.");
      return;
    }
    try {
      const res = await fetch(getApiUrl('/settings'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(settingsForm)
      });
      if (res.ok) {
        alert("Lưu cấu hình thành công!");
        setShowSettings(false);
      } else {
        alert("Lưu cấu hình thất bại.");
      }
    } catch (e) {
      alert("Lỗi khi gửi cấu hình tới thiết bị.");
    }
  };

  const isDanger = data.buzzer; 
  const isConnected = connectionStatus === ConnectionState.CONNECTED;

  if (!isAuthenticated) {
    return (
      <LoginScreen 
        onLogin={handleLogin}
        isLocked={isLocked}
        lockoutEndTime={lockoutEndTime}
        onUnlock={handleUnlock}
        failedAttempts={failedAttempts}
        maxAttempts={MAX_ATTEMPTS}
      />
    );
  }

  return (
    <div className="min-h-screen bg-slate-100 text-slate-800 flex flex-col">
      <Header 
        isConnected={isConnected} 
        ssid={data.ssid} 
        isDanger={isDanger} 
        showSettings={showSettings} 
        onToggleSettings={() => setShowSettings(!showSettings)}
        onLogout={handleLogout}
        onChangePassword={() => setShowPasswordModal(true)}
      />

      <main className="flex-grow max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8 space-y-6 w-full">
        <StatusBanner isConnected={isConnected} isDanger={isDanger} />

        <SettingsPanel 
          show={showSettings}
          isConnected={isConnected}
          hostIp={hostIp}
          onHostIpChange={handleHostIpChange}
          settingsForm={settingsForm}
          setSettingsForm={setSettingsForm}
          onSave={handleSaveSettings}
        />

        <SensorGrid 
          data={data} 
          settings={settingsForm} 
          isConnected={isConnected}
          chartData={chartData}
          history={history}
        />

        <div className={`grid grid-cols-1 lg:grid-cols-3 gap-6 transition-opacity duration-500 ${!isConnected ? 'opacity-60 grayscale' : ''}`}>
          <ActuatorPanel data={data} />
          <LiveChart data={chartData} isConnected={isConnected} />
        </div>

        <HistoryLog history={history} />
      </main>

      <Footer />

      <ChangePasswordModal 
        isOpen={showPasswordModal}
        onClose={() => { if (!mustChangePassword) setShowPasswordModal(false); }}
        isForced={mustChangePassword}
        onChangePassword={handleChangePassword}
      />
    </div>
  );
};

export default App;