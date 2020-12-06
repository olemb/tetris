/*
  Amiga Tetris
  Ole Martin Bjorndalen
  1996

  + Use font height for brick height?
*/

#include <proto/intuition.h>
#include <proto/timer.h>
#include <exec/memory.h>

#include <stdio.h>
#include <math.h>

typedef signed char bool;

#define BOARD_WIDTH    10 /* 10 */
#define BOARD_HEIGHT  16 /* 16 */
#define SQUARE_WIDTH  14
#define SQUARE_HEIGHT  12
#define SPACING      2
#define DISPLAY_WIDTH  ((BOARD_WIDTH + 3) * SQUARE_WIDTH + (SPACING*4+2))
#define DISPLAY_HEIGHT  (BOARD_HEIGHT * SQUARE_HEIGHT + (SPACING*2))

#define SQUARE_EMPTY  0
#define SQUARE_USED    1
#define SQUARE_ACTIVE  2
#define SQUARE_DISABLED  4
#define SQUARE_FALLING  SQUARE_USED | SQUARE_ACTIVE
#define SQUARE_PAUSED  SQUARE_FALLING | SQUARE_DISABLED

#define SCREEN_X(x)    (SPACING + (x) * SQUARE_WIDTH)
#define SCREEN_Y(y)    (SPACING + (y) * SQUARE_HEIGHT)

#define MOVE_PLACE  0
#define MOVE_ROTATE  1
#define MOVE_DOWN  2
#define MOVE_LEFT  3
#define MOVE_RIGHT  4

#define BRICK_ROTATIONS  4
#define BRICK_SQUARES  4

struct coord {
  signed char x;
  signed char y;
};

struct brick {
  struct coord squares[BRICK_ROTATIONS][BRICK_SQUARES];
};

struct brick bricks[7] = {
  /*
    ##
     #
     #
  */
  {
    -1, -1,
    0, -1,
    0, 0,
    0, 1,

    -1, 0,
    0, 0,
    1, 0,
    1, -1,

    0, -1,
    0, 0,
    0, 1,
    1, 1,

    -1, 1,
    -1, 0,
    0, 0,
    1, 0,
  },
  /*
     ##
     #
     #
  */
  {
    1, -1,
    0, -1,
    0, 0,
    0, 1,

    -1, 0,
    0, 0,
    1, 0,
    1, 1,

    0, -1,
    0, 0,
    0, 1,
    -1, 1,

    -1, -1,
    -1, 0,
    0, 0,
    1, 0,
  },
  /*
    ##
    ##
  */
  {
    -1, -1,
    0, -1,
    -1, 0,
    0, 0,

    -1, -1,
    0, -1,
    -1, 0,
    0, 0,

    -1, -1,
    0, -1,
    -1, 0,
    0, 0,

    -1, -1,
    0, -1,
    -1, 0,
    0, 0,
  },
  /*
    ##
     ##
  */
  {
    -1, -1,
    0, -1,
    0, 0,
    1, 0,

    1, -1,
    1, 0,
    0, 0,
    0, 1,

    -1, 0,
    0, 0,
    0, 1,
    1, 1,

    -1, 1,
    -1, 0,
    0, 0,
    0, -1,
  },
  /*
     ##
    ##
  */
  {
    -1, 0,
    0, 0,
    0, -1,
    1, -1,

    1, 1,
    1, 0,
    0, 0,
    0, -1,

    1, 0,
    0, 0,
    0, 1,
    -1, 1,

    -1, -1,
    -1, 0,
    0, 0,
    0, 1,
  },
  /*
     #
    ###
  */
  {
    0, -1,
    -1, 0,
    0, 0,
    1, 0,

    0, -1,
    0, 0,
    0, 1,
    1, 0,

    -1, 0,
    0, 0,
    1, 0,
    0, 1,

    -1, 0,
    0, -1,
    0, 0,
    0, 1,
  },

  /*
     #
     #
     #
     #
  */
  {
    0, -1,
    0, 0,
    0, 1,
    0, 2,

    -2, 0,
    -1, 0,
    0, 0,
    1, 0,

    0, -2,
    0, -1,
    0, 0,
    0, 1,

    -1, 0,
    0, 0,
    1, 0,
    2, 0,
  },
};

struct tetris {
  struct coord pos;
  unsigned char rot;
  struct brick *brick;
  struct brick *nextBrick;
  int lines;
  unsigned char board[BOARD_WIDTH][BOARD_HEIGHT];
};

/* Core //////////////////////////////////////////////////////////*/

struct RastPort *RastPort;
struct DrawInfo *DrawInfo;

void drawSquare(struct RastPort *rp, struct DrawInfo *drawInfo, int x, int y, int state);
void drawSquareScreenCoord(struct RastPort *rp, struct DrawInfo *drawInfo, int x, int y, int state);

void redrawBoard(struct tetris *this,
        struct tetris *buffer,
        struct RastPort *rp,
        struct DrawInfo *drawInfo,
        bool drawAll)
{
  int x, y;

  for(y = 0; y < BOARD_HEIGHT; y++)
    for(x = 0; x < BOARD_WIDTH; x++)
      if(drawAll || this->board[x][y] != buffer->board[x][y])
        drawSquare(rp, drawInfo, x, y, this->board[x][y]);

  /* Update buffer */
  *buffer = *this;
}

void newBrick(struct tetris *this)
{
  int i;

  /* Erase the old brick */
  SetAPen(RastPort, DrawInfo->dri_Pens[BACKGROUNDPEN]);
  RectFill(RastPort,
    SCREEN_X(BOARD_WIDTH)+SPACING*3,
    SCREEN_Y(BOARD_HEIGHT - 5),
    SCREEN_X(BOARD_WIDTH+3)+SPACING*3-1,
    SCREEN_Y(BOARD_HEIGHT) - 1);

  this->pos.x = BOARD_WIDTH/2;
  this->pos.y = 1;
  this->brick = this->nextBrick;
  this->nextBrick = &bricks[lrand48() % 7];
  this->rot = 0;

  for(i = 0; i < 4; i ++)
  {
    drawSquareScreenCoord(RastPort, DrawInfo,
        SCREEN_X(BOARD_WIDTH+1 + this->nextBrick->squares[0][i].x) + SPACING*3,
        SCREEN_Y(BOARD_HEIGHT - 4 + this->nextBrick->squares[0][i].y),
        SQUARE_USED);
  }
}

void drawBrick(struct tetris *this, int state)
{
  int i;

  for(i = 0; i < 4; i++)
    this->board  [this->pos.x + this->brick->squares[this->rot][i].x]
          [this->pos.y + this->brick->squares[this->rot][i].y]
          = state;
}

void initTetris(struct tetris *this)
{
  int x, y;

  for(y = 0; y < BOARD_HEIGHT; y++)
    for(x = 0; x < BOARD_WIDTH; x++)
      this->board[x][y] = 0;

  this->lines = 0;
  this->nextBrick = &bricks[lrand48() % 7];
}

bool collapseBoard(struct tetris *this)
{
  int x, y;
  bool result = FALSE;

  for(y = 0; y < BOARD_HEIGHT; y++)
  {
    int spaces = BOARD_WIDTH;

    for(x = 0; x < BOARD_WIDTH; x++)
    {
      if(this->board[x][y])
        spaces--;
    }

    if(spaces == 0)
    {
      int cy;

      /* Collapse! */
      for(x = 0; x < BOARD_WIDTH; x++)
      {
        for(cy = y; cy != 0; cy--)
          this->board[x][cy] = this->board[x][cy-1];

        this->board[x][0] = 0;
      }

      this->lines++;
      result = TRUE;
    }
  }

  return result;
}

/*
  Returns the value of a square (used of empty)
*/
bool squareUsed(struct tetris *this, int x, int y)
{
  if(this->board[x][y] != 0 ||
    x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

bool moveBrick(struct tetris *this, int direction)
{
  struct coord temp = this->pos;
  int rot = this->rot;
  bool result = TRUE;
  int i;

  /* Only erase if there is a brick */
  if(direction != MOVE_PLACE)
    drawBrick(this, SQUARE_EMPTY);

  switch(direction)
  {
    case MOVE_ROTATE:
      /* 0...1...2...3...0... */
      rot = ++rot & 3;
      break;
    case MOVE_DOWN:
      temp.y++;
      break;
    case MOVE_LEFT:
      temp.x--;
      break;
    case MOVE_RIGHT:
      temp.x++;
      break;
  }

  for(i = 0; i < 4; i++)
  {
    if(squareUsed(this,
      temp.x + this->brick->squares[rot][i].x,
      temp.y + this->brick->squares[rot][i].y))
    {

      result = FALSE;
      break;
    }
  }

  if(result)
  {
    this->pos = temp;
    this->rot = rot;
  }

  /* Don't draw brick if game over */
  if(direction == MOVE_PLACE && result == FALSE)
    ; // Do nothing
  else
    drawBrick(this, SQUARE_FALLING);

  return result;
}

/* Draw //////////////////////////////////////////////////////////*/

void drawSquareScreenCoord(struct RastPort *rp, struct DrawInfo *drawInfo, int x, int y, int state)
{
  if(state)
  {
    SetAPen(rp, drawInfo->dri_Pens[state & SQUARE_ACTIVE ? FILLPEN : BACKGROUNDPEN]);
    RectFill(rp,
      x + 1,
      y + 1,
      x + SQUARE_WIDTH - 2,
      y + SQUARE_HEIGHT - 2);

    if(state & SQUARE_DISABLED)
    {
      int cx, cy;

      SetAPen(rp, drawInfo->dri_Pens[SHINEPEN]);
      for(cy = y+1;
        cy < y + SQUARE_HEIGHT-1;
        cy++)
      {
        for(cx = x + (1 + cy & 3);
          cx < x + SQUARE_WIDTH-1;
          cx+=4)
        {
          WritePixel(rp, cx, cy);
        }
      }
    }

    SetAPen(rp, drawInfo->dri_Pens[SHINEPEN]);
    Move(rp, x,          y+SQUARE_HEIGHT-1);
    Draw(rp, x,          y);
    Draw(rp, x+SQUARE_WIDTH-1,  y);

    SetAPen(rp,          drawInfo->dri_Pens[SHADOWPEN]);
    Draw(rp, x+SQUARE_WIDTH-1,  y+SQUARE_HEIGHT-1);
    Draw(rp, x+1,        y+SQUARE_HEIGHT-1);
  }
  else
  {
    SetAPen(rp, drawInfo->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp,
      x,
      y,
      x + SQUARE_WIDTH - 1,
      y + SQUARE_HEIGHT - 1);
  }
}

void drawSquare(struct RastPort *rp, struct DrawInfo *drawInfo, int x, int y, int state)
{
  drawSquareScreenCoord(rp, drawInfo, SCREEN_X(x), SCREEN_Y(y), state);
}

/*
void drawSquare(struct RastPort *rp, struct DrawInfo *drawInfo, int x, int y, int state)
{
  if(state)
  {
    SetAPen(rp, drawInfo->dri_Pens[state & SQUARE_FALLING ? FILLPEN : BACKGROUNDPEN]);
    RectFill(rp,
      SCREEN_X(x) + 1,
      SCREEN_Y(y) + 1,
      SCREEN_X(x + 1) - 2,
      SCREEN_Y(y + 1) - 2);

    SetAPen(rp, drawInfo->dri_Pens[SHINEPEN]);
    Move(rp, SCREEN_X(x),    SCREEN_Y(y+1)-1);
    Draw(rp, SCREEN_X(x),    SCREEN_Y(y));
    Draw(rp, SCREEN_X(x+1)-1,  SCREEN_Y(y));
    SetAPen(rp, drawInfo->dri_Pens[SHADOWPEN]);
    Draw(rp, SCREEN_X(x+1)-1,  SCREEN_Y(y+1)-1);
    Draw(rp, SCREEN_X(x)+1,    SCREEN_Y(y+1)-1);
  }
  else
  {
    SetAPen(rp, drawInfo->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp,
      SCREEN_X(x),
      SCREEN_Y(y),
      SCREEN_X(x + 1) - 1,
      SCREEN_Y(y + 1) - 1);
  }
}
*/

#define KEY_ROTATE  76
#define KEY_DOWN  77
#define KEY_LEFT  79
#define KEY_RIGHT  78
#define KEY_PAUSE  192
#define KEY_QUIT  69

void startTimer(struct timerequest *this)
{
  this->tr_node.io_Command = TR_ADDREQUEST;
  this->tr_time.tv_secs = 1;
  this->tr_time.tv_micro = 0;

  SendIO((struct IORequest *)this);
}

void stopTimer(struct timerequest *this)
{
  AbortIO((struct IORequest *)this);
}

void handleTimer(struct timerequest *this)
{
  GetMsg(this->tr_node.io_Message.mn_ReplyPort);
  this->tr_time.tv_secs = 1;
  this->tr_time.tv_micro = 0;
  SendIO((struct IORequest *)this);
}

void deleteTimer(struct timerequest *this)
{
  stopTimer(this);
  CloseDevice((struct IORequest *)this);
  DeleteMsgPort(this->tr_node.io_Message.mn_ReplyPort);
  DeleteExtIO((struct IORequest *)this);
}

struct timerequest *createTimer(void)
{
  struct MsgPort *port;
  struct timerequest *this;
  ULONG unit = UNIT_MICROHZ;

  if(port = CreateMsgPort())
  {
    if(this = (struct timerequest *)
      CreateExtIO(port, sizeof(struct timerequest)))
    {
      if(!OpenDevice(TIMERNAME, unit, (struct IORequest *)this, 0L))
      {
        return this;
      }

      DeleteExtIO((struct IORequest *)this);
    }

    DeleteMsgPort(port);
  }

  return NULL;;
}

void drawFrames(struct RastPort *rp, struct DrawInfo *drawInfo)
{
  SetAPen(rp, drawInfo->dri_Pens[SHADOWPEN]);
  Move(rp, SCREEN_X(BOARD_WIDTH) + SPACING, SCREEN_Y(0));
  Draw(rp, SCREEN_X(BOARD_WIDTH) + SPACING, SCREEN_Y(BOARD_HEIGHT)-1);
  SetAPen(rp, drawInfo->dri_Pens[SHINEPEN]);
  Move(rp, SCREEN_X(BOARD_WIDTH) + SPACING + 1, SCREEN_Y(0));
  Draw(rp, SCREEN_X(BOARD_WIDTH) + SPACING + 1, SCREEN_Y(BOARD_HEIGHT)-1);
}

enum {
  STATE_PAUSED,
  STATE_GAME,
  STATE_GAMEOVER
};

UBYTE Title[80] = { '\0' };

ULONG moveBrickDown(struct tetris *this, struct Window *window)
{
  ULONG state = STATE_GAME;

  if(!moveBrick(this, MOVE_DOWN))
  {
    drawBrick(this, SQUARE_USED);
    collapseBoard(this);

    newBrick(this);
    if(moveBrick(this, MOVE_PLACE))
      drawBrick(this, SQUARE_FALLING);
    else
    {
      sprintf(Title, "GAME OVER!  %d lines", this->lines);
      SetWindowTitles(window, (UBYTE *)Title, (UBYTE *)-1);
      state = STATE_GAMEOVER;
    }
  }

  return state;
}

main()
{
  struct Window *window;
  struct timerequest *timer;
  struct Screen *screen;
  struct DrawInfo *drawInfo;
  ULONG secs, micros;

  CurrentTime(&secs, &micros);
  srand48(secs);

  if(timer = createTimer())
  {
    if(screen = LockPubScreen(NULL))
    {
      if(drawInfo = GetScreenDrawInfo(screen))
      {
        if(window = OpenWindowTags(NULL,
          WA_InnerWidth, DISPLAY_WIDTH,
          WA_InnerHeight, DISPLAY_HEIGHT,
          WA_Left, (screen->Width - DISPLAY_WIDTH)/2,
          WA_Top, (screen->Height - DISPLAY_HEIGHT)/2,
          WA_GimmeZeroZero, TRUE,
          WA_Title, "Press Space",
          WA_ScreenTitle, "Tetris by Ole Martin Bjørndalen - Bodø BBS Juleutgave - GOD JUL!",
          WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
          WA_CloseGadget, TRUE,
          WA_DragBar, TRUE,
          WA_DepthGadget, TRUE,
          WA_Activate, TRUE,
          WA_PubScreen, screen,
          TAG_DONE))
        {
          struct IntuiMessage *msg;
          struct tetris tetris;
          struct tetris buffer;
          BOOL done = FALSE;
          ULONG windowWait = 1L << window->UserPort->mp_SigBit;
          ULONG timerWait = 1L << timer->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
          ULONG sig;
          ULONG state = STATE_GAMEOVER;

          RastPort = window->RPort;
          DrawInfo = drawInfo;

          initTetris(&tetris);
          initTetris(&buffer);

          startTimer(timer);

          drawFrames(window->RPort, drawInfo);
          redrawBoard(&tetris, &buffer, window->RPort, drawInfo, TRUE);

          while(!done)
          {
            sig = Wait(windowWait | timerWait);

            if(sig & timerWait)
            {
              if(state == STATE_GAME)
                state = moveBrickDown(&tetris, window);

              handleTimer(timer);
            }

            if(sig & windowWait)
            {
              while(msg = (struct IntuiMessage *)GetMsg(window->UserPort))
              {
                switch(msg->Class)
                {
                  case IDCMP_RAWKEY:
                    switch(msg->Code)
                    {
                      case KEY_ROTATE:
                        if(state == STATE_GAME)
                          moveBrick(&tetris, MOVE_ROTATE);
                        break;
                      case KEY_DOWN:
                        if(state == STATE_GAME)
                          state = moveBrickDown(&tetris, window);
                        break;
                      case KEY_LEFT:
                        if(state == STATE_GAME)
                          moveBrick(&tetris, MOVE_LEFT);
                        break;
                      case KEY_RIGHT:
                        if(state == STATE_GAME)
                          moveBrick(&tetris, MOVE_RIGHT);
                        break;
                      case KEY_PAUSE:
                        switch(state)
                        {
                          case STATE_GAME:
                            sprintf(Title, "PAUSED  %d lines", tetris.lines);
                            SetWindowTitles(window, (UBYTE *)Title, (UBYTE *)-1);
                            drawBrick(&tetris, SQUARE_PAUSED);
                            state = STATE_PAUSED;
                            break;
                          case STATE_GAMEOVER:
                            initTetris(&tetris);
                            newBrick(&tetris);
                            drawBrick(&tetris, SQUARE_FALLING);
                          case STATE_PAUSED:
                            sprintf(Title, "Tetris  %d lines", tetris.lines);
                            SetWindowTitles(window, Title, (UBYTE *)-1);
                            drawBrick(&tetris, SQUARE_FALLING);
                            state = STATE_GAME;
                            break;
                        }
                        break;
                      case KEY_QUIT:
                        done = TRUE;
                        break;
                    }
                    break;
                  case IDCMP_CLOSEWINDOW:
                    done = TRUE;
                    break;
                }

                ReplyMsg((struct Message *)msg);
              }
            }

            if(tetris.lines > buffer.lines)
            {
              sprintf(Title, "Tetris  %d lines", tetris.lines);
              SetWindowTitles(window, Title, (UBYTE *)-1);
            }

            redrawBoard(&tetris, &buffer, window->RPort, drawInfo, FALSE);
          }

          CloseWindow(window);
        }

        FreeScreenDrawInfo(screen, drawInfo);
      }

      UnlockPubScreen(NULL, screen);
    }

    deleteTimer(timer);
  }
}
