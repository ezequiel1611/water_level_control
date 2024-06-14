import tkinter as tk
from tkinter import ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial
import threading
import time
import re

class WaterTankControlApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Water Tank Control")
        
        self.serial_port = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)  # Ajusta el puerto y la velocidad según corresponda
        
        self.level_data = []
        self.qi_data = []
        self.qo_data = []
        
        self.setup_ui()
        self.update_data()
    
    def setup_ui(self):
        self.fig = Figure(figsize=(10, 6), dpi=100)
        
        self.ax1 = self.fig.add_subplot(311)
        self.ax1.set_title("Nivel del Líquido (WL)")
        self.ax1.set_ylabel("Nivel")
        
        self.ax2 = self.fig.add_subplot(312)
        self.ax2.set_title("Caudal de Entrada (Qi)")
        self.ax2.set_ylabel("Caudal")
        
        self.ax3 = self.fig.add_subplot(313)
        self.ax3.set_title("Caudal de Salida (Qo)")
        self.ax3.set_ylabel("Caudal")
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.info_frame = tk.Frame(self.root)
        self.info_frame.pack(side=tk.TOP, pady=10)
        
        self.dc_label = tk.Label(self.info_frame, text="PWM de la Bomba: ", font=("Helvetica", 16))
        self.dc_label.grid(row=0, column=0, padx=10)
        
        self.sp_label = tk.Label(self.info_frame, text="Setpoint del Nivel (cm): ", font=("Helvetica", 16))
        self.sp_label.grid(row=0, column=1, padx=10)

        self.avg_label = tk.Label(self.info_frame, text="Nivel (cm): ", font=("Helvetica", 16))
        self.avg_label.grid(row=0, column=2, padx=10)
        
        self.start_button = tk.Button(self.root, text="Start", bg="green", command=self.start_pump)
        self.start_button.pack(side=tk.LEFT, padx=20, pady=20)
        
        self.stop_button = tk.Button(self.root, text="Stop", bg="red", command=self.stop_pump)
        self.stop_button.pack(side=tk.RIGHT, padx=20, pady=20)
    
    def start_pump(self):
        self.serial_port.write(b'start\n')
    
    def stop_pump(self):
        self.serial_port.write(b'stop\n')
    
    def update_data(self):
        line = self.serial_port.readline().decode('utf-8').strip()
        if line:
            match = re.match(r'Qi=(?P<qi>\d+\.\d+)/Qo=(?P<qo>\d+\.\d+)/WL=(?P<wl>\d+\.\d+)/DC=(?P<dc>\d+)/SP=(?P<sp>\d+)', line)
            if match:
                qi = float(match.group('qi'))
                qo = float(match.group('qo'))
                wl = float(match.group('wl'))
                dc = int(match.group('dc'))
                sp = int(match.group('sp'))

                self.level_data.append(wl)
                self.qi_data.append(qi)
                self.qo_data.append(qo)
                
                self.level_data = self.level_data[-500:]
                self.qi_data = self.qi_data[-500:]
                self.qo_data = self.qo_data[-500:]
                
                avg_wl = sum(self.level_data[-10:]) / min(len(self.level_data), 10)

                self.dc_label.config(text=f"PWM de la Bomba: {dc}")
                self.sp_label.config(text=f"Setpoint del Nivel (cm): {sp}")
                self.avg_label.config(text=f"Nivel (cm): {avg_wl:.2f}")

                self.ax1.clear()
                self.ax1.plot(self.level_data)
                self.ax1.set_title("Nivel del Líquido (WL)")
                self.ax1.set_ylabel("Nivel")
                
                self.ax2.clear()
                self.ax2.plot(self.qi_data)
                self.ax2.set_title("Caudal de Entrada (Qi)")
                self.ax2.set_ylabel("Caudal")
                
                self.ax3.clear()
                self.ax3.plot(self.qo_data)
                self.ax3.set_title("Caudal de Salida (Qo)")
                self.ax3.set_ylabel("Caudal")
                
                self.canvas.draw()
        
        self.root.after(1000, self.update_data)

if __name__ == "__main__":
    root = tk.Tk()
    app = WaterTankControlApp(root)
    root.mainloop()
