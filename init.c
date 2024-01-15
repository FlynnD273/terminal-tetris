#include "init.h"
#include "matrix.h"

#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>

void init(void){ //sets up ncurses
  initscr();

  if (!has_colors()){//if colors cannot be used, then don't!
    endwin();
    printf("Terminal does not support color.\n");
    exit(1);
  }
  start_color();
    
  /* int rows, cols; */
  /* getmaxyx(stdscr, rows, cols); */
  /* if (rows != 24 || cols != 80){ */
  /*   endwin(); */
  /*   printf("Terminal is not the optimal size (80 x 24). Continue? (y/n)"); */
  /*   if (getchar() != 'y') */ 
  /*     exit(2); */
  /*   refresh(); */
  /* } */

  if (can_change_color()){
    //if colors can be changed, then make them nice
    //e.g. some color schemes are pretty wacky

    init_color(COLOR_WHITE, 1000, 1000, 1000);
    init_color(COLOR_ORANGE, 827, 624, 420); //new color!
    init_color(COLOR_CYAN, 51, 804, 804);
    init_color(COLOR_YELLOW, 808, 800, 420);
    init_color(COLOR_MAGENTA, 812, 573, 820);
    init_color(COLOR_GREEN, 219, 788, 439);
    init_color(COLOR_RED, 800, 573, 573);
    init_color(COLOR_BLUE, 51, 451, 800);
  }
  else{
    endwin();
    printf("Terminal does not support color changing.\nThe game may use an odd color scheme. Continue? (y/n)");
    if (getchar() != 'y') 
      exit(3);
    refresh();
  }

  cbreak(); //interrupts are still passed directly to terminal
  //raw(); //even interrupts will pass through program first
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
  
  /* The required 7 colors of tetrominoes are:
     I: cyan O: yellow T: purple S: green Z: red J: blue L: orange   
     of these, cyan, yellow, green, red, and blue are built-in to xterm-256
     magenta as purple is close enough, 
     orange must be created if possible
  */
  
  init_pair(I, COLOR_BLACK, COLOR_CYAN);   //colored background
  init_pair(O, COLOR_BLACK, COLOR_YELLOW); //used for drawing minos
  init_pair(T, COLOR_BLACK, COLOR_MAGENTA);//a character subtracts color
  init_pair(S, COLOR_BLACK, COLOR_GREEN);
  init_pair(Z, COLOR_BLACK, COLOR_RED);
  init_pair(J, COLOR_BLACK, COLOR_BLUE);
  init_pair(L, COLOR_BLACK, COLOR_ORANGE);
  
  init_pair(8, COLOR_BLACK, COLOR_BLACK);
  init_pair(9, COLOR_BLACK, COLOR_WHITE);

  init_pair(11, COLOR_CYAN, COLOR_BLACK);    //black background
  init_pair(12, COLOR_YELLOW, COLOR_BLACK);  //used for drawing ghosts and UI
  init_pair(13, COLOR_MAGENTA, COLOR_BLACK); //a character adds color
  init_pair(14, COLOR_GREEN, COLOR_BLACK);
  init_pair(15, COLOR_RED, COLOR_BLACK);
  init_pair(16, COLOR_BLUE, COLOR_BLACK);
  init_pair(17, COLOR_ORANGE, COLOR_BLACK);

  init_pair(18, COLOR_BLACK, COLOR_BLACK);
}

