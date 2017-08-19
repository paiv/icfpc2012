#include <cassert>
#include <string>
#include <sstream>
#include "../src/common.cpp"


namespace paiv {


game_state
read_map(const string& s) {
    stringstream so(s);
    return read_map(so);
}


void
test_map_reader() {
    {
        auto m = read_map("");
        assert(m.width == 0 && m.height == 0);
        assert(m.board == board_t({}));
    }

    {
        auto m = read_map("#");
        assert(m.width == 1 && m.height == 1);
        assert(m.board == board_t({ {cell::wall} }));
    }

    {
        auto m = read_map("###");
        assert(m.width == 3 && m.height == 1);
        assert(m.board == board_t({ {cell::wall, cell::wall, cell::wall} }));
    }

    {
        auto m = read_map("#\n#");
        assert(m.width == 1 && m.height == 2);
        assert(m.board == board_t({ {cell::wall}, {cell::wall} }));
    }

    {
        auto m = read_map("##\n#");
        assert(m.width == 2 && m.height == 2);
        assert(m.board == board_t({
            {cell::wall, cell::wall},
            {cell::wall, cell::empty},
        }));
    }

    {
        auto m = read_map("#\n");
        assert(m.width == 1 && m.height == 1);
        assert(m.board == board_t({ {cell::wall} }));
    }

    {
        auto m = read_map("#\n\n#");
        assert(m.width == 1 && m.height == 1);
        assert(m.board == board_t({ {cell::wall} }));
    }

    {
        auto m = read_map("R");
        assert(m.robot_pos == pos({0,0}));
    }

    {
        auto m = read_map("#\n#R");
        assert(m.robot_pos == pos({1,1}));
    }
}


void
test_program_reader() {
    {
        auto prog = read_program("");
        assert(prog == program_t({}));
    }

    {
        auto prog = read_program("LRUDWA");
        assert(prog == program_t({action::left, action::right, action::up, action::down, action::wait, action::abort}));
    }

    {
        auto prog = read_program("A -- U");
        assert(prog == program_t({action::abort, action::up}));
    }

    {
        auto prog = read_program("U\nD");
        assert(prog == program_t({action::up}));
    }
}


void
test_sim_step() {
    {
        auto state = read_map("L\\R");
        state = run_step(state, action::left);
        assert(state.robot_pos == pos({1,0}));
        state = run_step(state, action::left);
        assert(state.is_ended);
        assert(state.lambdas_collected == 1);
        assert(state.score == 73);
    }

    {
        auto state = read_map("* \n  \n R\nL#");
        state = run_step(state, action::left);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto state = read_map("* \n R\nL#");
        state = run_step(state, action::left);
        assert(state.robot_pos == pos({0,1}));
        assert(!state.is_ended);
        assert(state.score == -1);
        state = run_step(state, action::down);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -2);
    }

    {
        auto state = read_map("   \n * \n.*R\nL##");
        state = run_step(state, action::wait);
        assert(state.robot_pos == pos({2,2}));
        assert(!state.is_ended);
        assert(state.score == -1);
    }

    {
        auto state = read_map("* \n  \n R\nL#");
        state = run_step(state, action::left);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto state = read_map("*\nR\n \nL#");
        state = run_step(state, action::down);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto state = read_map("* \n  \n  \nL\\R");
        state = run_step(state, action::left);
        state = run_step(state, action::left);
        assert(state.robot_pos == pos({0,3}));
        assert(state.is_ended);
        assert(state.score == 73);
    }
}


void
test_sim() {
    {
        auto state = read_map("L\\R");
        state = runsim(state, read_program("WWWLL"));
        assert(state.is_ended);
        assert(state.lambdas_collected == 0);
        assert(state.score == -3);
    }
}


int
test() {
    test_map_reader();
    test_program_reader();
    test_sim_step();
    test_sim();
    return 0;
}


}


int main(int argc, char* argv[]) {
    return paiv::test();
}
