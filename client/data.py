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

    def get_users(self) -> List[User]:
        raise NotImplemented


class MessagesAccessorInterface:
    def get_messages(self) -> List[Message]:
        raise NotImplemented


