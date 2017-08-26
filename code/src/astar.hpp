#pragma once

#include <forward_list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace paiv {
namespace astar {

using namespace std;


template<typename Graph,
    typename Location = typename Graph::Location,
    typename Distance = typename Graph::Distance>
class search {
    Graph& graph;
public:
    search(Graph& graph) : graph(graph) {
    }

    vector<Location>
    operator () (const Location& from, const Location& goal, const r64 timelimit) {
        auto started = appclock::now();

        unordered_map<Location, Distance> distance;
        unordered_map<Location, Location> parent;
        // unordered_set<Location> visited;

        typedef struct node {
            Location location;
            Distance cost;
        } node;

        static const auto cost_comp = [](const node& a, const node& b) {
            return a.cost > b.cost;
        };

        priority_queue<node, vector<node>, decltype(cost_comp)>
        fringe(cost_comp);

        fringe.push({from, 0});

        while (!fringe.empty()) {
            auto now = appclock::now();
            chrono::duration<r64> elapsed = now - started;
            auto timeleft = timelimit - elapsed.count();
            if (timeleft <= 0) {
                break;
            }

            auto current = fringe.top();
            fringe.pop();

            if (graph.check_goal(current.location, goal)) {
                return _rebuild_path(current.location, parent);
            }

            // visited.insert(current.location);

            for (auto child : graph.children(current.location)) {
                // if (visited.find(child) != end(visited)) {
                //     continue;
                // }

                auto dist = distance[current.location] + graph.distance(current.location, child);
                auto it = distance.find(child);

                if (it == end(distance) || it->second > dist) {
                    parent[child] = current.location;
                    distance[child] = dist;

                    auto cost = dist + graph.path_estimate(child, goal);
                    fringe.push({child, cost});
                }
            }
        }

        return {};
    }

private:
    vector<Location>
    _rebuild_path(const Location& goal, const unordered_map<Location, Location>& parent) const {
        forward_list<Location> path;
        auto loc = goal;
        while (1) {
            path.push_front(loc);
            auto it = parent.find(loc);
            if (it == end(parent)) {
                break;
            }
            loc = it->second;
        }
        return vector<Location>(begin(path), end(path));
    }
};


}
}
