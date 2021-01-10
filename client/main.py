import tkinter as tk
from typing import Callable

from data import UsersAccessorInterface, MessagesAccessorInterface, SocketConnector, MessagesAccessor, UsersAccessor
from models import Message, User
from fake_data import FakeMessagesAccessor, FakeUsersAccessor


class Application(tk.Frame):
    def __init__(self,
                 socket: SocketConnector,
                 master=None
                 ):
        super().__init__(master)
        self.frame_users_nav = None
        self.socket = socket
        self.user_id = socket.this_user_id
        self.messages_accessor = socket.messages_accessor
        self.users_accessor = socket.users_accessor
        self.users_accessor.bind_on_change(self.create_widgets)
        self.messages_accessor.bind_on_change(self.create_widgets)
        self.master = master
        self.grid()
        self.create_widgets()

    def is_message_mine(self, message: Message) -> bool:
        return message.sender == self.user_id

    def create_widgets(self):
        chat = tk.Frame(self, width=200)

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
            frame = tk.Frame(chat, width=300, padx=50)
            message = tk.Label(frame, text=m.body, **style_element)
            message.pack(side='right' if mine else 'left')
            frame.grid(padx=30, sticky='e')

        chat.grid(row=0, column=0)
        self.users_nav_bar()

    def users_nav_bar(self):
        if self.frame_users_nav:
            self.frame_users_nav.destroy()
        frame = tk.Frame(
            self,
            relief=tk.RAISED,
            borderwidth=1
        )
        self.frame_users_nav = frame

        def filter_users(user: User):
            return user.id != self.socket.this_user_id

        for u in filter(filter_users, self.users_accessor.get_users()):
            button = tk.Button(frame, text=u.name, fg="red")
            button.grid()
        frame.grid(row=0, column=1, sticky='e')


connector = SocketConnector(1, UsersAccessor(), MessagesAccessor())
connector.run()

root = tk.Tk()
app = Application(connector, master=root)
app.mainloop()
