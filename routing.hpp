// routing.hpp -- UNIT IV: custom functors for priority-based evacuation routing
#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace DisasterCore::Routing {

struct Zone    { std::string name; double hazard; int population; };
struct Shelter { std::string name; int capacity; };

struct EvacuationPriority {                                        // comparator functor for std::sort
    bool operator()(const Zone& a, const Zone& b) const {
        if (a.hazard != b.hazard) return a.hazard > b.hazard;      // most dangerous first
        return a.population > b.population;                       // then most people
    }
};

class EvacuationRouter {                                           // stateful functor for std::for_each
    std::vector<Shelter> shelters;
    std::map<std::string, std::string> assigned;
public:
    explicit EvacuationRouter(std::vector<Shelter> s) : shelters(std::move(s)) {}
    void operator()(const Zone& z) {
        if (z.hazard < 50.0) return;                               // only WARNING+ zones evacuate
        auto best = std::max_element(shelters.begin(), shelters.end(),
                        [](const Shelter& a, const Shelter& b) { return a.capacity < b.capacity; });
        assigned[z.name] = best->name + (best->capacity < z.population ? " (OVER CAPACITY)" : "");
        best->capacity -= z.population;
    }
    const std::map<std::string, std::string>& routes() const { return assigned; }
};

} // namespace DisasterCore::Routing
