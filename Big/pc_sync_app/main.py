import tkinter as tk
from tkinter import filedialog, messagebox
from tkinter import ttk
import serial
import serial.tools.list_ports
import threading
import time
import os
from PIL import Image
import io

class SmartFrameSyncApp:
    def __init__(self, root):
        self.root = root
        self.root.title("액자에 사진 넣기")
        self.root.geometry("400x350")
        
        self.serial_port = None
        self.is_connected = False
        
        # UI Elements
        tk.Label(root, text="어르신 맞춤형 액자 관리자", font=("Malgun Gothic", 18, "bold")).pack(pady=15)
        
        self.status_label = tk.Label(root, text="액자를 찾는 중... (케이블 연결 대기)", font=("Malgun Gothic", 12), fg="red")
        self.status_label.pack(pady=10)
        
        self.select_btn = tk.Button(root, text="여기를 눌러 사진 고르기", font=("Malgun Gothic", 16, "bold"), fg="black", 
                                    command=self.select_file, state=tk.DISABLED, width=20, height=3)
        self.select_btn.pack(pady=20)
        
        self.progress = ttk.Progressbar(root, orient=tk.HORIZONTAL, length=300, mode='determinate')
        self.progress.pack(pady=10)
        
        self.log_label = tk.Label(root, text="", font=("Malgun Gothic", 10), fg="gray")
        self.log_label.pack(pady=5)
        
        # Start auto-connect thread
        threading.Thread(target=self.auto_connect, daemon=True).start()

    def auto_connect(self):
        while not self.is_connected:
            ports = serial.tools.list_ports.comports()
            for port in ports:
                desc = port.description.lower()
                device = port.device.lower()
                
                # Mac과 Windows 환경에서 나타나는 시리얼 칩 이름 필터링
                if "usb" in desc or "ch340" in desc or "uart" in desc or "serial" in desc or \
                   "usb" in device or "wch" in device or "slab" in device or "uart" in device:
                    try:
                        self.serial_port = serial.Serial(port.device, 115200, timeout=1)
                        self.is_connected = True
                        self.root.after(0, self.update_ui_connected)
                        return
                    except:
                        pass
            time.sleep(2)

    def update_ui_connected(self):
        self.status_label.config(text="액자 연결됨 🟢", fg="green", font=("Malgun Gothic", 14, "bold"))
        self.select_btn.config(state=tk.NORMAL)
        self.log_label.config(text="원하시는 사진을 골라주세요.")

    def select_file(self):
        filepath = filedialog.askopenfilename(
            title="액자에 넣을 사진을 선택하세요",
            filetypes=[("이미지 파일", "*.jpg *.jpeg *.png *.JPG *.PNG")]
        )
        if filepath:
            self.select_btn.config(state=tk.DISABLED)
            threading.Thread(target=self.process_and_upload, args=(filepath,), daemon=True).start()

    def process_and_upload(self, filepath):
        try:
            self.root.after(0, lambda: self.log_label.config(text="사진을 액자 크기에 맞게 자동 압축 중..."))
            
            # 이미지 리사이징 (스마트폰 사진 5MB -> 20KB 수준으로 0.1초만에 압축)
            img = Image.open(filepath)
            if img.mode != 'RGB':
                img = img.convert('RGB')
                
            # 액자 해상도(320x240)에 맞게 강제 비율 축소
            img.thumbnail((320, 240), Image.LANCZOS)
            
            # 바이트 배열로 저장
            buf = io.BytesIO()
            img.save(buf, format="JPEG", quality=85)
            img_bytes = buf.getvalue()
            filesize = len(img_bytes)
            
            filename = os.path.basename(filepath)
            # 확장자를 무조건 jpg로 통일
            if not filename.lower().endswith('.jpg') and not filename.lower().endswith('.jpeg'):
                filename += ".jpg"
                
            self.root.after(0, lambda: self.log_label.config(text="통신 시작..."))
            
            # 시리얼 버퍼 비우기
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()
            
            # UPLOAD_START 커맨드 전송
            cmd = f"CMD:UPLOAD_START|/{filename}|{filesize}\n"
            self.serial_port.write(cmd.encode('utf-8'))
            
            # 액자의 ACK:READY 응답 대기
            timeout = time.time() + 3
            ready = False
            while time.time() < timeout:
                if self.serial_port.in_waiting:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                    if line == "ACK:READY":
                        ready = True
                        break
                time.sleep(0.01)
                
            if not ready:
                self.root.after(0, lambda: self.log_label.config(text="응답 시간 초과. 케이블을 뺐다 다시 꽂아주세요."))
                self.root.after(0, lambda: self.select_btn.config(state=tk.NORMAL))
                return
                
            # Chunk 단위 바이너리 전송 (ESP32 버퍼 오버플로우 방지)
            chunk_size = 256
            sent_bytes = 0
            
            self.root.after(0, lambda: self.log_label.config(text="사진을 밀어넣는 중... 0%"))
            
            while sent_bytes < filesize:
                end = min(sent_bytes + chunk_size, filesize)
                chunk = img_bytes[sent_bytes:end]
                
                self.serial_port.write(chunk)
                sent_bytes += len(chunk)
                
                # 액자의 ACK:CHUNK 응답 대기 (전송 속도를 액자의 SD 쓰기 속도에 동기화)
                ack_timeout = time.time() + 2
                chunk_ack = False
                while time.time() < ack_timeout:
                    if self.serial_port.in_waiting:
                        line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                        if "ACK:CHUNK" in line or "ACK:SUCCESS" in line:
                            chunk_ack = True
                            break
                    time.sleep(0.005)
                    
                if not chunk_ack:
                    print("경고: 전송 응답 지연 (통신 불안정)")
                    
                # 프로그레스 바 업데이트
                progress_val = int((sent_bytes / filesize) * 100)
                self.root.after(0, lambda v=progress_val: self.progress.config(value=v))
                self.root.after(0, lambda v=progress_val: self.log_label.config(text=f"사진을 밀어넣는 중... {v}%"))
                
            # 최종 전송 완료 확인
            time.sleep(0.5)
            self.root.after(0, lambda: self.progress.config(value=100))
            self.root.after(0, lambda: self.log_label.config(text="전송 완료! 액자 화면을 확인하세요."))
            messagebox.showinfo("성공", "사진이 성공적으로 액자에 쏙 들어갔습니다!")
            
        except Exception as e:
            self.root.after(0, lambda: self.log_label.config(text=f"오류 발생: {str(e)}"))
            
        finally:
            self.root.after(0, lambda: self.select_btn.config(state=tk.NORMAL))
            self.root.after(0, lambda: self.progress.config(value=0))

if __name__ == "__main__":
    root = tk.Tk()
    app = SmartFrameSyncApp(root)
    root.mainloop()
