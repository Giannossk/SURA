#include "test_macros.hpp"
#include <iostream>

// Declarations of tests
void test_surface_mesh_primitives();
void test_surface_mesh_obj_io();
void test_surface_mesh_stl_io();

void test_tetrahedral_mesh_volume_and_barycentric();
void test_tetrahedral_mesh_boundary_extraction();

void test_netgen_mesher_cube();
void test_netgen_mesher_sphere();

void test_simplex_projection();
void test_embedding_and_deformation();

void test_collision_mesh_extraction_and_bullet_export();

void test_organ_full_pipeline();

int main(int argc, char** argv) {
    std::cout << "===================================================\n";
    std::cout << "           SURA ORGAN SIMULATION TESTS             \n";
    std::cout << "===================================================\n";

    // SurfaceMesh
    RUN_TEST(test_surface_mesh_primitives);
    RUN_TEST(test_surface_mesh_obj_io);
    RUN_TEST(test_surface_mesh_stl_io);

    // TetrahedralMesh
    RUN_TEST(test_tetrahedral_mesh_volume_and_barycentric);
    RUN_TEST(test_tetrahedral_mesh_boundary_extraction);

    // Netgen mesher
    RUN_TEST(test_netgen_mesher_cube);
    RUN_TEST(test_netgen_mesher_sphere);

    // Embedding & Simplex projection
    RUN_TEST(test_simplex_projection);
    RUN_TEST(test_embedding_and_deformation);

    // Collision mesh
    RUN_TEST(test_collision_mesh_extraction_and_bullet_export);

    // End-to-end Organ pipeline
    RUN_TEST(test_organ_full_pipeline);

    std::cout << "===================================================\n";
    std::cout << "Tests Passed: " << g_tests_passed << "\n";
    std::cout << "Tests Failed: " << g_tests_failed << "\n";
    std::cout << "===================================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
