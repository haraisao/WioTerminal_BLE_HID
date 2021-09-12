#
#
from tkinter import *
import tkinter.ttk as ttk
import ble

E2J = {'"': '@', '&': '^', '\'' : '&', '(' : '*', ')' : '(', 
      '=' : '_', '^' : '=', '~' : '+', '@' : '[', '`' : '{' , '[' : ']' , '{' : '}', 
      '+' : ':', ':' : '\'', '*' : '"', ']' : '\\',  '}' : '|' , 
      '\\' : 0x89, '|' : 0x88, '_' : 0x87}

class BLE_Term(object):
    def __init__(self):
        pass

    def connect(self):
        ble.ble_connect()
        return

    def disconnect(self):
        ble.ble_disconnect()
        return

    def write(self,v):
        ble.ble_message(v)
        return

class TouchPad(ttk.Frame):
    def __init__(self, master, mode=True):
        super().__init__(master,width=300,height=300)
        self.start_xy = None
        self.x_y  = None
        self.create_pane()
        self.propagate(False)
        self.pack()
        self.keyin=False
        self.pressBtn=None
        self.enable_jis=mode
        self.out_port=BLE_Term()
        print("enable_jis", self.enable_jis)

    def create_pane(self):
        self.pane=ttk.Frame(self, width=300, height=300, relief="groove")
        self.pane.bind("<Button-1>",self.move_start)
        self.pane.bind("<B1-Motion>",self.move_now)
        self.pane.bind("<ButtonRelease-1>",self.move_end)
        self.pane.place(x=0,y=0)

        self.buttonL = ttk.Button(self,text = "L", width=10, takefocus=0)
        self.buttonL.place(x=0,y=0)
        self.buttonL.bind("<Button-1>",self.move_start)
        self.buttonL.bind("<B1-Motion>",self.move_now)
        self.buttonL.bind("<ButtonRelease-1>",self.move_end)

        self.buttonR = ttk.Button(self,text = "R", width=10, takefocus=0)
        self.buttonR.place(x=230,y=0)
        self.buttonR.bind("<Button-1>",self.move_start)
        self.buttonR.bind("<B1-Motion>",self.move_now)
        self.buttonR.bind("<ButtonRelease-1>",self.move_end)

        s_r=ttk.Style()
        s_r.configure('Key.TButton', background="red")
        s_b=ttk.Style()

        s_b.map("KeyB.TButton",
             foreground=[('pressed','red'), ('active', 'blue')],
             background=[('pressed', '!disabled', 'black'), ('active', 'green')]
        )
        
        self.buttonKey = ttk.Button(self,text = "Connect", style='KeyB.TButton')
        self.buttonKey.place(x=110,y=0)
        self.buttonKey.bind("<Button-1>", self.keyin_set)
        self.buttonKey.bind("<KeyPress>", self.keyin)

    def keyin_set(self, event):
        if self.keyin :
            self.keyin = False
            self.out_port.disconnect()
            self.buttonKey['text'] = "Connect"
            print("keyin_set 0")
        else:
            self.keyin = True
            self.out_port.connect()
            self.buttonKey['text'] = "Disconnect"
            print("keyin_set 1")

    def keyin(self, event):
        if self.keyin:
            if event.char:
                if event.state == 0 or event.state == 1:
                    if self.enable_jis and event.char in E2J:
                        ch = E2J[event.char]
                        if type(ch) is int:
                            self.out_port.write(ch.to_bytes(1, 'big'))
                            print("SP1:", ch.to_bytes(1, 'big'))
                        else:
                            self.out_port.write(bytes(ch, 'UTF-8'))
                            print("SP2:", bytes(ch, 'UTF-8'))
                    else:
                        self.out_port.write(bytes(event.char, 'UTF-8'))
                        #print("[",bytes(event.char, 'UTF-8'),"]")
                elif event.state == 4:
                    self.out_port.write(bytes(event.char, 'UTF-8'))
                    #print("ST4:",bytes(event.char, 'UTF-8'))
                else:
                    print(event.char, event.state)
            else:
                if not event.keycode in (16, 17, 18):
                    if event.keycode == 37: # A_LEFT
                        self.out_port.write(b'\x1b\x5b\x44')
                    elif event.keycode == 38: # A_UP
                        self.out_port.write(b'\x1b\x5b\x41')
                    elif event.keycode == 39: # A_RIGHT
                        self.out_port.write(b'\x1b\x5b\x43')
                    elif event.keycode == 40: # A_DOWN
                        self.out_port.write(b'\x1b\x5b\x42')
                    elif event.keycode == 33: # PageUp
                        self.out_port.write(b'\x1b\x5b\x34\x7e')
                    elif event.keycode == 34: # PageDown
                        self.out_port.write(b'\x1b\x5b\x35\x7e')
                    elif event.keycode == 35: # END
                        self.out_port.write(b'\x1b\x5b\x33\x7e')
                    elif event.keycode == 36: # HOME
                        self.out_port.write(b'\x1b\x5b\x31\x7e')
                    elif event.keycode == 45: # INSERT
                        self.out_port.write(b'\x1b\x5b\x32\x7e')
                    elif event.keycode == 46: # DEL
                        self.out_port.write(b'\x7f')
                    elif event.keycode == 112: # F1
                        self.out_port.write(b'\x1b\x5b\x31\x31\x7e')
                    elif event.keycode == 113: # F2
                        self.out_port.write(b'\x1b\x5b\x31\x32\x7e')
                    elif event.keycode == 114: # F3
                        self.out_port.write(b'\x1b\x5b\x31\x33\x7e')
                    elif event.keycode == 115: # F4
                        self.out_port.write(b'\x1b\x5b\x31\x34\x7e')
                    elif event.keycode == 116: # F5
                        self.out_port.write(b'\x1b\x5b\x31\x35\x7e')
                    elif event.keycode == 117: # F6
                        self.out_port.write(b'\x1b\x5b\x31\x36\x7e')
                    elif event.keycode == 118: # F7
                        self.out_port.write(b'\x1b\x5b\x31\x37\x7e')
                    elif event.keycode == 119: # F8
                        self.out_port.write(b'\x1b\x5b\x31\x38\x7e')
                    elif event.keycode == 120: # F9
                        self.out_port.write(b'\x1b\x5b\x32\x30\x7e')
                    elif event.keycode == 244: # Hakaku/Zenkaku
                        self.out_port.write(b'\x8b')
                    elif event.keycode == 243: # Hakaku/Zenkaku
                        self.out_port.write(b'\x8b')
                    else:
                        print("FN:",event.keycode)

    def move_start(self,event):
        # マウスカーソルの座標取得
        self.start_xy = (event.x_root,event.y_root)
        # 位置情報取得
        place_info = event.widget.place_info()
        x = int(place_info['x'])
        y = int(place_info['y'])
        self.x_y = (x,y)
        try:
            #print("Push", event.widget.cget('text'))
            self.pressBtn=event.widget.cget('text')
            self.button_press(event)
        except:
            self.pressBtn=None

    def move_now(self,event):
        if self.start_xy is None:
            return
        # 移動距離を調べる
        distance = (event.x_root-self.start_xy[0], event.y_root-self.start_xy[1])
        x=(event.x_root-self.start_xy[0]) *5
        y=(event.y_root-self.start_xy[1]) *5

        data = self.mouse_move_cmd(x, y, 0)
        self.out_port.write(data)
        #print(data)
        #print(distance, self.pressBtn)
        self.start_xy = (event.x_root, event.y_root)

    def move_end(self,event):
        self.start_xy = None
        self.x_y = None
        place_info = event.widget.place_info()
        self.button_release(event)

    def button_press(self,event):
        data = self.mouse_move_cmd(-255, 0, 0)
        self.out_port.write(data)

    def button_release(self,event):
        data = self.mouse_move_cmd(-255, 0, 0)
        self.out_port.write(data)
        self.pressBtn=None

    def mouse_move_cmd(self, x, y, w=0):
        data = b'\x1b\x5b\x6d'
        if self.pressBtn == 'L':
            data += b'\x01'
        elif self.pressBtn == 'R':
            data += b'\x02'
        else:
            data += b'\x00'

        if x < -128 or y < -128 or w < -128:
            data += b'\x00\x00\x00\x7e'
        else:
            x = min(max(x + 128, 1), 255)
            y = min(max(y + 128, 1), 255)
            w = min(max(w + 128, 1), 255)
            data += x.to_bytes(1, 'big')
            data += y.to_bytes(1, 'big')
            data += w.to_bytes(1, 'big')
            data += b'\x7e'
        
        return data

if __name__ == '__main__':

    mode=False
    if len(sys.argv) > 1:
        mode=(sys.argv[1] == 'JP')

    master = Tk()
    master.title("TouchPad")
    master.geometry("300x300")
    TouchPad(master,  mode)
    master.mainloop()
    