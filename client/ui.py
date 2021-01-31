from typing import List

import tkinter as tk
from models import Message


def messages(root, messages_array: List[Message], user_id):
    for m in messages_array:
        is_mine = m.sender == user_id
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

        style_element = styles['element']['mine'] if is_mine else styles['element']['not_mine']
        style_grid = styles['grid']['mine'] if is_mine else styles['grid']['not_mine']
        frame = tk.Frame(root, width=300)
        message = tk.Label(frame, text=m.body, **style_element)
        message.pack(side='right' if is_mine else 'left', expand=True, fill='both')
        frame.grid(sticky='s')
        root.columnconfigure(1, weight=1)

