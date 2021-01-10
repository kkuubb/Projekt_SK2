from socket import socket
from threading import Thread
from typing import List

from models import User, Message


class AccessorInterface:
    def __init__(self):
        self.on_change = None

    def bind_on_change(self, function):
        self.on_change = function

    def call_on_change(self):
        if self.on_change:
            self.on_change()


class UsersAccessorInterface(AccessorInterface):

    def __init__(self):
        super().__init__()
        self.users: List[User] = []

    def get_users(self) -> List[User]:
        return self.users

    def clean(self):
        raise NotImplemented

    def store_user(self, user: User):
        raise NotImplemented


class UsersAccessor(UsersAccessorInterface):
    def __init__(self):
        super().__init__()

    def clean(self):
        self.users = []

    def store_user(self, user: User):
        self.users.append(user)
        self.call_on_change()


class MessagesAccessorInterface(AccessorInterface):
    def get_messages(self) -> List[Message]:
        raise NotImplemented

    def store_message(self, message):
        raise NotImplemented


class MessagesAccessor(MessagesAccessorInterface):
    def __init__(self):
        super().__init__()
        self.messages: List[Message] = []

    def get_messages(self) -> List[Message]:
        return self.messages

    def store_message(self, message):
        self.messages.append(message)
        self.call_on_change()


class Request:
    def __init__(self, data: str):
        self.data = data

    def get_action(self) -> str:
        return self.data.split(":")[0]

    def get_data(self):
        return self.data.split(":", 1)[1]

    def get_parts(self):
        s: str = self.get_data()
        parts = []
        while len(s):
            size = int(s[:3])
            parts.append(s[4:4 + size])
            s = s[4 + size:]
        print(self.get_data())
        return parts


class SocketConnector:

    def __init__(self, this_user_id: int, users_accessor: UsersAccessorInterface,
                 messages_accessor: MessagesAccessorInterface):
        self.this_user_id = this_user_id
        self.messages_accessor = messages_accessor
        self.users_accessor = users_accessor

    def run(self, port=1230):
        s = socket()
        s.connect(("localhost", port))

        def read():
            while (1):
                len = s.recv(5)
                if len[-1] != ord(':'):
                    print(f"Błąd {len}")
                    continue
                len = int(len[0:4])
                print(f"odczytano {len} znaków")
                request = Request(s.recv(len).decode('utf-8'))

                print(f"akcja: {request.get_action()}, dane: {request.get_data()}")

                if request.get_action() == 'newMessage':
                    data = request.get_parts()[0]
                    parts = data.split(",", 1)
                    message = Message(int(parts[0]), self.this_user_id, parts[1])
                    self.messages_accessor.store_message(message)

                if request.get_action() == 'your_id':
                    self.this_user_id = int(request.get_parts()[0])

                if request.get_action() == 'users':
                    self.users_accessor.clean()
                    for part in request.get_parts():
                        user_name, id = part.split(",")
                        id = int(id)
                        if id != self.this_user_id:
                            self.users_accessor.users.append(User(id, user_name))

                    self.users_accessor.call_on_change()
                    print(request.get_parts())

                if request.get_action() == 'loginUser':
                    s.send(b"0000:getUsers")

                if request.get_action() == 'logoutUser':
                    s.send(b"0000:getUsers")

        Thread(name='socket_receiver', target=read).start()
