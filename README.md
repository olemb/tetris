# Tetris for Python / Tkinter

![Tetris Screenshot](screenshot.png "Screenshot")

## Controls

* w / up = rotate left
* a / left = move left
* s / down = move down
* d / right = move right
* space / escape = pause / resume / new game

The scoring is a simple as it can be. You get one point for every line cleared.


## Motivation

Experimenting with ways of implementing Tetris in Python and having my own Tetris that's exactly the way I like it.


## Shapes

I found a very compact way to represent the piece shapes:

```python
shapes = {
    'O': ['56a9', '6a95', 'a956', '956a'],
    'I': ['4567', '26ae', 'ba98', 'd951'],
    'J': ['0456', '2159', 'a654', '8951'],
    'L': ['2654', 'a951', '8456', '0159'],
    'T': ['1456', '6159', '9654', '4951'],
    'Z': ['0156', '2659', 'a954', '8451'],
    'S': ['1254', 'a651', '8956', '0459'],
}
```

Each hex character corresponds to a square in this 4x4 box:

```
0 1 2 3
4 5 6 7
8 9 a b
c d e f
```

For example the first rotation of `T` is `'1456'` which draws this shape:

```
- 1 - -
4 5 6 -
- - - -
- - - -
```

while the second rotation `'6159'` draws this shape:

```
- 1 - -
- 5 6 -
- 9 - -
- - - -
```

You can get coordinates with something like:

```python
>>> [divmod(int(char, 16), 4) for char in '1456']
[(0, 1), (1, 0), (1, 1), (1, 2)]
```

Not that these are returned in `(y, x)` order due to the use of `divmod()`. You may want to flip them around to `(x, y)` before using them.


## Piece

The falling piece is represented as a dataclass:

```python
@dataclass(frozen=True)
class Piece:
    shape: str
    rot: int = 0
    x: int = 0
    y: int = 0
```





Ole Martin Bjorndalen
https://github.com/olemb/tetris/

