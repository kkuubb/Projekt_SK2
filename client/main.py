import tkinter as tk

from data import UsersAccessorInterface, MessagesAccessorInterface
from models import Message
from fake_data import FakeMessagesAccessor, FakeUsersAccessor


class Application(tk.Frame):
    def __init__(self,
                 user_id: int,
                 users_accessor: UsersAccessorInterface,
                 messages_accessor: MessagesAccessorInterface,
                 master=None
                 ):
        super().__init__(master)
        self.frame_users_nav = None
        self.user_id = user_id
        self.messages_accessor = messages_accessor
        self.users_accessor = users_accessor
        self.users_accessor.bind_on_change(self.create_widgets)
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
        for u in self.users_accessor.get_users():
            button = tk.Button(frame, text=u.name, fg="red")
            button.grid()
        frame.grid(row=0, column=1, sticky='e')


root = tk.Tk()
app = Application(1, FakeUsersAccessor(), FakeMessagesAccessor(), master=root)
app.mainloop()
