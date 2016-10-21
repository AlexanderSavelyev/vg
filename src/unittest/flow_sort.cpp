#include "catch.hpp"
#include "vg.hpp"
#include "xg.hpp"
#include "vg.pb.h"

namespace vg {
namespace unittest {

using namespace std;

TEST_CASE("sorts input graph using max flow approach", "[sort_flow]") {
    SECTION("Sort simple graph") {
        // Build the read
        // Build a toy graph
//    const string graph_json = R"(
//    
//    {"node": [{"sequence": "C", "name": "1", "id": 1}, {"sequence": "G", "name": "2", "id": 2}, {"sequence": "C", "name": "3", "id": 3}, {"sequence": "G", "name": "4", "id": 4}, {"sequence": "C", "name": "5", "id": 5}, {"sequence": "T", "name": "6", "id": 6}, {"sequence": "C", "name": "7", "id": 7}, {"sequence": "A", "name": "8", "id": 8}, {"sequence": "C", "name": "9", "id": 9}, {"sequence": "G", "name": "10", "id": 10}, {"sequence": "A", "name": "11", "id": 11}, {"sequence": "T", "name": "12", "id": 12}, {"sequence": "C", "name": "13", "id": 13}], "edge": [{"from": 1, "to": 2, "overlap": 1}, {"from": 1, "to": 6, "overlap": 1}, {"from": 2, "to": 10, "overlap": 1}, {"from": 2, "to": 11, "overlap": 1}, {"from": 3, "to": 5, "overlap": 1}, {"from": 3, "to": 4, "overlap": 1}, {"from": 4, "to": 5, "overlap": 1}, {"from": 5, "to": 11, "overlap": 1}, {"from": 6, "to": 8, "overlap": 1}, {"from": 6, "to": 7, "overlap": 1}, {"from": 7, "to": 7, "overlap": 1}, {"from": 7, "to": 8, "overlap": 1}, {"from": 8, "to": 9, "overlap": 1}, {"from": 10, "to": 12, "overlap": 1}, {"from": 10, "to": 9, "overlap": 1}, {"from": 11, "to": 12, "overlap": 1}, {"from": 11, "to": 3, "overlap": 1}, {"from": 12, "to": 13, "overlap": 1}, {"from": 9, "to": 12, "overlap": 1}]}
//    
//    )";
//        
//        VG graph;
//        Graph chunk;
//        json2pb(chunk, graph_json.c_str(), graph_json.size());
//        graph.merge(chunk);
//        
//        REQUIRE(graph.length() == 13);
    }
}

}
}