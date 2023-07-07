#include "window.h"
#include <ncurses.h>
#include "audio_effects.h"


/* Global control flags */
extern _Atomic int play_flag;
extern _Atomic int window_flag;
extern _Atomic int plotting;
extern _Atomic int writing_buffer_plot;
extern _Atomic int buffer_ready_to_plot;

/* Ncurses parameters */
const int screen_width = 100;
const int screen_height = 30;
const int screen_pedals_width = 80;


struct Window {
    int width, height;
    struct {
        int y, x;
    } pos;
    WINDOW *window_ptr;
};


enum Move_effect {
    MOVE_UP,
    MOVE_DOWN
};

enum Effects_type effects_order[NUMBER_OF_EFFECTS] = {DISTORTION, TREMOLO, FLANGER, BITCRUSHER};

int effect_index = -1;
int effect_param_index = 0;
int effect_arg_index = -1;

extern struct Effects_config tremolo_conf;
extern struct Effects_config distortion_conf;
extern struct Effects_config bitcrusher_conf ;
extern struct Effects_config flanger_conf ;



extern FirstOrderLP_t lp;
extern FirstOrderHP_t hp;
extern BandPass_t bp;
extern BandStop_t bs;
extern SecondOrderLP_t lp2;
extern Flanger_t flgr;
extern Tremolo_t trml;
extern Distortion_t distort;
extern Crossover_t cross;
extern Crossover2_t cross2;
extern Bitcrusher_t bcrush;


extern float buffer_plot_input[N_SAMPLES * PLOT_DECIMATION];
extern float buffer_plot_output[N_SAMPLES * PLOT_DECIMATION];


enum Window_mode {
    MODE_NORMAL,
    MODE_SELECT_PEDAL,
    MODE_QUIT,
    MODE_EDIT_ARGS
};

static void define_effects_positions(void) {
    int line = 1;
    for (int i = 0; i < NUMBER_OF_EFFECTS; i++) {
        switch (effects_order[i]) {
            case TREMOLO:
                tremolo_conf.pos_y = line;
                line += tremolo_conf.height;
                break;
            case DISTORTION:
                distortion_conf.pos_y = line;
                line += distortion_conf.height;
                break;
            case FLANGER:
                flanger_conf.pos_y = line;
                line += flanger_conf.height;
                break;
            case BITCRUSHER:
                bitcrusher_conf.pos_y = line;
                line += bitcrusher_conf.height;
                break;
            default:
                continue;
        }
    }
}


static void redraw_pedals(struct Window *window, enum Window_mode win_mode, int key_pressed) {
    int status_color, param_color;
    char string_number[100] = "";
    define_effects_positions();
    for (int i = 0; i < NUMBER_OF_EFFECTS; i++) {
        switch (effects_order[i]) {
            case TREMOLO:
                status_color = (i == effect_index && effect_arg_index == -1) ? 4 : (tremolo_conf.status ? 1 : 2);
                if (tremolo_conf.status) {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, tremolo_conf.pos_y, 2, "[ X ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                else {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, tremolo_conf.pos_y, 2, "[   ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                wprintw(window->window_ptr, "   Tremolo   ");

                if (i == effect_index && effect_arg_index >= tremolo_conf.n_args) effect_arg_index = tremolo_conf.n_args - 1;

                param_color = (i == effect_index && effect_arg_index == 0) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 0)
                    tremolo_conf.values[0] += .005;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 0)
                    tremolo_conf.values[0] -= 0.005;
                if (tremolo_conf.values[0] > 1.f) tremolo_conf.values[0] = 1.f;
                else if (tremolo_conf.values[0] < 0.f) tremolo_conf.values[0] = 0.f;
                sprintf(string_number, "[ %.3f ]", tremolo_conf.values[0]);
                wprintw(window->window_ptr, " | Gain ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                param_color = (i == effect_index && effect_arg_index == 1) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 1)
                    tremolo_conf.values[1] += .25;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 1)
                    tremolo_conf.values[1] -= 0.25;
                if (tremolo_conf.values[1] < 0.f) tremolo_conf.values[1] = 0.f;
                sprintf(string_number, "[ %.2f ]", tremolo_conf.values[1]);
                wprintw(window->window_ptr, " Freq ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                wprintw(window->window_ptr, "                          ");
                trml.depth = tremolo_conf.values[0];
                trml.f = tremolo_conf.values[1];

                break;
            case DISTORTION:
                status_color = (i == effect_index && effect_arg_index == -1) ? 4 : (distortion_conf.status ? 1 : 2);
                if (distortion_conf.status) {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, distortion_conf.pos_y, 2, "[ X ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                else {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, distortion_conf.pos_y, 2, "[   ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }

                if (i == effect_index && effect_arg_index >= distortion_conf.n_args) effect_arg_index = distortion_conf.n_args - 1;

                param_color = (i == effect_index && effect_arg_index == 0) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 0) 
                    distortion_conf.values[0] += .005;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 0)
                    distortion_conf.values[0] -= 0.005;
                if (distortion_conf.values[0] > 1.f) distortion_conf.values[0] = 1.f;
                else if (distortion_conf.values[0] < 0.f) distortion_conf.values[0] = 0.f;
                sprintf(string_number, "[ %.3f ]", distortion_conf.values[0]);
                wprintw(window->window_ptr, "   Distortion");
                wprintw(window->window_ptr, " | max ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "                             ");
                distort.max = distortion_conf.values[0];
                break;
            case FLANGER:
                status_color = (i == effect_index && effect_arg_index == -1) ? 4 : (flanger_conf.status ? 1 : 2);
                if (flanger_conf.status) {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, flanger_conf.pos_y, 2, "[ X ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                else {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, flanger_conf.pos_y, 2, "[   ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                wprintw(window->window_ptr, "   Flanger   ");
                
                if (i == effect_index && effect_arg_index >= flanger_conf.n_args) effect_arg_index = flanger_conf.n_args - 1;

                param_color = (i == effect_index && effect_arg_index == 0) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 0)
                    flanger_conf.values[0] += .05;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 0)
                    flanger_conf.values[0] -= .05;
                if (flanger_conf.values[0] > 1.f) flanger_conf.values[0] = 1.f;
                else if (flanger_conf.values[0] < 0.f) flanger_conf.values[0] = 0.f;
                sprintf(string_number, "[ %.2f ]", flanger_conf.values[0]);
                wprintw(window->window_ptr, " | A ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                param_color = (i == effect_index && effect_arg_index == 1) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 1)
                    flanger_conf.values[1] += .05;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 1)
                    flanger_conf.values[1] -= 0.05;
                if (flanger_conf.values[1] > 1.f) flanger_conf.values[1] = 1.f;
                else if (flanger_conf.values[1] < 0.f) flanger_conf.values[1] = 0.f;
                sprintf(string_number, "[ %.2f ]", flanger_conf.values[1]);
                wprintw(window->window_ptr, " Depth ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                param_color = (i == effect_index && effect_arg_index == 2) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 2)
                    flanger_conf.values[2] += .05;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 2)
                    flanger_conf.values[2] -= .05;
                if (flanger_conf.values[2] < 0.2f) flanger_conf.values[2] = 0.f;
                sprintf(string_number, "[ %.2f ]", flanger_conf.values[2]);
                wprintw(window->window_ptr, " Freq ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "                 ");

                flgr.A = flanger_conf.values[0];
                flgr.delay_gain = flanger_conf.values[1];
                flgr.f = flanger_conf.values[2];
                break;
            case BITCRUSHER:
                status_color = (i == effect_index && effect_arg_index == -1) ? 4 : (bitcrusher_conf.status ? 1 : 2);
                if (bitcrusher_conf.status) {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, bitcrusher_conf.pos_y, 2, "[ X ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                else {
                    wattron(window->window_ptr, COLOR_PAIR(status_color));
                    mvwprintw(window->window_ptr, bitcrusher_conf.pos_y, 2, "[   ]");
                    wattroff(window->window_ptr, COLOR_PAIR(status_color));
                }
                wprintw(window->window_ptr, "   Bitcrusher");

                if (i == effect_index && effect_arg_index >= bitcrusher_conf.n_args) effect_arg_index = bitcrusher_conf.n_args - 1;

                param_color = (i == effect_index && effect_arg_index == 0) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 0)
                    bitcrusher_conf.values[0] += 10;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 0)
                    bitcrusher_conf.values[0] -= 10;
                //if (bitcrusher_conf.values[0] > 1.f) bitcrusher_conf.values[0] = 1.f;
                if (bitcrusher_conf.values[0] < 0.f) bitcrusher_conf.values[0] = 0.f;
                sprintf(string_number, "[ %.0f ]", bitcrusher_conf.values[0]);
                wprintw(window->window_ptr, " | Resol ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                param_color = (i == effect_index && effect_arg_index == 1) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 1)
                    bitcrusher_conf.values[1] += .25;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 1)
                    bitcrusher_conf.values[1] -= 0.25;
                if (bitcrusher_conf.values[1] < 0.f) bitcrusher_conf.values[1] = 0.f;
                sprintf(string_number, "[ %.2f ]", bitcrusher_conf.values[1]);
                wprintw(window->window_ptr, " Depth ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));

                param_color = (i == effect_index && effect_arg_index == 2) ? 4 : 3;
                if (i == effect_index && key_pressed == KEY_UP && effect_arg_index == 2)
                    bitcrusher_conf.values[2] += 1;
                else if (i == effect_index && key_pressed == KEY_DOWN && effect_arg_index == 2)
                    bitcrusher_conf.values[2] -= 1;
                if (bitcrusher_conf.values[2] < 0.f) bitcrusher_conf.values[2] = 0.f;
                sprintf(string_number, "[ %.0f ]", bitcrusher_conf.values[2]);
                wprintw(window->window_ptr, " Decim ");
                wattron(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "%s", string_number);
                wattroff(window->window_ptr, COLOR_PAIR(param_color));
                wprintw(window->window_ptr, "              ");

                bcrush.A = bitcrusher_conf.values[0];
                bcrush.depth = bitcrusher_conf.values[1];
                bcrush.decimation = bitcrusher_conf.values[2];
                break;
        }
    }
    //wrefresh(window->window_ptr);
}


static void set_pedal_status(size_t pedal_index) {
    switch (effects_order[pedal_index]) {
        case TREMOLO:
            tremolo_conf.status = !tremolo_conf.status;
            break;
        case DISTORTION:
            distortion_conf.status = !distortion_conf.status;
            break;
        case FLANGER:
            flanger_conf.status = !flanger_conf.status;
            break;
        case BITCRUSHER:
            bitcrusher_conf.status = !bitcrusher_conf.status;
            break;
        default:
            break;
    }
}


static void move_effect(char move_up_or_down) {
    enum Effects_type effect_temp; 
    if (move_up_or_down == MOVE_UP) {
        effect_index--;
        if (effect_index < 0) {
            effect_index = 0;
            return;
        }
        effect_temp = effects_order[effect_index + 1];
        effects_order[effect_index + 1] = effects_order[effect_index];
        effects_order[effect_index] = effect_temp;
    }
    else if (move_up_or_down == MOVE_DOWN) {
        effect_index++;
        if (effect_index >= NUMBER_OF_EFFECTS) {
            effect_index = NUMBER_OF_EFFECTS - 1;
            return;
        }
        effect_temp = effects_order[effect_index - 1];
        effects_order[effect_index - 1] = effects_order[effect_index];
        effects_order[effect_index] = effect_temp;
    }

}


static void select_pedal_mode(struct Window *window, int char_read) {
    //redraw_pedals(window, MODE_SELECT_PEDAL, char_read);
    switch (char_read) {
        case KEY_UP:
            effect_index--;
            if (effect_index < 0) effect_index = 0;
            break;
        case KEY_DOWN:
            effect_index++;
            if (effect_index >= (NUMBER_OF_EFFECTS)) effect_index = NUMBER_OF_EFFECTS - 1;
            break;
        case ' ':
            set_pedal_status(effect_index);
            break;
        case KEY_SR: // shift up
            move_effect(MOVE_UP);
            break;
        case KEY_SF: // shift down 
            move_effect(MOVE_DOWN);
            break;
    }
    redraw_pedals(window, MODE_SELECT_PEDAL, char_read);
}


static void plot(struct Window *window_input) {
    const int center_line = window_input->height / 2;
    plotting = buffer_ready_to_plot;
    if (plotting) {
        if (play_flag) {
        /* Clean plot area */
        for (int line = 1; line < window_input->height - 1; line++) {
            for (int column = 1; column < window_input->width - 1; column++) {
                mvwprintw(window_input->window_ptr, line, column, " ");
            }
        }
        /* Plot input */
        wattron(window_input->window_ptr, COLOR_PAIR(5));
        for (int column = 0; column < window_input->width - 4; column++) {
            int y = center_line - buffer_plot_input[column * PLOT_DECIMATION] *
                                  (- 1 + window_input->height / 2);
            mvwprintw(window_input->window_ptr, y, column + 2, "o");
        }
        wattroff(window_input->window_ptr, COLOR_PAIR(5));
        wattron(window_input->window_ptr, COLOR_PAIR(6));
        for (int column = 0; column < window_input->width - 4; column++) {
            int y = center_line - buffer_plot_output[column * PLOT_DECIMATION] *
                                  (- 1 + window_input->height / 2);
            mvwprintw(window_input->window_ptr, y, column + 2, "x");
        }
        wattroff(window_input->window_ptr, COLOR_PAIR(6));
        buffer_ready_to_plot = 0;
        }
    }
    //wrefresh(window_input->window_ptr);
    plotting = 0;
}


enum Window_mode window_mode_normal(int char_read) {
 
    enum Window_mode win_mode = MODE_NORMAL;

    switch (char_read) {
        case 'q':
            win_mode = MODE_QUIT;
            break;
        case 's':
            win_mode = MODE_SELECT_PEDAL;
            effect_index = 0;
            break;
        case 'p':
            play_flag = !play_flag;
        default:
            break;
    }
    return win_mode;
}


enum Window_mode window_mode_select_pedal(int char_read, struct Window* window_pedals) {
 
    enum Window_mode win_mode = MODE_SELECT_PEDAL;

    switch (char_read) {
        case 'q':
            win_mode = MODE_QUIT;
            break;
        case 'n':
            effect_index = -1;
            select_pedal_mode(window_pedals, char_read);
            win_mode = MODE_NORMAL;
            break;
        case 'p':
            play_flag = !play_flag;
            break;
        case KEY_RIGHT:
            effect_arg_index++;
            win_mode = MODE_EDIT_ARGS;
            break;
        default:
            select_pedal_mode(window_pedals, char_read);
            break;
    }
    return win_mode;
}


enum Window_mode edit_args_mode(struct Window *window, int char_read) {
    enum Window_mode mode = MODE_EDIT_ARGS;

    //redraw_pedals(window, MODE_EDIT_ARGS, char_read);
    switch (char_read) {
        case KEY_RIGHT:
            effect_arg_index++;
            break;
        case KEY_LEFT:
            effect_arg_index--;
            if (effect_arg_index == -1) mode = MODE_SELECT_PEDAL;
            break;
    }
    redraw_pedals(window, MODE_SELECT_PEDAL, char_read);
    return mode;
}

enum Window_mode window_mode_edit_args(int char_read, struct Window* window_pedals) {
    enum Window_mode win_mode = MODE_EDIT_ARGS;
    
    switch (char_read) {
        case 'q':
            win_mode = MODE_QUIT;
            break;
        case 'n':
            effect_index = -1;
            effect_arg_index = -1;
            win_mode = MODE_NORMAL;
            select_pedal_mode(window_pedals, char_read);
            break;
        case 'p':
            play_flag = !play_flag;
            break;
        case 's':
            win_mode = MODE_SELECT_PEDAL;
            break;
        default:
            win_mode = edit_args_mode(window_pedals, char_read);
    }

    return win_mode;
}


enum Window_mode window_modes_process(enum Window_mode win_mode, int char_read,
                                      struct Window* window_pedals) {

    switch (win_mode) {
        case MODE_NORMAL:
            win_mode = window_mode_normal(char_read);
            break;
        case MODE_SELECT_PEDAL:
            win_mode = window_mode_select_pedal(char_read, window_pedals);
            break;
        case MODE_QUIT:
            break;
        case MODE_EDIT_ARGS:
            win_mode = window_mode_edit_args(char_read, window_pedals);
            break;
    }
    return win_mode;
}


void draw_mode(struct Window* window, enum Window_mode win_state) {

    wattron(window->window_ptr, COLOR_PAIR(4));
    switch (win_state) {
        
        case MODE_NORMAL:
            mvwprintw(window->window_ptr, 0, 0, "  NORMAL        ");
            break;
        case MODE_SELECT_PEDAL:
            mvwprintw(window->window_ptr, 0, 0, "  SELECT PEDAL  ");
            break;
        case MODE_QUIT:
            mvwprintw(window->window_ptr, 0, 0, "  QUIT          ");
            break;
        case MODE_EDIT_ARGS:
            mvwprintw(window->window_ptr, 0, 0, "  EDIT PARAMS   ");
            break;
    }
    
    wattroff(window->window_ptr, COLOR_PAIR(4));
    //wrefresh(window->window_ptr);
}


int window_manager(void *arg) {
    (void) arg;

    struct Window window_title = {.width = screen_pedals_width, .height = 4,
                                  .pos = {.x = 2, .y = 2}};
    struct Window window_pedals = {.width = screen_pedals_width, .height = 30,
                                   .pos = {.x = window_title.pos.x,
                                           .y = window_title.pos.y + window_title.height}};
    struct Window window_help = {.width = screen_pedals_width, .height = 4,
                                 .pos = {.x = window_title.pos.x,
                                         .y = window_pedals.pos.y + window_pedals.height}};
    struct Window window_plot = {.width = 128, .height = 50,
                                 .pos = {.x = window_pedals.width +  window_pedals.pos.x,
                                         .y = window_title.pos.y}};
    struct Window window_mode = {.width = screen_pedals_width, . height = 2,
                                 .pos = {.x = window_title.pos.x,
                                         .y = window_help.pos.y + window_help.height}};
    int char_read = 0;

    window_flag = 1;

    initscr(); 
    timeout(50);
    refresh();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (!has_colors()) {
        printw("Terminal does not support color");
        getch();
        return -1;
    }

    start_color();
    
    init_pair(1, COLOR_WHITE, COLOR_GREEN);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_WHITE, COLOR_YELLOW);
    init_pair(6, COLOR_WHITE, COLOR_BLUE);
    /* n_rows, n_columns, pos_y, pos_x */
    window_title.window_ptr = newwin(window_title.height, window_title.width,
                                     window_title.pos.y, window_title.pos.x);
    box(window_title.window_ptr, 0, 0);
    mvwprintw(window_title.window_ptr, 1, 1, "CAPITU PEDAL");
    mvwprintw(window_title.window_ptr, 2, 1, "By Adailton Braga Júnior");
    wrefresh(window_title.window_ptr);

    /* n_rows, n_columns, pos_y, pos_x */
    window_pedals.window_ptr = newwin(window_pedals.height, window_pedals.width,
                                      window_pedals.pos.y, window_pedals.pos.x);
    box(window_pedals.window_ptr, 0, 0); 
    mvwprintw(window_pedals.window_ptr, 0, 2, "Pedals");
    redraw_pedals(&window_pedals, MODE_NORMAL, 0); 

    wrefresh(window_pedals.window_ptr);

    window_help.window_ptr = newwin(window_help.height, window_help.width,
                                    window_help.pos.y, window_help.pos.x);
    box(window_help.window_ptr, 0, 0);
    mvwprintw(window_help.window_ptr, 0, 2, "Help");
    mvwprintw(window_help.window_ptr, 1, 1, "q - Quit");
    mvwprintw(window_help.window_ptr, 2, 1, "p - Play/Pause");
    mvwprintw(window_help.window_ptr, 1, 20, "s - select pedals");
    mvwprintw(window_help.window_ptr, 2, 20, "n - normal mode");
    wrefresh(window_help.window_ptr);

    window_plot.window_ptr = newwin(window_plot.height, window_plot.width,
                                    window_plot.pos.y, window_plot.pos.x);
    box(window_plot.window_ptr, 0, 0);
    mvwprintw(window_plot.window_ptr, 0, 2, "Signals");
    wattron(window_plot.window_ptr, COLOR_PAIR(5));
    mvwprintw(window_plot.window_ptr, 0, 12, "[input]");
    wattroff(window_plot.window_ptr, COLOR_PAIR(5));
    wattron(window_plot.window_ptr, COLOR_PAIR(6));
    mvwprintw(window_plot.window_ptr, 0, 21, "[output]");
    wattroff(window_plot.window_ptr, COLOR_PAIR(6));
    wrefresh(window_plot.window_ptr);

    window_mode.window_ptr = newwin(window_mode.height, window_mode.width,
                                    window_mode.pos.y, window_mode.pos.x);

    enum Window_mode win_mode = MODE_NORMAL;
    /* Ncurses main loop */
    do {
        //char_read = getch();
        //wmove(0,0,window_pedals.window_ptr);
        //char_read = wgetch(window_pedals.window_ptr);
        char_read = wgetch(stdscr);
        win_mode = window_modes_process(win_mode, char_read, &window_pedals);
        if (win_mode == MODE_QUIT) {
            play_flag = 0;
            window_flag = 0;
        }

        draw_mode(&window_mode, win_mode);
        plot(&window_plot);
        //wrefresh(window_title.window_ptr);
        wrefresh(window_plot.window_ptr);
        //wrefresh(window_help.window_ptr);
        wrefresh(window_mode.window_ptr);
        wrefresh(window_pedals.window_ptr);
    } while(window_flag);

    delwin(window_pedals.window_ptr);
    delwin(window_title.window_ptr);
    delwin(window_help.window_ptr);
    delwin(window_plot.window_ptr);
    delwin(window_mode.window_ptr);
    endwin();

    return 0;
}
