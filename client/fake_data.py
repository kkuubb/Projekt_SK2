from random import random
from threading import Timer
from typing import List

from data import MessagesAccessorInterface, Message, UsersAccessorInterface, User


class FakeMessagesAccessor(MessagesAccessorInterface):
    def get_messages(self) -> List[Message]:
        return [Message((id%2), ((id+1)%2), f"message{id}") for id in range(5)]


class FakeUsersAccessor(UsersAccessorInterface):
    def __init__(self):
        super().__init__()
        self.users: List[User] = [User(id, f"user{id}") for id in range(5)]
        self.start_timer()

    def get_users(self) -> List[User]:
        return self.users

    def start_timer(self):
        def handle_change():
            start = int(random() * 10)
            len = int(random() * 15)
            self.users = [User(id, f"user{id}") for id in range(start, start+len)]
            self.call_on_change()
            self.start_timer()

        Timer(1, handle_change).start()