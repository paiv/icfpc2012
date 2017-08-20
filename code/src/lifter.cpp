#include <csignal>
#include "common.cpp"
#include "solver.cpp"


int main(int argc, char* argv[]) {

    static u8 cancelled = 0;

    signal(SIGINT, [](int){ cancelled = 1; });

    return paiv::solve(cin, cout, cancelled);
}
