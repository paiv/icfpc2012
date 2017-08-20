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
        assert(getmap(m).width == 0 && getmap(m).height == 0);
        assert(getsim(m).board == board_t({}));
    }

    {
        auto m = read_map("#");
        assert(getmap(m).width == 1 && getmap(m).height == 1);
        assert(getsim(m).board == board_t({cell::wall}));
    }

    {
        auto m = read_map("###");
        assert(getmap(m).width == 3 && getmap(m).height == 1);
        assert(getsim(m).board == board_t({cell::wall, cell::wall, cell::wall}));
    }

    {
        auto m = read_map("#\n#");
        assert(getmap(m).width == 1 && getmap(m).height == 2);
        assert(getsim(m).board == board_t({cell::wall, cell::wall}));
    }

    {
        auto m = read_map("##\n#");
        assert(getmap(m).width == 2 && getmap(m).height == 2);
        assert(getsim(m).board == board_t({
            cell::wall, cell::wall,
            cell::wall, cell::empty,
        }));
    }

    {
        auto m = read_map("#\n");
        assert(getmap(m).width == 1 && getmap(m).height == 1);
        assert(getsim(m).board == board_t({cell::wall}));
    }

    {
        auto m = read_map("#\n\n#");
        assert(getmap(m).width == 1 && getmap(m).height == 1);
        assert(getsim(m).board == board_t({cell::wall}));
    }

    {
        auto m = read_map("R");
        assert(getsim(m).robot_pos == pos({0,0}));
    }

    {
        auto m = read_map("#\n#R");
        assert(getsim(m).robot_pos == pos({1,1}));
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
        auto m = read_map("L\\R");
        auto state = simulator_step(getmap(m), getsim(m), action::left);
        assert(state.robot_pos == pos({1,0}));
        state = simulator_step(getmap(m), state, action::left);
        assert(state.is_ended);
        assert(state.lambdas_collected == 1);
        assert(state.score == 73);
    }

    {
        auto m = read_map("* \n  \n R\nL#");
        auto state = simulator_step(getmap(m), getsim(m), action::left);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto m = read_map("* \n R\nL#");
        auto state = simulator_step(getmap(m), getsim(m), action::left);
        assert(state.robot_pos == pos({0,1}));
        assert(!state.is_ended);
        assert(state.score == -1);
        state = simulator_step(getmap(m), state, action::down);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -2);
    }

    {
        auto m = read_map("   \n * \n.*R\nL##");
        auto state = simulator_step(getmap(m), getsim(m), action::wait);
        assert(state.robot_pos == pos({2,2}));
        assert(!state.is_ended);
        assert(state.score == -1);
    }

    {
        auto m = read_map("* \n  \n R\nL#");
        auto state = simulator_step(getmap(m), getsim(m), action::left);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto m = read_map("*\nR\n \nL#");
        auto state = simulator_step(getmap(m), getsim(m), action::down);
        assert(state.robot_pos == pos({0,2}));
        assert(state.is_ended);
        assert(state.score == -1);
    }

    {
        auto m = read_map("* \n  \n  \nL\\R");
        auto state = simulator_step(getmap(m), getsim(m), action::left);
        state = simulator_step(getmap(m), state, action::left);
        assert(state.robot_pos == pos({0,3}));
        assert(state.is_ended);
        assert(state.score == 73);
    }
}


void
test_sim() {
    {
        auto m = read_map("L\\R");
        auto state = runsim(getmap(m), getsim(m), read_program("WWWLL"));
        assert(state.is_ended);
        assert(state.lambdas_collected == 0);
        assert(state.score == -3);
    }

    {
        auto m = read_map("L\\R");
        static u32 x = 0;
        auto state = runsim(getmap(m), getsim(m), read_program("LL"), [](const map_info&, const sim_state&){ x++; });
        assert(x == 5); // initial + 2 x move+update
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
