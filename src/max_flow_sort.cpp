#include "vg.hpp"
#include "stream.hpp"
#include "gssw_aligner.hpp"
#include "vg.pb.h"
#include <raptor2/raptor2.h> 


#define debug 1
  

#ifdef debug
#include <jansson.h>
#endif


namespace vg {

using namespace std;
using namespace gfak;

#ifdef debug

static void visualize_graph(VG& vg, list<id_t>& reference) {
//        cerr << "######## Start debug test input any symbol\n";
    json_t* js_root = json_object();
    
    
    json_t* js_graph = json_object();
    json_t* js_nodes = json_array();
    json_t* js_edges = json_array();


    for (auto &node : vg.node_index) {
//            cerr << "node " << node.second << endl;
        json_t* js_node = json_object();

        std::string s_id = std::to_string(node.second);
        json_t* js_id = json_string(s_id.c_str());

        json_object_set(js_node, "id", js_id);
        json_object_set(js_node, "name", js_id);

        json_t* js_data = json_object();
        json_object_set(js_data, "data", js_node);
        json_array_append(js_nodes, js_data);
    }

    for (auto &edge : vg.edge_index)
    {
        json_t* js_edge = json_object();
        id_t from = edge.first->from();
        id_t to = edge.first->to();
        
        if(from == 55 || to == 55) {
            continue;
        }

        std::string s_id = std::to_string(from);
        std::string t_id = std::to_string(to);
        


        json_object_set(js_edge, "source", json_string(s_id.c_str()));
        json_object_set(js_edge, "target", json_string(t_id.c_str()));

        json_t* js_data = json_object();
        json_object_set(js_data, "data", js_edge);
        json_array_append(js_edges, js_data);
    }
    json_object_set(js_graph, "nodes", js_nodes);
    json_object_set(js_graph, "edges", js_edges);
    
    
    json_object_set(js_root, "graph", js_graph);
    
    json_t* js_ref = json_array();
    
    for(auto& r: reference) {
        string rs = std::to_string(r);
        json_array_append(js_ref, json_string(rs.c_str()));
    }
    
    json_object_set(js_root, "reference", js_ref);

    char* js_dump = NULL;
    js_dump = json_dumps(js_root, 0);

//        puts(s);

    ofstream data_js;
    data_js.open ("js/data.json");
    data_js << js_dump;
    data_js.close();
    json_decref(js_graph);
}

#endif

void VG::max_flow(const string& ref_name, bool isGrooming) {
    if (size() <= 1) return;
    // Topologically sort, which orders and orients all the nodes.
    
    list<NodeTraversal> sorted_nodes;
    paths.sort_by_mapping_rank();
    max_flow_sort(sorted_nodes, ref_name, isGrooming);
    list<NodeTraversal>::reverse_iterator n = sorted_nodes.rbegin();
    int i = 0;
//    cout << "result" << endl;
    for ( ; i < graph.node_size() && n != sorted_nodes.rend();
          ++i, ++n) {
        // Put the nodes in the order we got
        cout << (*n).node->id() << " ";
        swap_nodes(graph.mutable_node(i), (*n).node);
    }
//    rebuild_indexes();
    cout << endl;
}

void VG::fast_linear_sort(const string& ref_name, bool isGrooming)
{
    if (size() <= 1) return;
    // Topologically sort, which orders and orients all the nodes.
    list<NodeTraversal> sorted_nodes;
    paths.sort_by_mapping_rank();

    //get weighted graph
    WeightedGraph weighted_graph = get_weighted_graph(ref_name, isGrooming);

    //all nodes size
    id_t nodes_size =node_by_id.size();

    //create set of sinks and sources
    std::set<id_t> sources;
    //index - degree of nodes
    std::vector<std::set<id_t>> nodes_degree;
    int cur_node_degree;
    //find sources
    for (auto const &entry : node_by_id)
    {
        if(weighted_graph.edges_in_nodes.find(entry.first) == weighted_graph.edges_in_nodes.end() )
        {
            sources.insert(entry.first);
            continue;
        }
        cur_node_degree = get_node_degree(weighted_graph, entry.first);
        if (cur_node_degree < 0)
            continue;

        if (cur_node_degree + 1 > nodes_degree.size())
            nodes_degree.resize(cur_node_degree + 1);
        nodes_degree[cur_node_degree].insert(entry.first);
        //if(edges_out_nodes.find(entry.first) != edges_out_nodes.end() )
        //   sinks.insert(entry.first);
    }

    id_t next = -1;
    //std::vector<id_t> sorted;


    int remaining_nodes_size = nodes_size;
    set<id_t>::iterator i_last_el;

    while (remaining_nodes_size > 0)
    {   //Get next vertex to delete
        if (next == -1)
        {
            if (sources.size() > 0) //|| sinks.size() > 0)
            {
                i_last_el = sources.end();
                --i_last_el;
                next = *i_last_el;
                sources.erase(i_last_el);
            }
            else
                next = find_max_node(nodes_degree);
        }
        else
        {
            i_last_el = sources.find(next);
            if(i_last_el != sources.end())
                sources.erase(i_last_el);
        }

        //add next node
        //sorted.push_back(next);
        NodeTraversal node = NodeTraversal(node_by_id[next], false);
        sorted_nodes.push_back(node);

        //remove edges related with node
        next = get_next_node_recalc_degrees(weighted_graph, nodes_degree,sources,
                                            next);
        remaining_nodes_size--;
    }

    //output
    list<NodeTraversal>::iterator n = sorted_nodes.begin();
    int i = 0;
//    cout << "result" << endl;
    for ( ; i < graph.node_size() && n != sorted_nodes.end();
          ++i, ++n) {
        // Put the nodes in the order we got
        cout << (*n).node->id() << " ";
        swap_nodes(graph.mutable_node(i), (*n).node);
    }
//    rebuild_indexes();
    cout << endl;
}

void VG::max_flow_sort(list<NodeTraversal>& sorted_nodes, const string& ref_name, bool isGrooming) {

    //assign weight to edges
    //edge weight is determined as number of paths, that go through the edge
    WeightedGraph weighted_graph = get_weighted_graph(ref_name, isGrooming);
    list<Mapping> ref_path(paths.get_path(ref_name).begin(),
            paths.get_path(ref_name).end());
    ref_path.reverse();
    set<id_t> backbone;
    list<id_t> reference;

    for(auto const &mapping : ref_path) {
        backbone.insert(mapping.position().node_id());
        reference.push_back(mapping.position().node_id());
    }
    set<id_t> nodes;
    for (auto const &entry : node_by_id) {
        nodes.insert(entry.first);
    }
    
        
    #ifdef debug
    {
        visualize_graph(*this, reference);
//        for(auto& p : reference) {
//            cerr << p << endl;
//        }
    }
    #endif

    set<id_t> unsorted_nodes(nodes.begin(), nodes.end());
    InOutGrowth growth = InOutGrowth(nodes, backbone, reference);
    find_in_out_web(sorted_nodes, growth, weighted_graph, unsorted_nodes);
    

   
    while (sorted_nodes.size() != graph.node_size()) {
        cerr << "additional sorting for missing nodes" << endl;
//        cerr << "unsorted: " << unsorted_nodes.size() << endl;
        cerr << "sorted: " << sorted_nodes.size() << " in graph: " <<
                graph.node_size() << endl;
        list<NodeTraversal> sorted_nodes_new;
    
        set<id_t> unsorted_nodes_new(growth.nodes.begin(), growth.nodes.end());
        set<id_t> nodes_new(growth.nodes.begin(), growth.nodes.end());
 
        set<id_t> backbone_new;
        list<id_t> reference_new;
        for (auto const &node : sorted_nodes) {
            backbone_new.insert(node.node->id());
            reference_new.push_back(node.node->id());
        }
      
        InOutGrowth growth_new = InOutGrowth(nodes_new, backbone_new, reference_new);
        WeightedGraph weighted_graph_new = get_weighted_graph(ref_name);
        find_in_out_web(sorted_nodes_new, growth_new, weighted_graph_new, unsorted_nodes_new);

        sorted_nodes = sorted_nodes_new;
        if (unsorted_nodes.size() == unsorted_nodes_new.size()) {
            cerr << "Failed to insert missing nodes"<< endl;
            return;
        }
        unsorted_nodes = unsorted_nodes_new;
    }
}

/* return degree of the node in the WeightedGraph*/
int VG::get_node_degree(VG::WeightedGraph& wg, id_t node_id)
{

    std::vector<Edge*> in_edges = wg.edges_in_nodes[node_id];
    int degree = 0;
    for(auto const &edge : in_edges)
        degree -= wg.edge_weight[edge];

    std::vector<Edge*> out_edges = wg.edges_out_nodes[node_id];
    for(auto const &edge : out_edges)
        degree += wg.edge_weight[edge];

    return degree;
}

/* Weight is assigned to edges as number of paths, that go through hat edge.
   Path goes through the edge, if both adjacent nodes of the edge are mapped to that path.*/

VG::WeightedGraph VG::get_weighted_graph(const string& ref_name, bool isGrooming)
{
    EdgeMapping edges_out_nodes;
    EdgeMapping edges_in_nodes;
    map<Edge*, int> edge_weight;
    int ref_weight = paths._paths.size();
    if (ref_weight < 5) {
        ref_weight = 5;
    }

    this->flip_doubly_reversed_edges();
    //"bad" edges
    map<id_t, set<Edge*>> minus_start;//vertex - edges
    map<id_t, set<Edge*>> minus_end;//vertex - edges
    set<id_t> nodes;
    id_t start_ref_node = 0;
    for (auto &edge : edge_index)
    {
        id_t from = edge.first->from();
        id_t to = edge.first->to();
        if (edge.first->from_start() || edge.first->to_end())
        {
                minus_start[from].insert(edge.first);
                minus_end[to].insert(edge.first);
        }
        else
        {
            edges_out_nodes[edge.first->from()].push_back(edge.first);
            edges_in_nodes[edge.first->to()].push_back(edge.first);
        }
        //Node* nodeTo = node_by_id[edge.first->to()];
        nodes.insert(edge.first->from());
        nodes.insert(edge.first->to());

        //assign weight to the minimum number of paths of the adjacent nodes
        NodeMapping from_node_mapping = paths.get_node_mapping(from);
//        NodeMapping to_node_mapping = paths.get_node_mapping(to);
        int weight = 1;

        for (auto const &path_mapping : from_node_mapping) {
            string path_name = path_mapping.first;
            if (paths.are_consecutive_nodes_in_path(from, to, path_name))
            {
                if (path_name == ref_name)
                {
                    weight += ref_weight;
                    if(start_ref_node == 0)
                        start_ref_node = edge.first->from();
                }
                weight++;
            }
        }

        edge_weight[edge.first] = weight;
//        cerr << from << "->" << to << " " << weight << endl;
    }
    if (isGrooming)
    {
        // get connected components
        vector<set<id_t>> ccs = get_cc_in_wg(edges_in_nodes, edges_out_nodes, nodes, start_ref_node);
        // grooming
        id_t main_cc = 0;
        if(ccs.size() > 1)
        {
            for(id_t j = 0; j < ccs.size(); j++)
            {
                if(j != main_cc)
                    groom_components(edges_in_nodes, edges_out_nodes, ccs[j], ccs[main_cc], minus_start, minus_end);

            }
        }
        this->rebuild_edge_indexes();
    }
    return WeightedGraph(edges_out_nodes, edges_in_nodes, edge_weight);
}


void VG::groom_components(EdgeMapping& edges_in, EdgeMapping& edges_out, set<id_t>& isolated_nodes, set<id_t>& main_nodes,
                          map<id_t, set<Edge*>> &minus_start, map<id_t, set<Edge*>> &minus_end)
{
    vector<Edge*> from_minus_edges;
    vector<Edge*> to_minus_edges;
    auto nodes_it = isolated_nodes.begin();
    //find all "bad" edeges
    while (nodes_it != isolated_nodes.end())
    {
        if(minus_start.find(*nodes_it) != minus_start.end())
        {
            for(auto& e: minus_start[*nodes_it])
            {
                //find edge between main component and isolated component
                if(main_nodes.find(e->from()) != main_nodes.end() || main_nodes.find(e->to()) != main_nodes.end() )
                {
                    if(e->from_start())
                        from_minus_edges.push_back(e);
                    if(e->to_end())
                        to_minus_edges.push_back(e);
                    //break;
                }
            }
        }
        id_t lol;
        if(minus_end.find(*nodes_it) != minus_end.end())
        {
            lol = *nodes_it;
            for(auto& e: minus_end[*nodes_it])
            {
                //find edge between main component and isolated component
                if(main_nodes.find(e->from()) != main_nodes.end() || main_nodes.find(e->to()) != main_nodes.end() )
                {
                    if(e->from_start())
                        from_minus_edges.push_back(e);
                    if(e->to_end())
                        to_minus_edges.push_back(e);
                    //break;
                }
            }
        }
        nodes_it++;
    }
    //reverse "bad" edges
    for(auto& e: from_minus_edges)
    {
        if(main_nodes.find(e->from()) != main_nodes.end())
        {
            from_simple_reverse_orientation(e);
            update_in_out_edges(edges_in,edges_out, e);
            continue;
        }
        if(main_nodes.find(e->to()) != main_nodes.end())
        {
            from_simple_reverse(e);
            update_in_out_edges(edges_in,edges_out, e);
        }
    }
    for(auto& e: to_minus_edges)
    {
        if(main_nodes.find(e->to()) != main_nodes.end())
        {
            to_simple_reverse_orientation(e);
            update_in_out_edges(edges_in,edges_out, e);
            continue;
        }
        if(main_nodes.find(e->from()) != main_nodes.end())
        {
            to_simple_reverse(e);
            update_in_out_edges(edges_in,edges_out, e);
        }
    }

    nodes_it = isolated_nodes.begin();
    Edge* internal_edge;
    vector<Edge*> edges_to_flip;
    //reverse all internal edeges
    while (nodes_it != isolated_nodes.end())
    {
        for(auto& e:edges_in[*nodes_it])
        {

            // if edge is internal
            if(isolated_nodes.find(e->from()) != isolated_nodes.end())
            {
                internal_edge = e;
                edges_to_flip.push_back(e);
            }
        }
        nodes_it++;
    }
    for(auto& e: edges_to_flip)
    {
        internal_edge = e;
        erase_in_out_edges(edges_in, edges_out, internal_edge);
        reverse_edge(internal_edge);
        update_in_out_edges(edges_in,edges_out, internal_edge);
    }
    //insert isolated set to main set
    main_nodes.insert(isolated_nodes.begin(), isolated_nodes.end());
}

void VG::update_in_out_edges(EdgeMapping& edges_in, EdgeMapping& edges_out, Edge* e)
{
    edges_in[e->to()].push_back(e);
    edges_out[e->from()].push_back(e);
}

void VG::erase_in_out_edges(EdgeMapping& edges_in, EdgeMapping& edges_out, Edge* e)
{
    int i = 0;
    while(*(edges_in[e->to()].begin() + i) != e)
        i++;
    edges_in[e->to()].erase(edges_in[e->to()].begin() + i);
    i = 0;
    while(*(edges_out[e->from()].begin() + i) != e)
        i++;
    edges_out[e->from()].erase(edges_out[e->from()].begin() + i);
}

void VG::reverse_from_start_to_end_edge(Edge* &e)
{
    e->set_from_start(false);
    e->set_to_end(false);
    reverse_edge(e);
}


void VG::reverse_edge(Edge* &e)
{
    id_t tmp_vrtx = e->to();
    e->set_to(e->from());
    e->set_from(tmp_vrtx);
}


// a(from_start ==true) -> b        =>        not a (from_start == false)  -> b
id_t VG::from_simple_reverse(Edge* &e)
{
    e->set_from_start(false);
    return e->from();
}

// b(from_start ==true) -> a        =>        not a (from_start == false)  -> b
id_t VG::from_simple_reverse_orientation(Edge* &e)
{
    e->set_from_start(false);
    reverse_edge(e);
    return e->from();
}

// a -> b (to_end ==true)       =>        a -> not b(to_end ==false)
id_t VG::to_simple_reverse(Edge* &e)
{
    e->set_to_end(false);
    return e->to();
}

// b -> a (to_end ==true)       =>        a -> not b(to_end ==false)
id_t VG::to_simple_reverse_orientation(Edge* &e)
{
    reverse_edge(e);
    e->set_to_end(false);
    return e->to();
}

vector<set<id_t>> VG::get_cc_in_wg(EdgeMapping& edges_in,EdgeMapping& edges_out,
                                   const set<id_t>& all_nodes, id_t start_ref_node)
{
    set<id_t> nodes(all_nodes.begin(), all_nodes.end());
    vector<set<id_t>> result;
    bool main_cc = true;
    id_t s = start_ref_node;
    nodes.erase(nodes.find(s));
    while(nodes.size() > 0)
    {
        set<id_t> visited;
        if(!main_cc)
        {
            s = *nodes.begin();
            nodes.erase(nodes.begin());
        }
        main_cc = false;
        std::stack<id_t> q({ s });
        while (!q.empty())
        {
            s = q.top();
            q.pop();
            if (visited.find(s) != visited.end())
                continue;
            nodes.erase(s);
            visited.insert(s);
            for(const auto& e: edges_in[s])
            {
                id_t from = e->from();
                if (visited.find(from ) == visited.end())
                    q.push(from);
            }
            for(const auto& e: edges_out[s])
            {
                id_t to = e->to();
                if (visited.find(to) == visited.end())
                    q.push(to);
            }
        }
        result.push_back(visited);
    }
    return result;
}

/* Iterate all edges adjacent to node, recalc degrees of related nodes.
 * If node has no incoming edges we add it to the sources and return as next node.
 * */
id_t VG::get_next_node_recalc_degrees(WeightedGraph& wg, std::vector<std::set<id_t>>& degrees,std::set<id_t> &sources,
                                     id_t node)
{
    id_t result = -1;
    //recalc degrees
    id_t cur_node;
    int new_degree;
    int cur_degree;
    //remove from degrees (if node is not source)
    int node_degree = get_node_degree(wg, node);
    if(node_degree >= 0 && (node_degree < degrees.size()))
    {
        set<id_t>::iterator i_el = degrees[node_degree].find(node);
        if(i_el != degrees[node_degree].end())
            degrees[node_degree].erase(i_el);
    }


    //decrease number of in_edges from current node
    for(const auto& edge: wg.edges_in_nodes[node])
    {
        cur_node = edge->from();
        cur_degree = get_node_degree(wg, cur_node);
        new_degree = cur_degree - wg.edge_weight[edge];

        //decrease out_edges from current node
        if(cur_degree >= 0)
            degrees[cur_degree].erase(cur_node);
        //add node to new_degree set
        if(new_degree >= 0)
        {
            if (new_degree + 1> degrees.size())
                degrees.resize(new_degree + 1);
            degrees[new_degree].insert(cur_node);
        }


        //remove current edge from wg
        std::vector<Edge*>& related_edges = wg.edges_out_nodes[cur_node];
        related_edges.erase(std::remove(related_edges.begin(), related_edges.end(), edge), related_edges.end());
    }
    for(const auto& edge: wg.edges_out_nodes[node])
    {
        cur_node = edge->to();
        //check if node is new source
        std::vector<Edge*>& related_edges = wg.edges_in_nodes[cur_node];
        //add related node to sources
        if(related_edges.size() == 1)
        {
            sources.insert(cur_node);
            result = cur_node;
        }

        cur_degree = get_node_degree(wg, cur_node);
        new_degree = cur_degree + wg.edge_weight[edge];

        //decrease in_edges from current node
        if(cur_degree >= 0)
            degrees[cur_degree].erase(cur_node);
        //add node to new_degree set (if not source)
        if(new_degree >= 0 && related_edges.size() != 1)
        {
            if (new_degree + 1> degrees.size())
                degrees.resize(new_degree + 1);
            degrees[new_degree].insert(cur_node);
        }
        //remove current edge from wg
        related_edges.erase(std::remove(related_edges.begin(), related_edges.end(), edge), related_edges.end());
    }

    return result;
}

/*Find node with max degree*/
id_t VG::find_max_node(std::vector<std::set<id_t>> nodes_degree)
{
    int start_size = nodes_degree.size()-1;
    int last_ind = start_size;
    while(nodes_degree[last_ind].size() == 0)
    {
        last_ind--;
    }
    if(last_ind != start_size)
        nodes_degree.resize(last_ind + 1);
    set<id_t>::iterator i_last_el = nodes_degree[last_ind].end();
    return *(--i_last_el);
}

/*  Method finds min cut in a given set of nodes, then removes min cut edges,
    finds in- and -out growth from the reference path and calls itself on them recursively.
*/
void VG::find_in_out_web(list<NodeTraversal>& sorted_nodes,
                            InOutGrowth& in_out_growth,
                            WeightedGraph& weighted_graph,
                            set<id_t>& unsorted_nodes) {

    set<id_t> backbone = in_out_growth.backbone;
    set<id_t> nodes = in_out_growth.nodes;
    list<id_t> ref_path = in_out_growth.ref_path;

    //for efficiency if size of the backbone == size of the nodes
    //we just add all backbone to sorted nodes and quit
    if (nodes.size() == backbone.size()) {
        for (auto const &id: ref_path) {
            if (!unsorted_nodes.count(id)) {
                continue;
            }
            NodeTraversal node = NodeTraversal(node_by_id[id], false);
            sorted_nodes.push_back(node);
            unsorted_nodes.erase(id);
        }
        return;
    }

    EdgeMapping& edges_out_nodes = weighted_graph.edges_out_nodes;
    EdgeMapping& edges_in_nodes = weighted_graph.edges_in_nodes;
    map<Edge*, int>& edge_weight = weighted_graph.edge_weight;

    //add source and sink nodes
    id_t source = graph.node_size() + 1;
    id_t sink = graph.node_size() + 2;
    id_t graph_size = sink + 1;

    set<Edge*> out_joins;
    set<Edge*> in_joins;

    map<id_t, map<id_t, int>> weighted_sub_graph;

    //determine max outgoing weight
    int max_weight = 0;
    for(auto const &node : nodes) {
        int current_weight = 0;
        for (auto const &edge : edges_out_nodes[node]) {
            if (!nodes.count(edge->to())) {
                continue;
            }
            current_weight += edge_weight[edge];
            if (backbone.count(node) && !backbone.count(edge->to())) {
                out_joins.insert(edge);
            }

            if (!backbone.count(node) && backbone.count(edge->to())) {
                in_joins.insert(edge);
                continue;
            }
            weighted_sub_graph[edge->from()][edge->to()] = edge_weight[edge];
        }

        if (current_weight > max_weight) {
            max_weight = current_weight;
        }
    }
    //set weight for source edges
    for (auto const &edge : out_joins) {
        weighted_sub_graph[source][edge->from()] = max_weight + 1;

    }
    //set weight for sink edges
    for (auto const &edge : in_joins) {
        weighted_sub_graph[edge->from()][sink] = edge_weight[edge];
    }

    //find min-cut edges
    vector<pair<id_t,id_t>> cut = min_cut(weighted_sub_graph, nodes, source,
                                                    sink, edges_out_nodes, in_joins);
    //remove min cut edges
    for(auto const &edge : cut) {
        id_t to = edge.second;
        if (to == sink) {
            for (auto const &in_join : in_joins) {
                if (in_join->from() == edge.first) {
                    to = in_join->to();
                }
            }
        }

        remove_edge(edges_out_nodes, edge.first, to, false);
        remove_edge(edges_in_nodes, to, edge.first, true);
    }

    set<id_t> visited;
    visited.insert(backbone.begin(), backbone.end());
    for (auto &current_id: ref_path) {
        NodeTraversal node = NodeTraversal(node_by_id[current_id], false);
        //out growth
        process_in_out_growth(edges_out_nodes, current_id, in_out_growth,
                    weighted_graph, visited, sorted_nodes, false, unsorted_nodes);
        //add backbone node to the result
        if (unsorted_nodes.count(current_id)) {
            sorted_nodes.push_back(node);
            unsorted_nodes.erase(current_id);
        }
        //in growth
        process_in_out_growth(edges_in_nodes, current_id, in_out_growth,
                    weighted_graph, visited, sorted_nodes, true, unsorted_nodes);
    }
}
/*
        Determines the presence of a in- out- growth, finds its backbone and calls min cut algorithm.
     */
void VG::process_in_out_growth(EdgeMapping& nodes_to_edges, id_t current_id,
        InOutGrowth& in_out_growth,
        WeightedGraph& weighted_graph,
        set<id_t>& visited,
        list<NodeTraversal>& sorted_nodes,
        bool reverse,
        set<id_t>& unsorted_nodes) {

    if (!nodes_to_edges.count(current_id) || nodes_to_edges[current_id].size() == 0)
        return;
    set<id_t> backbone = in_out_growth.backbone;
    set<id_t> nodes = in_out_growth.nodes;
    set<id_t> new_backbone;
    list<id_t> new_ref_path;
    id_t start_node = current_id;
    
    while (true) {
        if (new_backbone.count(start_node) || (start_node != current_id && 
                visited.count(start_node))) {
            break;
        }
        new_backbone.insert(start_node);
        new_ref_path.push_back(start_node);
        int weight = 0;
        Edge* next_edge;
        //take edges with maximum weight to the new reference path
        for (auto const &out_edge : nodes_to_edges[start_node]) {
            if (weighted_graph.edge_weight[out_edge] > weight) {
                id_t tmp = reverse ? out_edge->from() : out_edge->to();
                if (!nodes.count(tmp) || backbone.count(tmp) ||
                        new_backbone.count(tmp) || visited.count(tmp)) {
                    continue;
                }
                start_node = tmp;
                weight = weighted_graph.edge_weight[out_edge];
                next_edge = out_edge;

            }
        }
    }

    set<id_t> web;
    mark_dfs(nodes_to_edges, current_id, web, visited, reverse, nodes, backbone);
    if (web.size() == 1 && web.count(current_id)) {
        return;
    }
    if (!reverse)
        new_ref_path.reverse();
    InOutGrowth growth = InOutGrowth(web, new_backbone, new_ref_path);
    find_in_out_web(sorted_nodes, growth, weighted_graph, unsorted_nodes);
    
}

void VG::remove_edge(EdgeMapping& nodes_to_edges, id_t node, id_t to, bool reverse) {
    if (nodes_to_edges.count(node)) {
        auto& edges = nodes_to_edges[node];
        auto it = begin(edges);
        while (it != end(edges)) {
            id_t next = reverse ? (*it)->from() : (*it)->to();

            if (next == to) {
                edges.erase(it);
//                cerr << "removing edge " << reverse << " " << node << " " << to << endl;
            } else {
                ++it;
            }
        }
    }
}

/*
    Adds the nodes in dfs discovery order to sorted_nodes
 */
void VG::mark_dfs(EdgeMapping& edges_to_nodes, id_t s,
            set<id_t>& new_nodes, set<id_t>& visited, bool reverse,
        set<id_t>& nodes,
        set<id_t>& backbone) {
    visited.insert(s);
    new_nodes.insert(s);
    std::stack<id_t> q({ s });
    while(!q.empty()) {
        s = q.top();
        q.pop();
        if (edges_to_nodes.count(s)) {
            for (auto const &edge: edges_to_nodes[s]) {
                id_t next_node = reverse ? edge->from() : edge->to();
                if (!visited.count(next_node) && nodes.count(next_node)
                        && !backbone.count(next_node)) {
                    visited.insert(next_node);
                    new_nodes.insert(next_node);
                    q.push(next_node);
                }
            }
        }
    }

}

/* Returns true if there is a path from source 's' to sink 't' in
   residual graph. Also fills parent[] to store the path */
bool VG::bfs(set<id_t>& nodes, map<id_t, map<id_t, int>>& edge_weight, id_t s,
            id_t t, map<id_t, id_t>& parent) {
    // Create a visited array and mark all vertices as not visited
    set<id_t> visited;

    // Create a queue, enqueue source vertex and mark source vertex
    // as visited
    std::queue<id_t> q({ s });
    visited.insert(s);
    parent[s] = -1;

    // Standard BFS Loop
    while (!q.empty()) {
        id_t u = q.front();
        q.pop();
        for (auto const &weight : edge_weight[u]) {
            if (weight.second <= 0) {
                continue;
            }
            id_t v = weight.first;
            if (!visited.count(v)) {
                q.push(v);
                parent[v] = u;
                visited.insert(v);
                  if (v == t) {
                    return true;
                }
            }
        }
    }
    // If we reached sink in BFS starting from source, then return
    // true, else false
    return (visited.count(t) != 0);

}


void VG::dfs(set<id_t>& nodes, id_t s, set<id_t>& visited, map<id_t, map<id_t, int>>& edge_weight) {

    visited.insert(s);
    std::stack<id_t> q({ s });
    while (!q.empty()) {
        s = q.top();
        q.pop();
        for (auto const &weight : edge_weight[s]) {
            id_t to = weight.first;
            if (weight.second && !visited.count(to)) {
                visited.insert(to);
                q.push(to);
            }
        }
    }
}


// Returns the minimum s-t cut

vector<pair<id_t,id_t>> VG::min_cut(map<id_t, map<id_t, int>>& graph_weight,
                set<id_t>& nodes,
                id_t s, id_t t,
                EdgeMapping& edges_out_nodes,
                set<Edge*>& in_joins) {
    id_t u, v;
    nodes.insert(s);
    nodes.insert(t);
    map<id_t, id_t> parent_map;

    // Augment the flow while there is path from source to sink
    while (bfs(nodes, graph_weight, s, t, parent_map)) {
        // Find minimum residual capacity of the edges along the
        // path filled by BFS. Or we can say find the maximum flow
        // through the path found.
        int path_flow = numeric_limits<int>::max();
        for (v = t; v != s; v = parent_map[v]) {
            u = parent_map[v];
            path_flow = min(path_flow, graph_weight[u][v]);
        }
        // update residual capacities of the edges and reverse edges
        // along the path
        for (v = t; v != s; v = parent_map[v]) {
            u = parent_map[v];
            graph_weight[u][v] -= path_flow;
            graph_weight[v][u] += path_flow;
        }
    }

    // Flow is maximum now, find vertices reachable from s

    set<id_t> visited_set;
    dfs(nodes, s, visited_set, graph_weight);
    vector<pair<id_t,id_t>> min_cut;

    nodes.erase(s);
    nodes.erase(t);

    for(auto const node_id : nodes) {
        for (auto const &edge : edges_out_nodes[node_id]) {
            id_t to = edge->to();
            if (!nodes.count(to)) {
                continue;
            }

            if (in_joins.count(edge)) {
                 to = t;
            }
            if (visited_set.count(node_id) && !visited_set.count(to)) {
//                cerr << node_id << " - " << to << endl;
                //remove min cut edges
                min_cut.push_back(pair<id_t, id_t>(node_id, to));
            }
        }
    }

    return min_cut;
}
}