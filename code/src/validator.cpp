#include "common.cpp"


namespace paiv {


s32
validate(istream& si, ostream& so) {
    auto state = read_map(si);
    auto prog = read_program(si);

    state = runsim(state, prog);

    so << state.score << endl;
    so << setw(state.width) << state.board << endl;
    so << prog << endl;

    return 0;
}


}


int main(int argc, char* argv[]) {

    return paiv::validate(cin, cout);
}
