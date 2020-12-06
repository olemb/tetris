#!/usr/bin/env python3
"""
Tetris for Python / Tkinter

Ole Martin Bjorndalen
https://github.com/olemb/tetris/

http://tetris.wikia.com/wiki/Tetris_Guideline
"""
import copy
import random
from dataclasses import dataclass, replace
import tkinter


shapes = {
    #
    # Example (T)
    #
    # 0123  ....   ....
    # 4567  .##. = .56. = '56a9' (draw order)
    # 89ab  .##.   .9a.
    # cdef  ....   ....
    #

    # http://tetris.wikia.com/wiki/SRS
    # http://tetris.wikia.com/wiki/Orientation

    # Rotations are listed clockwise.
    'O': ['56a9', '6a95', 'a956', '956a'],
    'I': ['4567', '26ae', 'ba98', 'd951'],
    'J': ['0456', '2159', 'a654', '8951'],
    'L': ['2654', 'a951', '8456', '0159'],
    'T': ['1456', '6159', '9654', '4951'],
    'Z': ['0156', '2659', 'a954', '8451'],
    'S': ['1254', 'a651', '8956', '0459'],
}


@dataclass
class Piece:
    shape: str
    rot: int = 0
    x: int = 0
    y: int = 0


def move_piece(piece, *, rot=0, dx=0, dy=0):
    rot = (piece.rot + rot) % 4
    x = piece.x + dx
    y = piece.y + dy
    return replace(piece, rot=rot, x=x, y=y)


def get_piece_blocks(piece):
    for char in shapes[piece.shape][piece.rot % 4]:
        y, x = divmod(int(char, 16), 4)
        yield (piece.x + x, piece.y - y)


def piece_fits(field, piece):
    w = len(field[0])
    h = len(field)

    for x, y in get_piece_blocks(piece):
        if x < 0 or x >= w:
            return False
        elif y < 0 or y >= h:
            return False
        elif field[y][x]:
            return False
    else:
        return True


def remove_full_rows(field):
    return [row.copy() for row in field if not all(row)]


def random_shape_bag():
    bag = list(shapes)

    # Shuffle bag first time.
    while True:
        random.shuffle(bag)
        # First bag must start with I, L, J or T.
        if bag[0] in 'IJLT':
            break

    while True:
        yield from bag
        random.shuffle(bag)


def get_wallkicks(piece, *, rot=0):
    return [
        move_piece(piece, rot=rot, dx=dx, dy=dy)
        for (dx, dy) in [(0, 0), (-1, 0), (1, 0), (0, -1)]
    ]


class Tetris:
    def __init__(self, width=10, height=16):
        self.width = width
        self.height = height
        self.field = []
        self.piece = None
        self.game_over = False
        self.lines = 0

        self._pad_field()
        self._random_shapes = random_shape_bag()
        self.piece = self._get_next_piece()

    def _get_next_piece(self):
        shape = next(self._random_shapes)
        rot = 0
        centered = self.width // 2 - 2
        top = self.height - 1
        x = centered
        y = top
        return Piece(shape, rot, x, y)

    def _pad_field(self):
        while len(self.field) < self.height:
            self.field.append([''] * self.width)

    def _freeze_piece(self):
        char = self.piece.shape.lower()
        for (x, y) in get_piece_blocks(self.piece):
            self.field[y][x] = char

    def _remove_full_rows(self):
        field = remove_full_rows(self.field)
        lines = len(self.field) - len(field)
        self.lines += lines
        self.field = field
        self._pad_field()

    def _place_new_piece(self):
        self.piece = self._get_next_piece()
        if not piece_fits(self.field, self.piece):
            self.game_over = True

    def _freeze(self):
        self._freeze_piece()
        self._remove_full_rows()
        self._place_new_piece()

    def _move(self, *, rot=0, dx=0, dy=0) -> bool:
        if rot:
            candidate_pieces = get_wallkicks(self.piece, rot=rot)
        else:
            candidate_pieces = [move_piece(self.piece, dx=dx, dy=dy)]

        for piece in candidate_pieces:
            if piece_fits(self.field, piece):
                self.piece = piece
                moved = True
                break
        else:
            moved = False

        tried_to_move_down = dy == -1
        if tried_to_move_down and not moved:
            self._freeze()

        return moved

    def move(self, move):
        if not self.game_over:
            args = {
                'left': {'dx': -1},
                'right': {'dx': 1},
                'down': {'dy': -1},
                'rotleft': {'rot': -1},
                'rotright': {'rot': 1},
            }[move]
            self._move(**args)

    def get_visible_field(self):
        field = copy.deepcopy(self.field)

        if not self.game_over:
            char = self.piece.shape
            for x, y in get_piece_blocks(self.piece):
                field[y][x] = char

        return field


# http://unsoundscapes.com/elm-flatris.html
# https://github.com/skidding/flatris/blob/master/src/constants/tetromino.js
colors = {
    'I': '#3cc7d6',  # Cyan.
    'O': '#fbb414',  # Yellow.
    'T': '#b04497',  # Magenta.
    'J': '#3993d0',  # Blue.
    'L': '#ed652f',  # Orange.
    'S': '#95c43d',  # Green.
    'Z': '#e84138',  # Red.
    '':  '#ecf0f1',   # (Background color.)
}


class BlockDisplay(tkinter.Canvas):
    def __init__(self, parent, width, height, blocksize=40):
        tkinter.Canvas.__init__(self, parent,
                                width=width*blocksize,
                                height=height*blocksize)
        self.blocksize = blocksize
        self.width = width
        self.height = height
        self.blocks = {}
        self.colors = True

    def _create_block(self, x, y):
        # Flip Y coordinate.
        y = self.height - y - 1
        size = self.blocksize

        return self.create_rectangle(x * size,
                                     y * size,
                                     (x+1) * size,
                                     (y+1) * size,
                                     fill='',
                                     outline='')


    def set_block(self, x, y, char):
        try:
            block = self.blocks[(x, y)]
        except KeyError:
            block = self._create_block(x, y)
            self.blocks[(x, y)] = block

        if self.colors:
            fill = colors[char.upper()]
        else:
            if char == '':
                fill = colors['']
            elif char.isupper():
                fill = 'black'
            else:
                fill = 'gray50'

        self.itemconfigure(block, fill=fill)

    def clear(self):
        self.blocks = {}
        self.itemconfigure('all', fill='')

    def pause(self):
        self.itemconfigure('all', stipple='gray50')

    def resume(self):
        self.itemconfigure('all', stipple='')


class TetrisTk:
    def __init__(self):

        self.tk = tk = tkinter.Tk()
        self.tk.title('Tetris')

        self.tetris = Tetris()
        self.display = BlockDisplay(tk, self.tetris.width, self.tetris.height)
        self.display.pack(side=tkinter.TOP, fill=tkinter.X)

        self.score_view = tkinter.Label(self.tk, text='')
        self.score_view.pack(side=tkinter.TOP, fill=tkinter.X)
        self.score_view['font'] = 'Helvetica 30'

        tk.bind('<KeyPress>', self.keypress)

        self.paused = True
        self.fall_id = None
        self.redraw()
        self.resume()

        tk.mainloop()

    def fall(self):
        self.tetris.move('down')
        self.redraw()
        if self.tetris.game_over:
            self.pause()
        else:
            self.schedule_fall()

    def schedule_fall(self):
        # In case we're already called once.
        self.cancel_fall()
        self.fall_id = self.tk.after(500, self.fall)

    def cancel_fall(self):
        if self.fall_id is not None:
            self.tk.after_cancel(self.fall_id)
            self.fall_id = None

    def redraw(self):
        field = self.tetris.get_visible_field()

        for y, row in enumerate(field):
            for x, char in enumerate(row):
                self.display.set_block(x, y, char)

        self.score_view['text'] = str(self.tetris.lines)

        if self.tetris.game_over:
            self.pause()

    def pause(self):
        if not self.paused:
            self.paused = True
            self.display.pause()
            self.cancel_fall()

    def resume(self):
        if self.paused:
            self.paused = False
            self.display.resume()
            self.schedule_fall()

    def new_game(self):
        self.tetris = Tetris()
        self.display.resume()
        self.resume()

    def toggle_pause(self):
        if self.tetris.game_over:
            self.new_game()
        elif self.paused:
            self.resume()
        else:
            self.pause()

    def toggle_colors(self):
        self.display.colors = not self.display.colors

    def keypress(self, event):
        commands = {
            'Escape': self.toggle_pause,
            'space': self.toggle_pause,
            'c': self.toggle_colors,
        }

        if not self.paused:
            commands.update({
                'Up': lambda: self.tetris.move('rotleft'),
                'Left': lambda: self.tetris.move('left'),
                'Right': lambda: self.tetris.move('right'),
                'Down': lambda: self.tetris.move('down'),
            })

        if event.keysym in commands.keys():
            commands[event.keysym]()
            self.redraw()


if __name__ == '__main__':
    TetrisTk()
