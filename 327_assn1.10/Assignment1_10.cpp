#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <locale.h>

using namespace std; // if this game were to be updated this would be removed, but for now since it's fairly simple I added this in

struct Decoration {
    string name;
    wchar_t symbol;
};

class Cake {
    private:
    vector<wstring> layers;
    public:
    void createLayers(int numLayers) {
        static const vector<wstring> templates = {
            L"      |--|      ",
            L"     |----|     ",
            L"    |------|    ",
            L"   |--------|   ",
            L"  |----------|  ",
            L" |------------| ",
            L"|--------------|"
        };
        if (numLayers > 0 && numLayers <= (int)templates.size()) {
            layers = vector<wstring>(templates.begin(), templates.begin() + numLayers);
        }
    }

    void addDecoration(int layer, wchar_t decor) {
        if (layer >= 0 && layer < (int)layers.size()) {
            for (size_t i = 0; i < layers[layer].length(); i++) {
                if (layers[layer][i] == '-') {
                    layers[layer][i] = decor;
                    break;
                }
            }
        }
    }

    void display(int start_row) const {
        for (int i = layers.size() - 1; i >= 0; --i) {
            mvaddwstr(start_row + i, 10, layers[i].c_str());
        }
    }

    int getLayerCount() const {
        return layers.size();
    }
};

void init_ncurses() {
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    setlocale(LC_ALL, "");
    if (has_colors()) {
        start_color();
        if (can_change_color()) {
            init_color(COLOR_RED, 500, 250, 0);
            init_pair(1, COLOR_YELLOW, COLOR_BLACK);
            init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(3, COLOR_RED, COLOR_BLACK);
        } else {
            init_pair(1, COLOR_YELLOW, COLOR_BLACK);
            init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(3, COLOR_RED, COLOR_BLACK);
        }
    }
}

int pick_layers() {
    int num_layers;
    echo();
    while (true) {
        clear();
        mvprintw(0, 0, "Please type in how many layers you would like. You can pick between 1-6: ");
        refresh();
        // found doing this was easier for this specific method rather than switch-case for 1-6
        scanw("%d", &num_layers);

        // checking if it's between 1-6
        if (num_layers >= 1 && num_layers <= 6) {
            break;
        } else {
            mvprintw(1, 0, "Invalid input. Please enter a number between 1 and 6.");
            refresh();
            sleep(2);
        }
    }
    noecho();
    return num_layers;
}

int pick_flavor() {
    int flavor;
    int done = 0;
    while (!done) {
        clear();
        mvprintw(0, 0, "Please pick your cake flavor. Type v for vanilla, s for strawberry, and c for chocolate: ");
        refresh();

        switch(getch()) {
            case 'v':
                flavor = 1; // this number later will correspond to the correct text color
                done = 1;
                break;
            case 's':
                flavor = 2;
                done = 1;
                break;
            case 'c':
                flavor = 3;
                done = 1;
                break;
            default:
                break;            
        }
    }

    return flavor;
}

// very similar to picking decoration type, but for the cake topper
int pick_topper(vector<Decoration>& toppers) {
    int done = 0;
    int current_topper = 0;
    while (!done) {
        clear();
        mvprintw(0, 0, "Pick your cake topper: %s\nUse LEFT and RIGHT to navigate, press 'd' when done.", toppers[current_topper].name.c_str());
        refresh();

        switch(getch()) {
            case KEY_RIGHT:
                current_topper = (current_topper + 1) % toppers.size();
                break;
            case KEY_LEFT:
                current_topper = (current_topper - 1 + toppers.size()) % toppers.size();
                break;
            case 'd':
                done = 1;
                break;
        }
    }

    return current_topper;
}

void display_final_cake(Cake& cake, Decoration& topper, int flavor) {
    clear();
    mvprintw(0, 0, "Congratulations on your cake!");
    mvprintw(1, 0, "Press 'q' to quit.");

    int start_row = 5;
    attron(COLOR_PAIR(flavor));
    cake.display(start_row);
    attroff(COLOR_PAIR(flavor));

    // adding topper (some aren't completely centered.. sorry)
    if (topper.symbol != ' ') {
        attron(COLOR_PAIR(1));
        mvaddwstr(start_row - 1, 17, wstring(1, topper.symbol).c_str());
        attroff(COLOR_PAIR(1));
    }

    // put the cake on a plate!
    mvprintw(start_row + cake.getLayerCount(), 10, "================");
    refresh();
}

int main() {
    int num_layers;
    init_ncurses();
    // user picks layers
    num_layers = pick_layers();

    int flavor;
    // user picks cake flavor
    flavor = pick_flavor();

    clear();
    mvprintw(0, 0, "Press the arrow keys to choose the decoration type. (Left and right arrow keys)");
    mvprintw(1, 0, "Press numbers 1-%d to add a decoration to a corresponding layer. Keep in mind layer 1 refers to the top, not the bottom.", num_layers);
    mvprintw(2, 0, "Press 'd' when you're done.\nTo see these instructions again press ?\nPress any key to continue.");
    refresh();
    getch();

    Cake cake;
    cake.createLayers(num_layers);
    vector<Decoration> decorations = {
        {"Cherry", '*'},
        {"Sprinkle", ':'},
        {"Star", L'★'},
        {"Heart", L'♥'},
        {"Money", '$'},
        {"Diamond", L'✦'},
        {"Flower", L'❀'},
        {"Frosting Stripe", '='},
        {"Frosting Wave", '~'}
    };
    vector<Decoration> toppers = {
        {"None", ' '},
        {"Candles", L'🕯'},
        {"Crown", L'♔'},
        {"Heart", L'❣'}
    };

    int ch;
    int current_decoration = 0;
    int current_topper;
    int quit = 0;

    // Game loop
    while (!quit) {
        clear();
        mvprintw(0, 0, "Press 'd' when you're done.\nPress 'q' to quit.\nUse arrow keys to choose decoration: %s\nPress ? to see instructions again.", 
        decorations[current_decoration].name.c_str());
        attron(COLOR_PAIR(flavor));
        cake.display(5);
        attroff(COLOR_PAIR(flavor));
        refresh();

        switch(ch = getch()) {
            case KEY_RIGHT:
                current_decoration = (current_decoration + 1) % decorations.size();
                break;
            case KEY_LEFT:
                current_decoration = (current_decoration - 1 + decorations.size()) % decorations.size();
                break;
            case '?':
                clear();
                mvprintw(0, 0, "Press the arrow keys to choose the decoration type. (Left and right arrow keys)");
                mvprintw(1, 0, "Press numbers 1-%d to add a decoration to a corresponding layer.", num_layers);
                mvprintw(2, 0, "To see these instructions again press ?\nPress any key to continue.");
                refresh();
                getch();
                break;
            case 'q':
                quit = 1;
                break;
            case 'd':
                current_topper = pick_topper(toppers);
                display_final_cake(cake, toppers[current_topper], flavor);

                do {
                } while (getch() != 'q');
                endwin();
                return 0;
            default:
                if (ch >= '1' && ch < '1' + num_layers) {
                    int layer_index = ch - '1';  // Convert char to layer index
                    cake.addDecoration(layer_index, decorations[current_decoration].symbol);
                }
                break;
        }
    }

    endwin();
    return 0;
}