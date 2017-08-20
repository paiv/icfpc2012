#include <unistd.h>
#include "common.cpp"


namespace paiv {


s32
viz(istream& si, ostream& so, u32 delay) {
    auto game = read_map(si);
    auto prog = read_program(si);

    auto& map = getmap(game);
    auto state = getsim(game);

    static auto& sso = so;
    static auto sdelay = delay;

    state = runsim(map, state, prog, [](const map_info& map, const sim_state& state){
        sso << setw(map.width) << state.board << endl;
        usleep(sdelay * 1000);
    });

    so << prog << endl;
    so << state.score << endl;

    return 0;
}


}


int main(int argc, char* argv[]) {

    int delay = 300;

    if (argc > 1 && string(argv[1]) == "--delay") {
        if (argc > 2) {
            delay = stoi(argv[2]);
        }
    }

    return paiv::viz(cin, cout, delay);
}
