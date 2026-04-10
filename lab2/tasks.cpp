#include <unordered_map>

#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>

#include "Labs/2-GeometryProcessing/DCEL.hpp"
#include "Labs/2-GeometryProcessing/tasks.h"

namespace VCX::Labs::GeometryProcessing {

#include "Labs/2-GeometryProcessing/marching_cubes_table.h"

    /******************* 1. Mesh Subdivision *****************/
    void SubdivisionMesh(Engine::SurfaceMesh const & input, Engine::SurfaceMesh & output, std::uint32_t numIterations) {
        Engine::SurfaceMesh curr_mesh = input;
        // We do subdivison iteratively.
        for (std::uint32_t it = 0; it < numIterations; ++it) {
            // During each iteration, we first move curr_mesh into prev_mesh.
            Engine::SurfaceMesh prev_mesh;
            prev_mesh.Swap(curr_mesh);
            // Then we create doubly connected edge list.
            DCEL G(prev_mesh);
            if (! G.IsManifold()) {
                spdlog::warn("VCX::Labs::GeometryProcessing::SubdivisionMesh(..): Non-manifold mesh.");
                return;
            }
            // Note that here curr_mesh has already been empty.
            // We reserve memory first for efficiency.
            curr_mesh.Positions.reserve(prev_mesh.Positions.size() * 3 / 2);
            curr_mesh.Indices.reserve(prev_mesh.Indices.size() * 4);
            // Then we iteratively update currently existing vertices.
            for (std::size_t i = 0; i < prev_mesh.Positions.size(); ++i) {
                // Update the currently existing vetex v from prev_mesh.Positions.
                // Then add the updated vertex into curr_mesh.Positions.
                auto v           = G.Vertex(i);
                auto neighbors   = v->Neighbors();
                // your code here:
                int v_num = neighbors.size();
                float u = (v_num == 3) ? 3.0f / 16.0f : 3.0f / (8.0f * v_num);
                glm::vec3 newpos((1.0f - u * v_num) * prev_mesh.Positions[i]);
                for(auto neighbor : neighbors){
                    newpos += u * prev_mesh.Positions[neighbor];
                }
                curr_mesh.Positions.push_back(newpos);
            }
            // We create an array to store indices of the newly generated vertices.
            // Note: newIndices[i][j] is the index of vertex generated on the "opposite edge" of j-th
            //       vertex in the i-th triangle.
            std::vector<std::array<std::uint32_t, 3U>> newIndices(prev_mesh.Indices.size() / 3, { ~0U, ~0U, ~0U });
            // Iteratively process each halfedge.
            for (auto e : G.Edges()) {
                // newIndices[face index][vertex index] = index of the newly generated vertex
                newIndices[G.IndexOf(e->Face())][e->EdgeLabel()] = curr_mesh.Positions.size();
                auto eTwin                                       = e->TwinEdgeOr(nullptr);
                // eTwin stores the twin halfedge.
                if (! eTwin) {
                    // When there is no twin halfedge (so, e is a boundary edge):
                    // your code here: generate the new vertex and add it into curr_mesh.Positions.
                    glm::vec3 fromv = prev_mesh.Positions[e->From()];
                    glm::vec3 tov   = prev_mesh.Positions[e->To()];
                    glm::vec3 newv = 0.5f * (fromv + tov);
                    curr_mesh.Positions.push_back(newv);
                } else {
                    // When the twin halfedge exists, we should also record:
                    //     newIndices[face index][vertex index] = index of the newly generated vertex
                    // Because G.Edges() will only traverse once for two halfedges,
                    //     we have to record twice.
                    newIndices[G.IndexOf(eTwin->Face())][e->TwinEdge()->EdgeLabel()] = curr_mesh.Positions.size();
                    // your code here: generate the new vertex and add it into curr_mesh.Positions.
                    glm::vec3 A = prev_mesh.Positions[e->From()];
                    glm::vec3 B = prev_mesh.Positions[e->To()];
                    glm::vec3 C = prev_mesh.Positions[e->NextEdge()->To()];
                    glm::vec3 D = prev_mesh.Positions[eTwin->NextEdge()->To()];

                    glm::vec3 newv = (3.0f / 8.0f) * (A + B) + (1.0f / 8.0f) * (C + D);
                    curr_mesh.Positions.push_back(newv);
                }
            }

            // Here we've already build all the vertices.
            // Next, it's time to reconstruct face indices.
            for (std::size_t i = 0; i < prev_mesh.Indices.size(); i += 3U) {
                // For each face F in prev_mesh, we should create 4 sub-faces.
                // v0,v1,v2 are indices of vertices in F.
                // m0,m1,m2 are generated vertices on the edges of F.
                auto v0           = prev_mesh.Indices[i + 0U];
                auto v1           = prev_mesh.Indices[i + 1U];
                auto v2           = prev_mesh.Indices[i + 2U];
                auto [m0, m1, m2] = newIndices[i / 3U];
                // Note: m0 is on the opposite edge (v1-v2) to v0.
                // Please keep the correct indices order (consistent with order v0-v1-v2)
                //     when inserting new face indices.
                // toInsert[i][j] stores the j-th vertex index of the i-th sub-face.

                //       v0
                //      /  \
                //     m2   m1
                //    /      \
                //   v1--m0---v2
                std::uint32_t toInsert[4][3] = {
                    // your code here:
                    {v0, m2, m1},
                    {v1, m0, m2},
                    {v2, m1, m0},
                    {m0, m1, m2}
                };
                // Do insertion.
                curr_mesh.Indices.insert(
                    curr_mesh.Indices.end(),
                    reinterpret_cast<std::uint32_t *>(toInsert),
                    reinterpret_cast<std::uint32_t *>(toInsert) + 12U
                );
            }

            if (curr_mesh.Positions.size() == 0) {
                spdlog::warn("VCX::Labs::GeometryProcessing::SubdivisionMesh(..): Empty mesh.");
                output = input;
                return;
            }
        }
        // Update output.
        output.Swap(curr_mesh);
    }

    /******************* 2. Mesh Parameterization *****************/
    void Parameterization(Engine::SurfaceMesh const & input, Engine::SurfaceMesh & output, const std::uint32_t numIterations) {
        // Copy.
        output = input;
        // Reset output.TexCoords.
        output.TexCoords.resize(input.Positions.size(), glm::vec2 { 0 });

        // Build DCEL.
        DCEL G(input);
        if (! G.IsManifold()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::Parameterization(..): non-manifold mesh.");
            return;
        }
    
        // Set boundary UVs for boundary vertices.
        // your code here: directly edit output.TexCoords
        std::vector<std::uint32_t> boundaryVertices;
        std::vector<std::uint32_t> interiorVertices;
    
        for(std::uint32_t i = 0 ; i < input.Positions.size() ; i++){
            auto v = G.Vertex(i);
            if(v && v->OnBoundary()){
                boundaryVertices.push_back(i);
            }
            else{
                interiorVertices.push_back(i);
            }
        }

        std::vector<int> boundary_indice;
        int bounday_cnt = 1;
        if (!boundaryVertices.empty()) {
            int start = boundaryVertices[0];
            auto startv = G.Vertex(start);
            boundary_indice.push_back(start);

            int next = startv->BoundaryNeighbors().first;
            int prev = start;
            while(next != start){
                bounday_cnt++;
                boundary_indice.push_back(next);
                
                auto nextv = G.Vertex(next);
                if(nextv->BoundaryNeighbors().first != prev){
                    prev = next;
                    next = nextv->BoundaryNeighbors().first;
                }
                else{
                    prev = next;
                    next = nextv->BoundaryNeighbors().second;
                }
            }

            // 已经连接成边界环
            float pi = std::acos(-1.0);
            float step = 2 * pi / bounday_cnt;
            for(size_t i = 0 ; i < bounday_cnt ; i++){
                size_t boundary_index = boundary_indice[i];
                auto v = G.Vertex(boundary_index);
                if(v->OnBoundary()){
                    float x = 0.5f + 0.5f * std::cos(step * i);
                    float y = 0.5f + 0.5f * std::sin(step * i);
                    output.TexCoords[boundary_index] = glm::vec2{x, y};
                }
            }


        }
        
        // Solve equation via Gauss-Seidel Iterative Method.
        for (int k = 0; k < numIterations; ++k) {
            // your code here:
            for(size_t i = 0 ; i < input.Positions.size() ; i++){
                auto v = G.Vertex(i);
                if(!v->OnBoundary()){
                    auto neighbors = v->Neighbors();
                    if(neighbors.empty()) continue;
                    glm::vec2 sum = {0.0f, 0.0f};
                
                    for(auto neighbor : neighbors){
                        sum += output.TexCoords[neighbor];
                    }

                output.TexCoords[i] = sum / (1.0f * neighbors.size());
                }
            }
        }
    }

    /******************* 3. Mesh Simplification *****************/
    void SimplifyMesh(Engine::SurfaceMesh const & input, Engine::SurfaceMesh & output, float simplification_ratio) {

        DCEL G(input);
        if (! G.IsManifold()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::SimplifyMesh(..): Non-manifold mesh.");
            return;
        }
        // We only allow watertight mesh.
        if (! G.IsWatertight()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::SimplifyMesh(..): Non-watertight mesh.");
            return;
        }

        // Copy.
        output = input;

        // Compute Kp matrix of the face f.
        auto UpdateQ {
            [&G, &output] (DCEL::Triangle const * f) -> glm::mat4 {
                glm::mat4 Kp;
                // your code here:
                int v0_i = f->VertexIndex(0);
                int v1_i = f->VertexIndex(1);
                int v2_i = f->VertexIndex(2);
                auto v0 = output.Positions[v0_i];
                auto v1 = output.Positions[v1_i];
                auto v2 = output.Positions[v2_i];

                glm::vec3 nor = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                double d = -glm::dot(nor, v0);

                glm::vec4 p{nor.x, nor.y, nor.z, d};
                for(int i = 0 ; i < 4 ; i++){
                    for(int j = 0 ; j < 4 ; j++){
                        Kp[i][j] = p[i] * p[j];
                    }
                }

                return Kp;
            }
        };

        // The struct to record contraction info.
        struct ContractionPair {
            DCEL::HalfEdge const * edge;            // which edge to contract; if $edge == nullptr$, it means this pair is no longer valid
            glm::vec4              targetPosition;  // the targetPosition $v$ for vertex $edge->From()$ to move to
            float                  cost;            // the cost $v.T * Qbar * v$
        };

        // Given an edge (v1->v2), the positions of its two endpoints (p1, p2) and the Q matrix (Q1+Q2),
        //     return the ContractionPair struct.
        static constexpr auto MakePair {
            [] (DCEL::HalfEdge const * edge,
                glm::vec3 const & p1,
                glm::vec3 const & p2,
                glm::mat4 const & Q
            ) -> ContractionPair {
                // your code here:

                glm::mat3 A = {Q};
                glm::vec3 b = {Q[0][3], Q[1][3], Q[2][3]};
                glm::vec3 target;

                if(glm::abs(glm::determinant(A)) < 0.001f){
                    target = (p1 + p2) * 0.5f;
                }
                else{
                    target = - glm::inverse(A) * b;
                }

                glm::vec4 newtarget = {target, 1};
                float cost = glm::dot(newtarget, Q * newtarget);

                return {edge, newtarget, cost};
            }
        };

        // pair_map: map EdgeIdx to index of $pairs$
        // pairs:    store ContractionPair
        // Qv:       $Qv[idx]$ is the Q matrix of vertex with index $idx$
        // Kf:       $Kf[idx]$ is the Kp matrix of face with index $idx$
        std::unordered_map<DCEL::EdgeIdx, std::size_t> pair_map; 
        std::vector<ContractionPair>                   pairs; 
        std::vector<glm::mat4>                         Qv(G.NumOfVertices(), glm::mat4(0));
        std::vector<glm::mat4>                         Kf(G.NumOfFaces(),    glm::mat4(0));

        // Initially, we compute Q matrix for each faces and it accumulates at each vertex.
        for (auto f : G.Faces()) {
            auto Q                 = UpdateQ(f);
            Qv[f->VertexIndex(0)] += Q;
            Qv[f->VertexIndex(1)] += Q;
            Qv[f->VertexIndex(2)] += Q;
            Kf[G.IndexOf(f)]       = Q;
        }

        pair_map.reserve(G.NumOfFaces() * 3);
        pairs.reserve(G.NumOfFaces() * 3 / 2);

        // Initially, we make pairs from all the contractable edges.
        for (auto e : G.Edges()) {
            if (! G.IsContractable(e)) continue;
            auto v1                            = e->From();
            auto v2                            = e->To();
            auto pair                          = MakePair(e, input.Positions[v1], input.Positions[v2], Qv[v1] + Qv[v2]);
            pair_map[G.IndexOf(e)]             = pairs.size();
            pair_map[G.IndexOf(e->TwinEdge())] = pairs.size();
            pairs.emplace_back(pair);
        }

        // Loop until the number of vertices is less than $simplification_ratio * initial_size$.
        while (G.NumOfVertices() > simplification_ratio * Qv.size()) {
            // Find the contractable pair with minimal cost.
            std::size_t min_idx = ~0;
            for (std::size_t i = 1; i < pairs.size(); ++i) {
                if (! pairs[i].edge) continue;
                if (!~min_idx || pairs[i].cost < pairs[min_idx].cost) {
                    if (G.IsContractable(pairs[i].edge)) min_idx = i;
                    else pairs[i].edge = nullptr;
                }
            }
            if (!~min_idx) break;

            // top:    the contractable pair with minimal cost
            // v1:     the reserved vertex
            // v2:     the removed vertex
            // result: the contract result
            // ring:   the edge ring of vertex v1
            ContractionPair & top    = pairs[min_idx];
            auto               v1     = top.edge->From();
            auto               v2     = top.edge->To();
            auto               result = G.Contract(top.edge);
            auto               ring   = G.Vertex(v1)->Ring();

            top.edge             = nullptr;            // The contraction has already been done, so the pair is no longer valid. Mark it as invalid.
            output.Positions[v1] = top.targetPosition; // Update the positions.

            // We do something to repair $pair_map$ and $pairs$ because some edges and vertices no longer exist.
            for (int i = 0; i < 2; ++i) {
                DCEL::EdgeIdx removed           = G.IndexOf(result.removed_edges[i].first);
                DCEL::EdgeIdx collapsed         = G.IndexOf(result.collapsed_edges[i].second);
                pairs[pair_map[removed]].edge   = result.collapsed_edges[i].first;
                pairs[pair_map[collapsed]].edge = nullptr;
                pair_map[collapsed]             = pair_map[G.IndexOf(result.collapsed_edges[i].first)];
            }

            // For the two wing vertices, each of them lose one incident face.
            // So, we update the Q matrix.
            Qv[result.removed_faces[0].first] -= Kf[G.IndexOf(result.removed_faces[0].second)];
            Qv[result.removed_faces[1].first] -= Kf[G.IndexOf(result.removed_faces[1].second)];

            // For the vertex v1, Q matrix should be recomputed.
            // And as the position of v1 changed, all the vertices which are on the ring of v1 should update their Q matrix as well.
            Qv[v1] = glm::mat4(0);
            for (auto e : ring) {
                // your code here:
                //     1. Compute the new Kp matrix for $e->Face()$.
                //     2. According to the difference between the old Kp (in $Kf$) and the new Kp (computed in step 1),
                //        update Q matrix of each vertex on the ring (update $Qv$).
                //     3. Update Q matrix of vertex v1 as well (update $Qv$).
                //     4. Update $Kf$.

                glm::mat4 new_kp = UpdateQ(e->Face());
                for(int index = 0; index <= 2; index++){ 
                    int target_index = e->Face()->VertexIndex(index); // index of vertex
                    if (target_index == v1) continue;
                    Qv[target_index] += (new_kp - Kf[G.IndexOf(e->Face())]);
                }
                Qv[v1] += new_kp;

                Kf[G.IndexOf(e->Face())] = new_kp;
            }

            // Finally, as the Q matrix changed, we should update the relative $ContractionPair$ in $pairs$.
            // Any pair with the Q matrix of its endpoints changed, should be remade by $MakePair$.
            // your code here:
            for (size_t i = 0 ; i < pairs.size(); ++i) {
                if(pairs[i].edge && (pairs[i].edge->From() == v1)) {
                    pairs[i] = MakePair(pairs[i].edge, output.Positions[v1], output.Positions[pairs[i].edge->To()], Qv[v1] + Qv[pairs[i].edge->To()]);
                }
                
                else if(pairs[i].edge && (pairs[i].edge->To() == v1)){
                    pairs[i] = MakePair(pairs[i].edge, output.Positions[pairs[i].edge->From()], output.Positions[v1], Qv[v1] + Qv[pairs[i].edge->From()]);
                }
            }

        }

        // In the end, we check if the result mesh is watertight and manifold.
        if (! G.DebugWatertightManifold()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::SimplifyMesh(..): Result is not watertight manifold.");
        }

        auto exported = G.ExportMesh();
        output.Indices.swap(exported.Indices);
    }

    /******************* 4. Mesh Smoothing *****************/
    void SmoothMesh(Engine::SurfaceMesh const & input, Engine::SurfaceMesh & output, std::uint32_t numIterations, float lambda, bool useUniformWeight) {
        // Define function to compute cotangent value of the angle v1-vAngle-v2
        static constexpr auto GetCotangent {
            [] (glm::vec3 vAngle, glm::vec3 v1, glm::vec3 v2) -> float {
                // your code here:
                glm::vec3 e1 = v1 - vAngle;
                glm::vec3 e2 = v2 - vAngle;

                float cos_val = glm::dot(e1, e2);
                glm::vec3 sin_vec = glm::cross(e1, e2);
                float sin_val = glm::length(sin_vec);

                if(sin_val < 1e-8f)
                    return 0.0f;

                if(cos_val < 0)
                    return 0.0f;

                return cos_val / sin_val;
            }
        };

        DCEL G(input);
        if (! G.IsManifold()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::SmoothMesh(..): Non-manifold mesh.");
            return;
        }
        // We only allow watertight mesh.
        if (! G.IsWatertight()) {
            spdlog::warn("VCX::Labs::GeometryProcessing::SmoothMesh(..): Non-watertight mesh.");
            return;
        }

        Engine::SurfaceMesh prev_mesh;
        prev_mesh.Positions = input.Positions;
        for (std::uint32_t iter = 0; iter < numIterations; ++iter) {
            Engine::SurfaceMesh curr_mesh = prev_mesh;
            for (std::size_t i = 0; i < input.Positions.size(); ++i) {
                // your code here: curr_mesh.Positions[i] = ...
                auto v = G.Vertex(i);
                auto rings = v->Ring();

                glm::vec3 v_old = (1 - lambda) * curr_mesh.Positions[i];
                glm::vec3 sum = {0.0f, 0.0f, 0.0f};
                float sum_val = 0.0f;
                for(auto ring : rings){
                    glm::vec3 vi = curr_mesh.Positions[ring->From()];
                    glm::vec3 vj = curr_mesh.Positions[ring->To()];
                    glm::vec3 b = curr_mesh.Positions[ring->NextEdge()->To()];
                    glm::vec3 a = curr_mesh.Positions[ring->TwinEdge()->NextEdge()->To()];
                    if(useUniformWeight){
                        sum += vj;
                        sum_val += 1.0f;
                    }
                    else{
                        float aij = GetCotangent(a, vi, vj);
                        float bij = GetCotangent(b, vi, vj);
                        sum += (aij + bij) * vj;
                        sum_val += aij + bij;
                    }
                }

                glm::vec3 v_new = v_old + lambda * sum / sum_val;
                curr_mesh.Positions[i] = v_new;
            }
            // Move curr_mesh to prev_mesh.
            prev_mesh.Swap(curr_mesh);
        }
        // Move prev_mesh to output.
        output.Swap(prev_mesh);
        // Copy indices from input.
        output.Indices = input.Indices;
    }

    /******************* 5. Marching Cubes *****************/
    void MarchingCubes(Engine::SurfaceMesh & output, const std::function<float(const glm::vec3 &)> & sdf, const glm::vec3 & grid_min, const float dx, const int n) {
        // your code here:
        glm::vec3 unit[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
        std::unordered_map<int, std::vector<std::pair<int, glm::vec3>>> Index_to_Cubes; // map: Index -> cube_mesh_edge[ pair< index(0 ~ 11), 插值点 > ]
        int cnt = 0;
        for(size_t z = 0 ; z < n ; z++){
            for(size_t y = 0 ; y < n ; y++){
                for(size_t x = 0 ; x < n ; x++){
                    glm::vec3 point = grid_min + glm::vec3{x * dx, y * dx, z * dx};
                    uint32_t v = 0;
                    for(int i = 0 ; i < 8 ; i++){
                        // (x + (i & 1) * dx, y + ((i >> 1) & 1) * dx, z + (i >> 2) * dx)
                        glm::vec3 cur = point + glm::vec3{ (i & 1) * dx, ((i >> 1) & 1) * dx, (i >> 2) * dx };
                        if(sdf(cur) > 0.0f) v |= (1 << i);
                    }

                    uint32_t f = c_EdgeStateTable[v];
                    std::unordered_map<int, std::pair<int, glm::vec3>> edge_to_points;
                    for(int j = 0 ; j < 12 ; j++){
                        if(f & (1 << j)){
                            // 第 j 条边的起始点为 ： v0 + dx * (j & 1) * unit(((j >> 2) + 1) % 3) + dx * ((j >> 1) & 1) * unit(((j >> 2) + 2) % 3)
                            glm::vec3 ej_from = point + dx * (j & 1) * unit[((j >> 2) + 1) % 3] + dx * ((j >> 1) & 1) * unit[((j >> 2) + 2) % 3];
                            glm::vec3 ej_to   = ej_from + dx * unit[j >> 2];

                            float p1 = sdf(ej_from);
                            float p2 = sdf(ej_to);

                            glm::vec3 target = (ej_from * p2 - ej_to * p1) / (p2 - p1);
                            int index = -1;

                            if(x > 0 && index == -1){
                                int nearby = z * n * n + y * n + (x - 1);
                                auto edges = Index_to_Cubes[nearby];
                                for(auto edge_pair : edges){
                                    glm::vec3 p = edge_pair.second;
                                    glm::vec3 distance = glm::abs(p - target);
                                    if(distance.x < dx / 1000 && distance.y < dx / 1000 && distance.z < dx / 1000){
                                        index = edge_pair.first;
                                        target = edge_pair.second;
                                        break;
                                    }
                                }
                            }
                            if(y > 0 && index == -1){
                                int nearby = z * n * n + (y - 1) * n + x;
                                auto edges = Index_to_Cubes[nearby];
                                for(auto edge_pair : edges){
                                    glm::vec3 p = edge_pair.second;
                                    glm::vec3 distance = glm::abs(p - target);
                                    if(distance.x < dx / 1000 && distance.y < dx / 1000 && distance.z < dx / 1000){
                                        index = edge_pair.first;
                                        target = edge_pair.second;
                                        break;
                                    }
                                }
                            }
                            if(z > 0 && index == -1){
                                int nearby = (z - 1) * n * n + y * n + x;
                                auto edges = Index_to_Cubes[nearby];
                                for(auto edge_pair : edges){
                                    glm::vec3 p = edge_pair.second;
                                    glm::vec3 distance = glm::abs(p - target);
                                    if(distance.x < dx / 1000 && distance.y < dx / 1000 && distance.z < dx / 1000){
                                        index = edge_pair.first;
                                        target = edge_pair.second;
                                        break;
                                    }
                                }
                            }

                            edge_to_points[j] = std::make_pair(index, target);
                        }
                    }

                    // according to table, add the edge
                    int k = 0;
                    while(c_EdgeOrdsTable[v][3 * k] != -1){
                        int Indexs[3] = {c_EdgeOrdsTable[v][3 * k], c_EdgeOrdsTable[v][3 * k + 1], c_EdgeOrdsTable[v][3 * k + 2]};
                        for(int j = 2 ; j >= 0 ; j--){
                            int edge_index = Indexs[j];
                            int v_index = edge_to_points[edge_index].first;
                            glm::vec3 v = edge_to_points[edge_index].second;
                            if(v_index == -1){ // 未加入过
                                Index_to_Cubes[z * n * n + y * n + x].push_back(std::make_pair(cnt, v));
                                output.Indices.push_back(cnt);
                                output.Positions.push_back(v);
                                edge_to_points[edge_index] = std::make_pair(cnt, v); // 如果在本次while循环中后续还使用到这个点，那么就不用重复计算了
                                cnt++;
                            }
                            else{ // 之前已有该点
                                Index_to_Cubes[z * n * n + y * n + x].push_back(std::make_pair(v_index, v));
                                output.Indices.push_back(v_index); // 记录三角形面片用上的点，索引可能有重复
                            }
                        }
                        k++;
                    }
                }
            }
        }
    }
} // namespace VCX::Labs::GeometryProcessing

