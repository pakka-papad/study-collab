#include <iostream>
#include <vector>
#include <string>
#include <ncurses.h>

int showMenu(const std::vector<std::string> &options) {
    initscr();
    initscr();
    noecho();  
    cbreak();  
    keypad(stdscr, TRUE); 

    int choice = 0;
    int max_choices = options.size();
    int res = -1;
    bool endLoop = 0;

    while (true) {
        clear(); 

        mvprintw(0, 0, "Hit Esc to go back");
        mvprintw(2, 0, "Select with UP and DOWN arrow key");
        for (int i = 0; i < max_choices; i++) {
            mvprintw(i + 3, 0, (choice == i ? "> " : "  "));
            printw("%d. %s", i + 1, &options[i][0]);
        }
        refresh();

        int key = getch(); 

        switch (key) {
            case KEY_UP:
                choice = (choice - 1 + max_choices) % max_choices;
                break;
            case KEY_DOWN:
                choice = (choice + 1) % max_choices;
                break;
            case 27: // escape key
                endLoop = 1;
                break;
            case 10: // Enter key
                res = choice;
                endLoop = 1;
                break;
            default:
                break;
        }
        
        if(endLoop) break;
    }

    endwin();
    return res;
}
