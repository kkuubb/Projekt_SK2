import tkinter as tk

from data import SocketConnector, MessagesAccessor, UsersAccessor
from models import Message, User


class Application(tk.Frame):
    def __init__(self,
                 socket: SocketConnector,
                 master=None
                 ):
        super().__init__(master)
        self.frame_users_nav = None
        self.chat_view = None
        self.input_frame = None
        self.socket = socket
        self.user_id = socket.this_user_id
        self.messages_accessor = socket.messages_accessor
        self.users_accessor = socket.users_accessor
        self.users_accessor.bind_on_change(self.create_widgets)
        self.messages_accessor.bind_on_change(self.create_widgets)
        self.master = master
        self.main_frame = tk.Frame(self)
        self.main_frame.rowconfigure(0, weight=0)
        self.main_frame.rowconfigure(1, weight=1)


        # self.main_frame.columnconfigure(0, weight=1)
        # self.main_frame.columnconfigure(1, weight=1)
        self.main_frame.grid()
        self.pack(expand=True)
        self.create_widgets()

    def is_message_mine(self, message: Message) -> bool:
        return message.sender == self.user_id

    def create_widgets(self):
        if self.chat_view:
            self.chat_view.destroy()

        chat = tk.Canvas(self.main_frame, width=200, height=400)
        self.chat_view = tk.Frame(chat)
        self.render_messages(chat)
        chat.grid(row=0, column=0)
        self.users_nav_bar()
        if self.input_frame:
            self.input_frame.destroy()

        self.get_input_element(self.main_frame).grid(row=1, column=0)

    def get_input_element(self, master):
        self.input_frame = tk.Frame(master=master, background='red', bd=3, width=50, height=800)
        #
        input_field = tk.Text(self.input_frame, height=2, width=50)
        input_field.pack(side=tk.LEFT)
        input_field.focus_force()
        input_field.bind("<Return>", lambda x: self.socket.send_message(input_field.get("1.0", "end-1c")))
        #
        send_button = tk.Button(self.main_frame, text="send", command=lambda: self.socket.send_message(input_field.get("1.0", "end-1c")))
        send_button.grid(row=1, column=1)
        return self.input_frame

    def render_messages(self, master):
        for m in self.messages_accessor.get_messages():
            mine = self.is_message_mine(m)

            styles = {
                'element': {
                    'mine': {'bg': 'gray', 'fg': 'black'},
                    'not_mine': {'bg': 'black', 'fg': 'white'}
                },
                'grid': {
                    'mine': {'sticky': 'w'},
                    'not_mine': {'sticky': 'e'}
                }
            }

            style_element = styles['element']['mine'] if mine else styles['element']['not_mine']
            style_grid = styles['grid']['mine'] if mine else styles['grid']['not_mine']
            frame = tk.Label(master, width=300, padx=50)
            message = tk.Label(frame, text=m.body, **style_element)
            message.pack(side='right' if mine else 'left', expand=True, fill='both')
            frame.grid(padx=30, sticky='Ne')

    def users_nav_bar(self):
        if self.frame_users_nav:
            self.frame_users_nav.destroy()
        frame = tk.Frame(
            self.main_frame,
            borderwidth=1
        )
        self.frame_users_nav = frame

        def filter_users(user: User):
            return user.id != self.socket.this_user_id

        for u in filter(filter_users, self.users_accessor.get_users()):
            def handle_click(user_id):
                print(f"nacisnieto {user_id}")
                return lambda: self.socket.select_user(user_id)
            button = tk.Button(frame, text=f"{u.name} {u.id}", fg="red", command=handle_click(u.id))
            button.grid()

        frame.grid(row=0, column=1, sticky='e')


connector = SocketConnector(1, UsersAccessor(), MessagesAccessor())
connector.run()

root = tk.Tk()
# root.attributes('-zoomed', True)
# root.columnconfigure(1, weight=1, minsize=800)
root.wm_minsize(500,500)
# root.wm_maxsize(500, 500)
# root.rowconfigure(0, weight=2)
# root.columnconfigure(0, weight=1)
# root.columnconfigure(0, weight=1)
# root.grid_rowconfigure(0, weight=1)
# root.grid_columnconfigure(0, weight=1)
app = Application(connector, master=root)
app.mainloop()
