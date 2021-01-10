class User:
    def __init__(self, id: int, name: str):
        self.id = id
        self.name = name


class Message:
    def __init__(self, sender: int, receiver: int, body: str):
        self.receiver = receiver
        self.sender = sender
        self.body = body