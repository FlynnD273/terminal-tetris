#include "init.h"
// controls starting up ncurses and important constants

#include "matrix.h"
// controls everything happening in the matrix

#include "interface.h"
// controls everything happening out of the matrix

#include "keymap.h"

// #include "menu.h"
// controls everything happening in the pregame menu

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> //may not be in every machine?

#define microsNow tval_now.tv_sec * 1000000 + tval_now.tv_usec
// utility macro, microseconds since unspecified epoch

#define MAX_MOVE_RESETS 15
#define MAX_FLOORKICKS 99999


// TODO one floorkick rule, implement enhanced rotationnn algorithm

int main(int argc, char *argv[]) {
  init(); // sets up ncurses
  initMatrix();
  initHold();
  initPreview(5);
  initInterface();
  initTime();
  refresh();

  struct timeval tval_now; // stores the current wall clock time
  gettimeofday(&tval_now, NULL);

  long int microsStart = microsNow; // time at start of game

  int framerate = 60; // set this at will, but keep it reasonable! default 60
  int microsPerFrame = 1000000 / framerate;
	short is_paused = 0;
	long paused_time = 0;
	long last_paused_timestamp = 0;

  /* gameplay flags */
  int pieceCanDrop = 0;  // is there a currently falling piece
  int pieceOnMatrix = 0; // is there a piece at all
  int canHold = 1;   // once hold is pressed, must lock before pressing again
  int mustClear = 0; // prevents harddrop from constantly causing lineclear
                     // delay

  // the velocity table for levels 0-18
  static double velocityTable[] = {
      0.01,     0.016666, 0.024712, 0.036642, 0.054331, 0.080558, 0.119447,
      0.177110, 0.262608, 0.389380, 0.577350, 0.856061, 1.269319, 1.882072,
      2.790628, 4.137781, 6.135262, 9.097012, 13.48852};
  // stored as G values, i.e. how many gravity events happen per frame
  // for 19+ the velocity is always 20G
  // defined so that at 60fps (the default) level 1 has 1 second between gravity
  // and exponentially interpolated

  // delays decrease for levels 20-30, proportional to this table
  static int delayTable[] = {29, 25, 22, 19, 17, 15, 13, 11, 9, 8, 7, 6};
  // for -19 the basic delay is always 30 frames

  /* gameplay variables */
  int level = 1;          // level determines many timing parameters
	if (argc > 1) {
		level = atoi(argv[1]);
	}

  int linesPerLevel = 10; // clear 10(x-1) lines in total to reach level x
  int lines = 0;          // lines cleared in total
  int score = 0;          // score
  int backToBack = 0;     // whether the last lineclear was "difficult"
  int combo = 0;          // chained number of lineclears
  int movesLeft =
      MAX_MOVE_RESETS; // only 15 move lockdelay resets allowed before a step
  int floorkicksLeft = MAX_FLOORKICKS; // only 1 floorkick allowed

  // lines = 100; //debug
  // velocity = 16; //debug

  /* timing parameters, unless otherwise, in microseconds */
  int displayPeriod = microsPerFrame;
  long int displayDeadline = microsNow;
  // update display every frame
  int techniquePeriod = 60 * microsPerFrame;
  long int techniqueDeadline = microsNow;
  // write down the name of the most recent lineclear for up to 1 second
  int levelupPeriod = 20 * microsPerFrame;
  long int levelupDeadline = microsNow;
  // write down a levelup event for up to 1 second
  int gravityPeriod = (int)(microsPerFrame / velocityTable[level]);
  long int gravityDeadline = microsNow;
  // minos fall variable units per second
  int lockPeriod = 30 * microsPerFrame;
  long int lockDeadline = microsNow;
  int canLock = 0;
  // minos on ground lock 0.5 seconds after their last input
  // this enables the controversial technique "infinity"
  // at very low levels, the 1 gravityPeriod (1second) necessary to lock pieces
  // is greater anyway
  int clearPeriod = 30 * microsPerFrame;
  // lineclearing takes 0.5 seconds
  int entryPeriod = 6 * microsPerFrame;
  // piece entry takes 0.2 seconds
  int controlPeriod = microsPerFrame;
  long int controlDeadline = microsNow + controlPeriod;
  // poll for inputs every frame
	

  while (!topout()) {
    napms(1); // 1 millisecond sleep for lots of cpu freedom

    gettimeofday(&tval_now, NULL); // get time now

		if (is_paused && microsNow > controlDeadline) {
      static int ch;
      while ((ch = getch()) != ERR) { // while inputs remain
				if (ch == KEY_PAUSE) {
					is_paused = 0;
					paused_time += microsNow - last_paused_timestamp;
				}
			}
			continue;
		}

    if (microsNow > displayDeadline) { // time to display update
      drawMatrix();
      drawGhost();
      drawMino();
      drawHold(canHold);
      drawPreview();
      refreshMatrix();

      drawTime(microsNow - microsStart - paused_time);
      drawLines(lines);
      if (microsNow > levelupDeadline)
        drawLevel(level);
      if (microsNow > techniqueDeadline) // replace the score counter with the
                                         // technique for only1 second
        drawScore(score);
      refreshInterface();

      doupdate();

      displayDeadline += displayPeriod;
    }

    if (microsNow > controlDeadline) { // time to read inputs
      static int ch;
      while ((ch = getch()) != ERR) { // while inputs remain
        // TODO customizable keys

        switch (ch) {
				case KEY_PAUSE:
					is_paused = 1;
					last_paused_timestamp = microsNow;
					continue;
					break;
        case KEY_HOLD:
          if (canHold && hold())
            canHold = 0;
          break;
        case KEY_SHIFT_RIGHT:
          if (movesLeft && shift(1)) {
            lockDeadline =
                microsNow + lockPeriod; // delay lock timer on input success
          }
          if (canLock)
            movesLeft--;
          break;
        case KEY_SHIFT_LEFT:
          if (movesLeft && shift(-1)) {
            lockDeadline =
                microsNow + lockPeriod; // delay lock timer on input success
          }
          if (canLock)
            movesLeft--;
          break;
        case KEY_ROT_CW:
          if (movesLeft && rotate(1, floorkicksLeft)) {
            lockDeadline = microsNow + lockPeriod;
          }
          if (floorkick())
            floorkicksLeft--;
          if (canLock)
            movesLeft--;
          break;
        case KEY_ROT_CCW:
          if (movesLeft && rotate(-1, floorkicksLeft)) {
            lockDeadline = microsNow + lockPeriod;
          }
          if (floorkick())
            floorkicksLeft--;
          if (canLock)
            movesLeft--;
          break;
        case KEY_SOFT_DROP:
          if (gravity(0)) { // won't lock if the mino bottoms out
            gravityDeadline = microsNow + gravityPeriod; // reset grav clock
            score += 1; // 1 point per row dropped
          }
          break;
        case KEY_HARD_DROP:                              // space = harddrop
          while ((pieceOnMatrix = gravity(1))) // immediately bottoms/locks
            score += 2;                        // 2 points per row dropped

          if (!mustClear) { // any lines to clear?
            gravityDeadline = microsNow + entryPeriod;
            if (lineclear(0))
              gravityDeadline += clearPeriod; // takes time to clear lines
            mustClear =
                1; // prevents constant harddrops delaying gravity indefinitely
          }
          canHold = 1;
          break;
        }

        if (!movesLeft) {             // no move resets left
          pieceOnMatrix = gravity(1); // immediately locks
        }
      }
    }

    if (microsNow > gravityDeadline) { // time to gravity

      if (mustClear) { /* scoring is processed here*/
        int clearCount =
            lineclear(1); // clear any lines waiting to be cleared, add them
        static int scoreTable[] = {0, 100, 300, 500,
                                   800}; // for simple lineclears
        static int tspinScoreTable[] = {400, 800, 1200, 1600}; // for tspins

        int difficult = 0;
        if (clearCount == 4)
          difficult = 1;
        if (clearCount && tspin(0))
          difficult = 1;

        int points =
            tspin(0) ? tspinScoreTable[clearCount] : scoreTable[clearCount];

        if (difficult) { // difficult clear, apply back to back bonus
          if (backToBack++)
            points = points * 3 / 2;
        } else if (clearCount)
          backToBack = 0;

        if (clearCount) // combo lineclears, reset by locking and not clearing a
                        // line
          score += 50 * combo++;
        else
          combo = 0;

        if (perfectClear()) // perfect clear awards a constant 5x multiple of
                            // points
          score *= 5; // you earned it

        if (tspin(0) || clearCount) {
          techniqueDeadline = microsNow + techniquePeriod; // set the timer
          drawTechnique(backToBack, tspin(0), clearCount, perfectClear(),
                        combo);
          refreshInterface();
        }

        score += points * level;
        lines += clearCount;

        int newLevel = lines / linesPerLevel + 1;
        if (newLevel > 30)
          newLevel = 30;        // 30 is the current level cap
        if (newLevel > level) { // promote levels if necessary
          levelupDeadline = microsNow + levelupPeriod; // set the timer
          drawLevelup(level, newLevel);
          refreshInterface();

          level = newLevel;
          gravityPeriod = (int)(microsPerFrame /
                                ((level > 18) ? 20 : velocityTable[level]));
          lockPeriod = clearPeriod =
              ((level < 20) ? 30 : delayTable[level - 20]) * microsPerFrame;
        }

        mustClear = 0;               // free up harddrop to delay gravity again
        movesLeft = MAX_MOVE_RESETS; // give 15 move resets back
      }

      if (pieceOnMatrix) { // is there a piece?
        pieceCanDrop = gravity(0);
        if (pieceCanDrop) { // yes, is piece in air?
          canLock = 0;
          movesLeft = MAX_MOVE_RESETS; // give 15 move resets back
          gravityDeadline = microsNow + gravityPeriod;
        } else if (canLock) {        // no, has canLock been enabled?
          pieceCanDrop = gravity(0); // at any time, piece may have moved
          if (microsNow > lockDeadline) {
            pieceCanDrop =
                gravity(1); // attempt to lock; may have moved so it could drop
            canLock = 0;
            if (!pieceCanDrop) { // locked successfully?
              pieceOnMatrix = 0;
              canHold = 1;
              floorkicksLeft = MAX_FLOORKICKS; // give 1 floorkick back
              movesLeft = MAX_MOVE_RESETS;     // give 15 move resets back
              mustClear = 1;                   // flag to clear some lines
              if (lineclear(0))                // any lines to clear?
                gravityDeadline =
                    microsNow + clearPeriod; // takes time to clear lines
            }
          }
        } else { // no, enable canLock
          canLock = 1;
          lockDeadline = microsNow + lockPeriod; // set lock timer
        }
      } else { // no, make a new one
        newMino(BLANK);
        pieceOnMatrix = 1;
        pieceCanDrop = 1;
        gravityDeadline = microsNow + entryPeriod; // entry delay
      }
    }

    // mvprintw(0,0,"pieceOnMatrix: %d", pieceOnMatrix);
    // mvprintw(1,0,"pieceCanDrop: %d", pieceCanDrop);
    // mvprintw(2,0,"canLock: %d",canLock);
  }

  endwin();
  printf("Your score: %d\n", score);
  printf("Ending level: %d\n", level);
  printf("Lines cleared: %d\n", lines);
  printf("Time: %0.02fs\n", (microsNow - microsStart - paused_time) / 1000000.);
}
