from typing import List


class User:
    def __init__(self, id: int, name: str):
        self.id = id
        self.name = name


class Message:
    def __init__(self, sender: int, receiver: int, body: str):
        self.receiver = receiver
        self.sender = sender
        self.body = body


class AccessorInterface:
    def __init__(self):
        self.on_change = None

    def bind_on_change(self, function):
        self.on_change = function

    def call_on_change(self):
        if self.on_change:
            self.on_change()


class UsersAccessorInterface(AccessorInterface):

    def get_users(self) -> List[User]:
        raise NotImplemented


class MessagesAccessorInterface:
    def get_messages(self) -> List[Message]:
        raise NotImplemented


