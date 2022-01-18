from dis import show_code
from modulefinder import packagePathMap
from pydoc import text
import socket
import time

import PySimpleGUI as sg

HOST = '127.0.0.1'      # Standard loopback interface address (localhost)
PORT = 1234             # Port to listen on (non-privileged ports are > 1023)
is_provider = 0         #variable for provider recognition

class Package:
    def __init__(self, data='', pg_type=0, size=0, text=""):
        self.data = data
        self.pg_type = pg_type
        self.size = max(len(text), size)
        self.text = text

    def send(self, connection):
        package_recv = Package()

        self.serialize()
        connection.sendall(self.data)
        package_recv.data = connection.recv(1024)
        package_recv.deserialize()
        
        return package_recv

    
    def deserialize(self):

        if len(self.data) < 5:
            return

        self.pg_type = int(chr(self.data[0]))*10 + int(chr(self.data[1]))
        self.size = int(chr(self.data[2]))*10 + int(chr(self.data[3]))
        self.text = self.data[4:self.size+4].decode()

        return (self.pg_type, self.size, self.text)

    def serialize(self):
        self.data += str(self.pg_type//10)
        self.data += str(self.pg_type%10)
        self.data += str(self.size//10)
        self.data += str(self.size%10)
        self.data += self.text
        self.data = str.encode(self.data)

        return self.data

class Login_window:
    def __init__(self):
        col1=[[sg.Text("Adres")], [sg.Text("port")], [sg.Text("Użytkownik")]]
        col2=[[sg.InputText(key="address")], [sg.InputText(key="port")], [sg.InputText(key="text")]]

        self.layout = [[sg.Column(col1),sg.Column(col2), sg.Button("OK")]]
        self.text = ""

    def show(self):
            # Create the window
            window = sg.Window("Login", self.layout)
            global HOST
            global PORT
            global is_provider

            # Create an event loop
            while True:
                event, values = window.read()
                # End program if user closes window or
                # presses the OK button
                if event == "OK" or event == sg.WIN_CLOSED:
                    try:
                        HOST = values["address"]
                        PORT = int(values["port"])
                        self.text = values["text"]
                    except:
                        HOST = '127.0.0.1'  
                        PORT = 1234
                        self.text = 'username'

                    if self.text == "":
                        self.text = "user"
                        
                    break

            window.close()


class Annoucment:
    def __init__(self, text):
        self.text = text
        self.layout = [[sg.Text(self.text)], [sg.Button("OK"), sg.Button("EXIT")]]

    def show(self):
        # Create the window
        window = sg.Window("Client", self.layout)

        # Create an event loop
        while True:
            event, values = window.read()
            # End program if user closes window or
            # presses the OK button
            if event == "OK":
                window.close()
                return 0
            if event == "EXIT"  or event == sg.WIN_CLOSED:
                window.close()
                return 1

class Message:
    def __init__(self):
        self.text = text
        self.layout = [[sg.Text("Nowa wiadomość")], [sg.InputText(key="message")],[sg.Button("OK"), sg.Button("EXIT")]]


    def show(self):
        # Create the window
        window = sg.Window("Client", self.layout)

        # Create an event loop
        while True:
            event, values = window.read()
            # End program if user closes window or
            # presses the OK button
            if event == "OK":
                window.close()
                return values["message"]
            if event == "EXIT"  or event == sg.WIN_CLOSED:
                window.close()
                return 1 

        

class Client:
    def __init__(self, name):
        self.name = name
        self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self):
        try:
            self.connection.connect((HOST, PORT))
            return 0
        except:
            Annoucment("Nie udało się nawiązać połączenia").show()
            return 1

    def login(self):
        global is_provider

        package_recv = Package(pg_type=0, text = self.name).send(self.connection)

        text = "logged as " + package_recv.text
        popup = Annoucment(text)
        val = popup.show()

        if val == 1:
            return False
    
        if package_recv.text == "provider":
            is_provider = True
            return True
        elif package_recv.text == "client":
            is_provider = False
            return True
        else:
            return False   
        
    def recive(self):
        while True:
            package_recv = Package(pg_type=3, text = "get size of queue").send(self.connection)
            package_recv.deserialize()

            print(f"size of queue: {package_recv.text}")

            if int(package_recv.text) > 0:
                package_recv = Package(pg_type=4, text = "get item").send(self.connection)
                val = Annoucment(package_recv.text).show()

            else:
                val = Annoucment("No more items at the moment").show()

            if val == 1:
                return
            
            time.sleep(1)

    def send(self):
        while True:
            val = Message().show()

            #exit button
            if val == 1:
                return

            package_recv = Package(pg_type=2, text = val).send(self.connection)
            package_recv.deserialize()

            val = Annoucment(package_recv.text).show()

            if val == 1:
                return


        

if __name__ == "__main__":

    login_window = Login_window()
    login_window.show()

    print(f"ip: {HOST}, port: {PORT}, val:{login_window.text}")

    user = Client(login_window.text)
    user.connect()

    if user.login():
        if is_provider:
            user.send()
        else:
            user.recive()

        time.sleep(1)

