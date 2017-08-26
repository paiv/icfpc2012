#include <csignal>
#include <string>
#include <utility>
#include "common.cpp"
#include "solver.cpp"


static constexpr double TIME_LIMIT = 150.0;


#if 0
#include "astar.hpp"
using namespace paiv;

game_state
read_map(const string& s) {
    stringstream so(s);
    return read_map(so);
}

struct search_graph {
    typedef struct {
        sim_state sim;
        program_t path;
    } search_state;

    typedef search_state Node;
    typedef program_t Location;
    typedef program_t Path;
    typedef s32 Distance;

    static constexpr Distance MaxDistance = s32_max;

    const map_info& map;
    const Location root;
    unordered_map<Location, search_state> tree;
    unordered_set<pos> visited_pos;
    unordered_set<u64> visited;

    search_graph(const map_info& map, const sim_state& initial) : map(map), root(0) {
        tree[{}] = {initial, {}};
    }

    Location stub_node(const pos& at) {
        search_state state = {};
        state.sim.robot_pos = at;
        state.path = { action::abort };
        tree[state.path] = state;
        return state.path;
    }

    bool check_goal(const Location& at, const Location& goal) const {
        // logger << "  check goal from " << at << " to " << goal << endl;
        auto fromit = tree.find(at);
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            // logger << "  check current: " << fromit->second.sim.robot_pos << endl;
            // logger << "  check goal: " << goalit->second.sim.robot_pos << endl;
            return fromit->second.sim.robot_pos == goalit->second.sim.robot_pos;
        }
        return false;
    }

    vector<Location> children(const Location& from) {
        logger << "children from " << from << endl;
        auto fromit = tree.find(from);
        if (fromit == end(tree)) {
            return {};
        }

        auto parent = fromit->second;

        visited.insert(parent.sim.board_hash);
        visited_pos.insert(parent.sim.robot_pos);

        vector<Location> res;

        auto moves = legal_moves(map, parent.sim, parent.path);

        for (auto mv : moves) {
            auto sim = simulator_step(map, parent.sim, mv);
            auto path = parent.path;
            path.push_back(mv);

            if (visited.find(sim.board_hash) != end(visited)) {
                continue;
            }

            logger << "  child from " << parent.sim.robot_pos << " " << path << endl;

            tree[path] = {sim, path};
            res.push_back(path);
        }

        return res;
    }

    Path path(const Node& from, const Node& to) const {
        // logger << "  path from: " << from.path << ", to: " << to.path << endl;
        auto it = begin(to.path);
        for (auto x = begin(from.path);
            x != end(from.path) && it != end(to.path) && *x == *it;
            x++, it++) {
        }

        return Path(it, end(to.path));
    }

    Distance distance(const Location& from, const Location& to) const {
        auto fromit = tree.find(from);
        auto toit = tree.find(to);
        if (fromit != end(tree) && toit != end(tree)) {
            return path(fromit->second, toit->second).size();
        }
        return MaxDistance;
    }

    Distance path_estimate(const Location& from, const Location& goal) const {
        auto fromit = tree.find(from);
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            return manhattan_distance(fromit->second.sim.robot_pos, goalit->second.sim.robot_pos);
        }
        return MaxDistance;
    }

    Path map_path(const vector<Location>& locs) const {
        auto from = locs.front();
        auto goal = locs.back();

        auto fromit = tree.find(from);
        auto goalit = tree.find(goal);

        #if 0
        logger << "  map path from " << fromit->second.sim.robot_pos << " to " << goalit->second.sim.robot_pos << endl;

        for (auto x : locs) {
            logger << "  " << x;
            auto it = tree.find(x);
            if (it != end(tree)) {
                logger << "  " << it->second.sim.robot_pos << "  " << it->second.path;
            }
            logger << endl;
        }
        #endif

        if (fromit != end(tree) && goalit != end(tree)) {
            return path(fromit->second, goalit->second);
        }

        return {};
    }
};


int static
test_astar() {
    auto game = read_map(
"######\n"
"#. *R#\n"
"#  \\.#\n"
"#\\ * #\n"
"L  .\\#\n"
"######\n"
);
    auto map = getmap(game);
    auto sim = getsim(game);

    clog << setw(map.width) << sim.board << endl;

    search_graph gen(map, sim);
    auto goal = gen.stub_node({4,4});

    astar::search<search_graph> find_path(gen);
    auto trail = find_path(gen.root, goal);
    auto path = trail.back();

    unordered_set<pos> path_set;
    auto state = getsim(game);
    for (auto mv : path) {
        state = simulator_step(map, state, mv);
        // logger << "  sim " << (char)mv << "  " << state.robot_pos << endl;
        path_set.insert(state.robot_pos);
    }

    for (coord row = 0; row < map.height; row++) {
        for (coord col = 0; col < map.width; col++) {
            auto x = sim.board[row * map.width + col];
            switch (x) {
                case cell::robot:
                case cell::lift:
                case cell::openlift:
                case cell::wall:
                    clog << (char)symb_from_cell(x);
                    break;
                default:
                    if (path_set.find({col,row}) != end(path_set)) {
                        clog << '"';
                    }
                    else if (gen.visited_pos.find({col,row}) != end(gen.visited_pos)) {
                        clog << '.';
                    }
                    else {
                        clog << (char)symb_from_cell(x);
                    }
                    break;
            }
        }
        clog << "\n";
    }
    clog << path << endl;

    return 0;
}
#endif


int main(int argc, char* argv[]) {

    // return test_astar();

    static paiv::u8 cancelled = 0;

    signal(SIGINT, [](int){ cancelled = 1; });


    auto timelimit = TIME_LIMIT;
    const char* v = getenv("PAIV_TIMEOUT");
    if (v != nullptr) {
        timelimit = stod(v);
    }


    return paiv::solve(cin, cout, timelimit - 0.5, cancelled);
}
