# Tetris for Python / Tkinter

![Tetris Screenshot](images/screenshot.png "Screenshot")

## Controls

* `w` / `up` = rotate left
* `a` / `left` = move left
* `s` / `down` = move down
* `d` / `right` = move right
* `c` = toggle color mode / monochromo mode
* `space` / `escape` = pause / resume / new game

The scoring is a simple as it can be. You get one point for every line cleared.


## Monochrome mode

![Monochrome Mode Screenshot](images/monochrome.png "Monochrome")

Monochrome mode (toggled with the `c` key) shows the falling piece as black
and frozen blocks as gray. This makes it easier to see the total shape
of the block landscape.


## Implementation

I've come up with some interesting ideas and techniques that I'd like to share below.


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

Note that these are returned in `(y, x)` order due to the use of `divmod()`. You may want to flip them around to `(x, y)` before using them.


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

```python
>>> from tetris import *
>>> piece = Piece('T')
>>> piece
Piece(shape='T', rot=0, x=0, y=0)
```

To move or rotate the piece I use `dataclasses.replace()`:

```python
>>> replace(piece, x=piece.x + 1)
Piece(shape='T', rot=0, x=1, y=0)

>>> replace(piece, rot=(piece.rot + 1) % 4)
Piece(shape='T', rot=1, x=0, y=0)
```

This returns a copy of the piece that I can test against the board to see if it fits before I actually move it.


## Random Shape Bag

If you choose a random shape every time there will sometimes be very long stretches between each `I` piece. A common solution is to put all 7 shapes in a bag and draw random shapes out of the bag until it's empty, and then repeat.

I've implemented this as a generator:

```python
>>> random_shapes = random_shape_bag()
>>> next(random_shapes)
'L'
>>> next(random_shapes)
'S'
>>> next(random_shapes)
'Z'
```


## Field

The field is a list of lists of shapes, with empty squares as `''`. Here's a smaller 8x8 version:

```python
field = [
    ['J', '',  'T', 'T', 'T', 'L', 'L', 'L']
    ['O', 'O', '',  'T', '',  'L', 'O', 'O']
    ['O', 'O', 'I', 'I', 'I', 'I', 'O', 'O']  # Full row
    ['',  '',  '',  'O', 'O', '',  '',  '' ]
    ['',  '',  '',  'O', 'O', '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
]
```

A few things to note here:

* The field looks upside down here. Row 0 is normally drawn at the bottom of the screen.

* We can still see which shape has made up each block. Since each shape has a distinct color this makes it easy to support colors.

Since the field is a list we can simple filter out full rows:

```python
field = [row for row in field if not all(row)]
```

Rows above the full rows will automatically collapse:

```python
[
    ['J', '',  'T', 'T', 'T', 'L', 'L', 'L']
    ['O', 'O', '',  'T', '',  'L', 'O', 'O']
    ['',  '',  '',  'O', 'O', '',  '',  '' ]
    ['',  '',  '',  'O', 'O', '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
    ['',  '',  '',  '',  '',  '',  '',  '' ]
]
```

We then need to pad the field with as many new rows as we removed:

```python
while len(field) < height:
    field.append([''] * width)
```

## BlockDisplay

The `BlockDisplay` is a subclass of the Tkinter canvas. It's works like a very chunky pixel display where you can set the color of each pixel individually:

```python
display[x, y] = 'T'
```

The display will look up the color of the `T` piece and use it to set the fill of the rectangle at `(x, y)`. You can clear a block with:

```python
display[x, y] = ''
```

This makes it straight forward to draw the field:

```python
for x in range(width):
    for y in range(height):
        display[x, y] = field[y][x]
```

and the piece:

```python
char = piece.shape.lower()
for x, y in get_piece_blocks(piece):
    display[x, y] = char
```

(The active piece is drawn in lowercase to a allow the rendered to use a different style for it.)

## Future Plans

I will tinker with the code from time to time but I have no plans for additional features.


## Contact


Ole Martin Bjørndalen
https://github.com/olemb/tetris/
