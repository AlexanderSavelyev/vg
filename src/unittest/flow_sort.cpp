#include "catch.hpp"
#include "vg.hpp"
#include "xg.hpp"
#include "vg.pb.h"

namespace vg {
namespace unittest {

using namespace std;

TEST_CASE("sorts input graph using max flow approach", "[sort_flow]") {
    SECTION("Sort simple graph") {
        const string graph_gfa = R"(H	VN:Z:1.0
S	1	G
L	1	+	2	+	0M
L	1	+	4	+	0M
S	2	T
L	2	+	3	+	0M
S	3	G
S	4	C
L	4	+	5	+	0M
S	5	C
L	5	+	2	+	0M
L	5	+	6	+	0M
S	6	T
L	6	+	3	+	0M
P	1	ref	1	+	1M
P	2	ref	2	+	1M
P	3	ref	3	+	1M
P	1	path1	1	+	1M
P	4	path1	2	+	1M
P	5	path1	3	+	1M
P	2	path1	4	+	1M
P	1	path2	1	+	1M
P	4	path2	2	+	1M
P	5	path2	3	+	1M
P	6	path2	4	+	1M
P	3	path2	5	+	1M)";
        
        VG vg;
        stringstream in(graph_gfa);
        vg.from_gfa(in);
        REQUIRE(vg.length() == 6);
        
        string reference_name = "ref";
        vg.max_flow(reference_name);
        
        stringstream res;
        for (int i =0; i < vg.graph.node_size(); ++i ) {
            if (i > 0)
               res << " ";
            res << vg.graph.mutable_node(i)->id();
        }
        // should be from manual calculation
        //1 4 5 2 6 3 
        REQUIRE(res.str().compare("1 2 4 5 6 3") == 0);
    }
}
}
}