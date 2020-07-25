/*
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Entry point, engine initialisation, and main program loop.
 *
 * */

#include "util/log.h"
#include "util/timer.h"
#include "core.h"
//#include "mainmenu.h"
#include "mainstate.h"

int
main(int argc, char **argv)
{
    Window *window;
    int running = 1;
    float dt;

    initatraengine();
    window = getmainwindow();

    mainstateinit();
    mainstatestart();

    while (running) {
        timerglobalupdate();
        dt = timerglobaldiff();

        atrastatehandle();
        atrastateupdate(dt);
        atrastaterender();

        SDL_GL_SwapWindow(window->wndw);
    }
    mainstatestop();
    stopatraengine();

    return 0;
}
