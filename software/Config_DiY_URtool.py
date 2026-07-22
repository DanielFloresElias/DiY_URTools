import tkinter as tk
from tkinter import ttk, messagebox
import serial.tools.list_ports
import serial
import time 
import sys 

class URToolConfigApp:
    def __init__(self, master):
        self.master = master
        master.title("⚙️ Configurador de Xarxa UR Tool")
        master.geometry("450x650")
        master.resizable(False, False)

        # --- Variables ---
        self.selected_port = tk.StringVar(master)
        self.ip_var = tk.StringVar(master, value="192.168.1.50")
        self.mask_var = tk.StringVar(master, value="255.255.255.0")
        self.gw_var = tk.StringVar(master, value="192.168.1.1")
        self.baud_rate = 115200
        self.serial_connection = None

        # --- Crear la interfície ---
        self.create_widgets()

    def create_widgets(self):
        main_frame = ttk.Frame(self.master, padding="15")
        main_frame.pack(fill='both', expand=True)

        # Configuració del Port Sèrie
        serial_group = ttk.LabelFrame(main_frame, text="🔌 Configuració Port Sèrie", padding="10")
        serial_group.pack(fill="x", pady=10)

        # 1. Selector de Port Sèrie
        ttk.Label(serial_group, text="Port Sèrie:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(serial_group, textvariable=self.selected_port, state="readonly", width=30)
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        self.port_combobox['values'] = self.list_serial_ports()
        
        if self.port_combobox['values']:
            self.selected_port.set(self.port_combobox['values'][0])

        # Botó d'actualització
        ttk.Button(serial_group, text="Actualitzar Llista", command=self.update_port_list).grid(row=0, column=2, padx=5, pady=5)
        
        serial_group.columnconfigure(1, weight=1)

        # Configuració de Xarxa
        network_group = ttk.LabelFrame(main_frame, text="🌐 Paràmetres de Xarxa (Ethernet)", padding="10")
        network_group.pack(fill="x", pady=10)

        # 2. IP
        ttk.Label(network_group, text="Adreça IP:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        ttk.Entry(network_group, textvariable=self.ip_var).grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        # 3. MASK
        ttk.Label(network_group, text="Subnet Mask:").grid(row=1, column=0, padx=5, pady=5, sticky="w")
        ttk.Entry(network_group, textvariable=self.mask_var).grid(row=1, column=1, padx=5, pady=5, sticky="ew")

        # 4. Gateway
        ttk.Label(network_group, text="Gateway:").grid(row=2, column=0, padx=5, pady=5, sticky="w")
        ttk.Entry(network_group, textvariable=self.gw_var).grid(row=2, column=1, padx=5, pady=5, sticky="ew")
        
        network_group.columnconfigure(1, weight=1)

        # Botons d'Acció
        action_frame = ttk.Frame(main_frame)
        action_frame.pack(fill="x", pady=15)
        
        ttk.Button(action_frame, text="✔️ Enviar i Guardar (SAVE)", command=self.send_and_save, style='Accent.TButton').pack(side=tk.LEFT, expand=True, fill='x', padx=5)
        ttk.Button(action_frame, text="❓ Estat (STATUS)", command=self.get_status).pack(side=tk.LEFT, expand=True, fill='x', padx=5)

        # Àrea de Missatges
        message_group = ttk.LabelFrame(main_frame, text="💬 Registre de Comandes", padding="10")
        message_group.pack(fill="both", expand=True)
        
        self.message_text = tk.Text(message_group, height=8, state='disabled', wrap='word', bg='#e0e0e0', bd=0)
        self.message_text.pack(fill="both", expand=True)

        self.add_message("Benvingut! Selecciona el port i configura els paràmetres.")
        self.add_message("RECORDA: Després de guardar, has de reiniciar l'ESP32 per aplicar els canvis.")

    def list_serial_ports(self):
        """Llista els ports sèrie disponibles."""
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def update_port_list(self):
        """Actualitza la llista de ports al desplegable."""
        ports = self.list_serial_ports()
        self.port_combobox['values'] = ports
        if ports:
            self.selected_port.set(ports[0])
            self.add_message(f"Llista de ports actualitzada: {ports}")
        else:
            self.selected_port.set('')
            self.add_message("No s'han trobat ports sèrie.")

    def open_serial_connection(self):
        """Obre la connexió serial, reinicia l'ESP32 i espera el boot."""
        port = self.selected_port.get()
        if not port:
            messagebox.showerror("Error de Connexió", "Si us plau, selecciona un port sèrie.")
            return False

        if self.serial_connection and self.serial_connection.is_open:
            return True

        try:
            # 1. Obrir la connexió. Això normalment reinicia l'ESP32.
            self.serial_connection = serial.Serial(port, self.baud_rate, timeout=1)
            self.serial_connection.reset_input_buffer()
            self.serial_connection.reset_output_buffer()
            
            self.add_message(f"Connexió oberta a {port}. Reiniciant ESP32...")

            # 2. Esperar que l'ESP32 arrenqui completament (3 segons per seguretat).
            boot_delay = 3
            time.sleep(boot_delay) 
            
            # 3. Netejar els missatges de boot loader (com el 'rst:0x1')
            self.serial_connection.reset_input_buffer()
            self.add_message(f"Esperat {boot_delay} segons. ESP32 llest per rebre comandes.")
            
            return True
        except serial.SerialException as e:
            messagebox.showerror("Error de Connexió", f"No es pot obrir el port {port}: {e}")
            self.serial_connection = None
            return False

    def close_serial_connection(self):
        """Tanca la connexió serial."""
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
            self.serial_connection = None
            self.add_message("Connexió serial tancada.")

    def send_command(self, command, expect_response=True):
        """Envia una comanda per serial i llegeix la resposta."""
        if not self.open_serial_connection():
            return ""

        try:
            # Enviar la comanda amb salt de línia
            self.serial_connection.write(f"{command}\n".encode())
            self.add_message(f"Enviant: {command}")
            
            if not expect_response:
                return ""

            # Llegir la resposta del dispositiu
            response_bytes = b""
            start_time = time.time() 
            
            # Temps màxim per a la resposta
            while (time.time() - start_time) < 3:
                if self.serial_connection.in_waiting > 0:
                    response_bytes += self.serial_connection.read(self.serial_connection.in_waiting)
                elif response_bytes and self.serial_connection.in_waiting == 0:
                    break
                time.sleep(0.01)

            if response_bytes:
                # Decodificació amb gestió d'errors per evitar errors 0x84, etc.
                response = response_bytes.decode('utf-8', errors='replace') 
                
                # Netejar la sortida
                cleaned_response = "\n".join([line.strip() for line in response.split('\n') if line.strip()])
                self.add_message(f"Resposta del dispositiu:\n{cleaned_response}")
                return cleaned_response
            else:
                self.add_message("Advertència: No s'ha rebut cap resposta de l'ESP32 (o estava ocupat).")
                return ""

        except Exception as e:
            messagebox.showerror("Error Serial", f"Error en enviar la comanda: {e}")
            self.close_serial_connection()
            return ""
    
    def add_message(self, message):
        """Afegeix un missatge al registre."""
        self.message_text.config(state='normal')
        self.message_text.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {message}\n")
        self.message_text.see(tk.END)
        self.message_text.config(state='disabled')

    def send_and_save(self):
        """Envia IP, MASK, GW i la comanda SAVE."""
        ip = self.ip_var.get()
        mask = self.mask_var.get()
        gw = self.gw_var.get()
        
        if not all([ip.count('.') == 3, mask.count('.') == 3, gw.count('.') == 3]):
            messagebox.showerror("Error de Format", "Si us plau, comprova que les adreces IP/MASK/GW tinguin el format correcte (e.g., 192.168.1.1).")
            return

        self.add_message("Iniciant procés de configuració...")
        
        # Enviament de comandes sequencials
        self.send_command(f"IP {ip}")
        self.send_command(f"MASK {mask}")
        self.send_command(f"GW {gw}")
        self.send_command("SAVE")
        
        self.add_message("✅ Configuració enviada i guardada a l'EEPROM de l'ESP32.")
        self.add_message("⭐⭐ REINICIA MANUALMENT l'ESP32 per aplicar la nova configuració Ethernet! ⭐⭐")
        
    def get_status(self):
        """Envia la comanda STATUS."""
        self.send_command("STATUS")


if __name__ == "__main__":
    root = tk.Tk()
    
    # Configuració de l'estil per a una aparença moderna
    try:
        style = ttk.Style()
        # Utilitzem 'clam' si està disponible, sinó fem servir 'default'
        if sys.platform == 'win32' or sys.platform == 'darwin':
             style.theme_use('clam')
        else:
             style.theme_use('default')
        
        style.configure('Accent.TButton', background='#007bff', foreground='white', font=('Arial', 10, 'bold'))
        style.map('Accent.TButton', background=[('active', '#0056b3')])
    except Exception:
        pass

    app = URToolConfigApp(root)
    root.mainloop()