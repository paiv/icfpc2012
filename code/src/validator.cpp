#include "common.cpp"


namespace paiv {


s32
validate(istream& si, ostream& so) {
    auto game = read_map(si);
    auto prog = read_program(si);

    auto& map = getmap(game);
    auto state = getsim(game);

    state = runsim(map, state, prog);

    so << state.score << endl;
    so << setw(map.width) << state.board << endl;
    so << prog << endl;

    return 0;
}


}


int main(int argc, char* argv[]) {

    return paiv::validate(cin, cout);
}
