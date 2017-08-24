#include <csignal>
#include <string>
#include <utility>
#include "common.cpp"
#include "solver.cpp"


static constexpr double TIME_LIMIT = 150.0;


int main(int argc, char* argv[]) {

    static paiv::u8 cancelled = 0;

    signal(SIGINT, [](int){ cancelled = 1; });


    auto timelimit = TIME_LIMIT;
    const char* v = getenv("PAIV_TIMEOUT");
    if (v != nullptr) {
        timelimit = stod(v);
    }


    return paiv::solve(cin, cout, timelimit - 0.5, cancelled);
}
