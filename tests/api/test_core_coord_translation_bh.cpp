/*
 * SPDX-FileCopyrightText: (c) 2023 Tenstorrent Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gtest/gtest.h"
#include "umd/device/blackhole_implementation.h"
#include "umd/device/coordinate_manager.h"

using namespace tt::umd;

// Tests that all physical coordinates are same as all virtual coordinates
// when there is no harvesting.
TEST(CoordinateManager, CoordinateManagerBlackholeNoHarvesting) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE);

    // We expect full grid size since there is no harvesting.
    tt_xy_pair tensix_grid_size = tt::umd::blackhole::TENSIX_GRID_SIZE;
    for (size_t x = 0; x < tensix_grid_size.x; x++) {
        for (size_t y = 0; y < tensix_grid_size.y; y++) {
            CoreCoord logical_coords = CoreCoord(x, y, CoreType::TENSIX, CoordSystem::LOGICAL);
            CoreCoord virtual_coords = coordinate_manager->to(logical_coords, CoordSystem::VIRTUAL);
            CoreCoord physical_coords = coordinate_manager->to(logical_coords, CoordSystem::PHYSICAL);

            // Virtual and physical coordinates should be the same.
            EXPECT_EQ(physical_coords.x, virtual_coords.x);
            EXPECT_EQ(physical_coords.y, virtual_coords.y);
        }
    }
}

// Test basic translation to virtual and physical noc coordinates.
// We expect that the top left core will have virtual and physical coordinates (1, 2) and (2, 2) for
// the logical coordinates if the first row is harvested.
TEST(CoordinateManager, CoordinateManagerBlackholeTopLeftCore) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 1);
    tt_xy_pair tensix_grid_size = tt::umd::blackhole::TENSIX_GRID_SIZE;

    CoreCoord logical_coords = CoreCoord(0, 0, CoreType::TENSIX, CoordSystem::LOGICAL);

    // Always expect same virtual coordinate for (0, 0) logical coordinate.
    CoreCoord virtual_cords = coordinate_manager->to(logical_coords, CoordSystem::VIRTUAL);
    EXPECT_EQ(virtual_cords, CoreCoord(1, 2, CoreType::TENSIX, CoordSystem::VIRTUAL));

    // This depends on harvesting mask. So expected physical coord is specific to this test and Blackhole arch.
    CoreCoord physical_cords = coordinate_manager->to(logical_coords, CoordSystem::PHYSICAL);
    EXPECT_EQ(physical_cords, CoreCoord(2, 2, CoreType::TENSIX, CoordSystem::PHYSICAL));
}

// Test logical to physical coordinate translation.
// For the full grid of logical coordinates we expect that there are no duplicates of physical coordinates.
// For the reverse mapping back of physical to logical coordinates we expect that same logical coordinates are returned
// as from original mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeLogicalPhysicalMapping) {
    const size_t max_num_harvested_x = 14;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_harvested_x); harvesting_mask++) {
        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_physical;
        std::set<CoreCoord> physical_coords_set;
        tt_xy_pair tensix_grid_size = tt::umd::blackhole::TENSIX_GRID_SIZE;

        size_t num_harvested_x = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < tensix_grid_size.x - num_harvested_x; x++) {
            for (size_t y = 0; y < tensix_grid_size.y; y++) {
                CoreCoord logical_coords = CoreCoord(x, y, CoreType::TENSIX, CoordSystem::LOGICAL);
                CoreCoord physical_coords = coordinate_manager->to(logical_coords, CoordSystem::PHYSICAL);
                logical_to_physical[logical_coords] = physical_coords;

                // Expect that logical to physical translation is 1-1 mapping. No duplicates for physical coordinates.
                EXPECT_EQ(physical_coords_set.count(physical_coords), 0);
                physical_coords_set.insert(physical_coords);
            }
        }

        EXPECT_EQ(physical_coords_set.size(), tensix_grid_size.y * (tensix_grid_size.x - num_harvested_x));

        for (auto it : logical_to_physical) {
            CoreCoord physical_coords = it.second;
            CoreCoord logical_coords = coordinate_manager->to(physical_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of physical coordinates gives the same logical coordinates
            // using which we got the physical coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test logical to virtual coordinate translation.
// For the full grid of logical coordinates we expect that there are no duplicates of virtual coordinates.
// For the reverse mapping back of virtual to logical coordinates we expect that same logical coordinates are returned
// as from original mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeLogicalVirtualMapping) {
    const size_t max_num_harvested_x = 14;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_harvested_x); harvesting_mask++) {
        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_virtual;
        std::set<CoreCoord> virtual_coords_set;
        tt_xy_pair tensix_grid_size = tt::umd::blackhole::TENSIX_GRID_SIZE;

        size_t num_harvested_x = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < tensix_grid_size.x - num_harvested_x; x++) {
            for (size_t y = 0; y < tensix_grid_size.y; y++) {
                CoreCoord logical_coords = CoreCoord(x, y, CoreType::TENSIX, CoordSystem::LOGICAL);
                CoreCoord virtual_coords = coordinate_manager->to(logical_coords, CoordSystem::VIRTUAL);
                logical_to_virtual[logical_coords] = virtual_coords;

                // Expect that logical to virtual translation is 1-1 mapping. No duplicates for virtual coordinates.
                EXPECT_EQ(virtual_coords_set.count(virtual_coords), 0);
                virtual_coords_set.insert(virtual_coords);
            }
        }

        EXPECT_EQ(virtual_coords_set.size(), tensix_grid_size.y * (tensix_grid_size.x - num_harvested_x));

        for (auto it : logical_to_virtual) {
            CoreCoord virtual_coords = it.second;
            CoreCoord logical_coords = coordinate_manager->to(virtual_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of virtual coordinates gives the same logical coordinates
            // using which we got the virtual coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test logical to translated coordinate translation.
// For the full grid of logical coordinates we expect that there are no duplicates of translated coordinates.
// For the reverse mapping back of translated to logical coordinates we expect that same logical coordinates are
// returned as from original mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeLogicalTranslatedMapping) {
    const size_t max_num_harvested_x = 14;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_harvested_x); harvesting_mask++) {
        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_translated;
        std::set<CoreCoord> translated_coords_set;
        tt_xy_pair tensix_grid_size = tt::umd::blackhole::TENSIX_GRID_SIZE;

        size_t num_harvested_x = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < tensix_grid_size.x - num_harvested_x; x++) {
            for (size_t y = 0; y < tensix_grid_size.y; y++) {
                CoreCoord logical_coords = CoreCoord(x, y, CoreType::TENSIX, CoordSystem::LOGICAL);
                CoreCoord translated_coords = coordinate_manager->to(logical_coords, CoordSystem::TRANSLATED);
                logical_to_translated[logical_coords] = translated_coords;

                // Expect that logical to translated translation is 1-1 mapping. No duplicates for translated
                // coordinates.
                EXPECT_EQ(translated_coords_set.count(translated_coords), 0);
                translated_coords_set.insert(translated_coords);
            }
        }

        EXPECT_EQ(translated_coords_set.size(), tensix_grid_size.y * (tensix_grid_size.x - num_harvested_x));

        for (auto it : logical_to_translated) {
            CoreCoord translated_coords = it.second;
            CoreCoord logical_coords = coordinate_manager->to(translated_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of translated coordinates gives the same logical coordinates
            // using which we got the translated coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test that virtual and translated coordinates are same for all logical coordinates.
// This is expected for Blackhole way of harvesting.
TEST(CoordinateManager, CoordinateManagerBlackholeVirtualEqualTranslated) {
    const size_t max_num_harvested_x = 14;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_harvested_x); harvesting_mask++) {
        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, harvesting_mask);

        size_t num_harvested_x = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < tt::umd::blackhole::TENSIX_GRID_SIZE.x - num_harvested_x; x++) {
            for (size_t y = 0; y < tt::umd::blackhole::TENSIX_GRID_SIZE.y; y++) {
                CoreCoord logical_coords = CoreCoord(x, y, CoreType::TENSIX, CoordSystem::LOGICAL);
                CoreCoord translated_coords = coordinate_manager->to(logical_coords, CoordSystem::TRANSLATED);
                CoreCoord virtual_coords = coordinate_manager->to(logical_coords, CoordSystem::VIRTUAL);

                // Expect that translated coordinates are same as virtual coordinates.
                EXPECT_EQ(translated_coords.x, virtual_coords.x);
                EXPECT_EQ(translated_coords.y, virtual_coords.y);
            }
        }
    }
}

// Test mapping of DRAM coordinates from logical to physical. When there is no DRAM harvesting, logical
// coordinates should cover all physical coordinates.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMNoHarvesting) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE);

    const size_t num_dram_banks = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_noc_ports_per_bank = tt::umd::blackhole::NUM_NOC_PORTS_PER_DRAM_BANK;
    const std::vector<tt_xy_pair>& dram_cores = tt::umd::blackhole::DRAM_CORES;

    for (size_t dram_bank = 0; dram_bank < num_dram_banks; dram_bank++) {
        for (size_t noc_port = 0; noc_port < num_noc_ports_per_bank; noc_port++) {
            const CoreCoord dram_logical(dram_bank, noc_port, CoreType::DRAM, CoordSystem::LOGICAL);
            const size_t physical_core_index = dram_bank * num_noc_ports_per_bank + noc_port;
            const CoreCoord expected_physical = CoreCoord(
                dram_cores[physical_core_index].x,
                dram_cores[physical_core_index].y,
                CoreType::DRAM,
                CoordSystem::PHYSICAL);

            const CoreCoord dram_physical = coordinate_manager->to(dram_logical, CoordSystem::PHYSICAL);

            EXPECT_EQ(dram_physical, expected_physical);
        }
    }
}

// Test top left corner translation from logical to physical coordinates.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMTopLeft) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 0, 1);

    const CoreCoord top_left_dram_logical = CoreCoord(0, 0, CoreType::DRAM, CoordSystem::LOGICAL);
    const CoreCoord expected_top_left_physical = CoreCoord(0, 2, CoreType::DRAM, CoordSystem::PHYSICAL);

    const CoreCoord top_left_physical = coordinate_manager->to(top_left_dram_logical, CoordSystem::PHYSICAL);

    EXPECT_EQ(top_left_physical, expected_top_left_physical);
}

// Test logical to physical DRAM coordinate translation.
// For the full grid of logical coordinates we expect that there are no duplicates of physical coordinates.
// For the reverse mapping back of physical to logical coordinates we expect that same logical coordinates are returned
// as from original mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMLogicalPhysicalMapping) {
    const size_t max_num_banks_harvested = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_dram_banks = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_noc_ports_per_bank = tt::umd::blackhole::NUM_NOC_PORTS_PER_DRAM_BANK;
    const std::vector<tt_xy_pair>& dram_cores = tt::umd::blackhole::DRAM_CORES;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_banks_harvested); harvesting_mask++) {
        if (CoordinateManager::get_num_harvested(harvesting_mask) > 1) {
            continue;
        }

        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 0, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_physical;
        std::set<CoreCoord> physical_coords_set;

        size_t num_banks_harvested = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < num_dram_banks - num_banks_harvested; x++) {
            for (size_t y = 0; y < num_noc_ports_per_bank; y++) {
                const CoreCoord logical_coords = CoreCoord(x, y, CoreType::DRAM, CoordSystem::LOGICAL);
                const CoreCoord physical_coords = coordinate_manager->to(logical_coords, CoordSystem::PHYSICAL);
                logical_to_physical[logical_coords] = physical_coords;

                // Expect that logical to physical translation is 1-1 mapping. No duplicates for physical coordinates.
                EXPECT_EQ(physical_coords_set.count(physical_coords), 0);
                physical_coords_set.insert(physical_coords);
            }
        }

        EXPECT_EQ(physical_coords_set.size(), num_noc_ports_per_bank * (num_dram_banks - num_banks_harvested));

        for (auto it : logical_to_physical) {
            const CoreCoord physical_coords = it.second;
            const CoreCoord logical_coords = coordinate_manager->to(physical_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of physical coordinates gives the same logical coordinates
            // using which we got the physical coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test logical to virtual DRAM coordinate translation.
// For the full grid of logical coordinates it is expected that there are no duplicates of virtual coordinates.
// For the reverse mapping back of virtual to logical coordinates it is expected that same logical coordinates are
// returned as from original mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMLogicalVirtualMapping) {
    const size_t max_num_banks_harvested = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_dram_banks = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_noc_ports_per_bank = tt::umd::blackhole::NUM_NOC_PORTS_PER_DRAM_BANK;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_banks_harvested); harvesting_mask++) {
        if (CoordinateManager::get_num_harvested(harvesting_mask) > 1) {
            continue;
        }

        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 0, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_virtual;
        std::set<CoreCoord> virtual_coords_set;

        size_t num_harvested_banks = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < num_dram_banks - num_harvested_banks; x++) {
            for (size_t y = 0; y < num_noc_ports_per_bank; y++) {
                CoreCoord logical_coords = CoreCoord(x, y, CoreType::DRAM, CoordSystem::LOGICAL);
                CoreCoord virtual_coords = coordinate_manager->to(logical_coords, CoordSystem::VIRTUAL);
                logical_to_virtual[logical_coords] = virtual_coords;

                // Expect that logical to virtual translation is 1-1 mapping. No duplicates for virtual coordinates.
                EXPECT_EQ(virtual_coords_set.count(virtual_coords), 0);
                virtual_coords_set.insert(virtual_coords);
            }
        }

        for (auto it : logical_to_virtual) {
            CoreCoord virtual_coords = it.second;
            CoreCoord logical_coords = coordinate_manager->to(virtual_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of virtual coordinates gives the same logical coordinates
            // using which we got the virtual coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test DRAM translated mapping.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMTranslatedMapping) {
    const size_t max_num_banks_harvested = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_dram_banks = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_noc_ports_per_bank = tt::umd::blackhole::NUM_NOC_PORTS_PER_DRAM_BANK;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_banks_harvested); harvesting_mask++) {
        if (CoordinateManager::get_num_harvested(harvesting_mask) > 1) {
            continue;
        }

        std::shared_ptr<CoordinateManager> coordinate_manager =
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 0, harvesting_mask);

        std::map<CoreCoord, CoreCoord> logical_to_translated;
        std::set<CoreCoord> translated_coord_set;

        const size_t num_harvested_banks = CoordinateManager::get_num_harvested(harvesting_mask);

        for (size_t x = 0; x < num_dram_banks - num_harvested_banks; x++) {
            for (size_t y = 0; y < num_noc_ports_per_bank; y++) {
                const CoreCoord logical_coords = CoreCoord(x, y, CoreType::DRAM, CoordSystem::LOGICAL);
                const CoreCoord translated_coords = coordinate_manager->to(logical_coords, CoordSystem::TRANSLATED);

                EXPECT_GE(translated_coords.x, tt::umd::blackhole::dram_translated_coordinate_start_x);
                EXPECT_GE(translated_coords.y, tt::umd::blackhole::dram_translated_coordinate_start_y);

                logical_to_translated[logical_coords] = translated_coords;

                // Expect that logical to translated translation is 1-1 mapping. No duplicates for translated
                // coordinates.
                EXPECT_EQ(translated_coord_set.count(translated_coords), 0);
                translated_coord_set.insert(translated_coords);
            }
        }

        for (auto it : logical_to_translated) {
            const CoreCoord translated_coords = it.second;
            const CoreCoord logical_coords = coordinate_manager->to(translated_coords, CoordSystem::LOGICAL);

            // Expect that reverse mapping of translated coordinates gives the same logical coordinates
            // using which we got the translated coordinates.
            EXPECT_EQ(it.first, logical_coords);
        }
    }
}

// Test that we cannot create a coordinate manager with more than one DRAM bank harvested.
TEST(CoordinateManager, CoordinateManagerBlackholeDRAMPMoreThanOneDRAMBankHarvested) {
    const size_t max_num_banks_harvested = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_dram_banks = tt::umd::blackhole::NUM_DRAM_BANKS;
    const size_t num_noc_ports_per_bank = tt::umd::blackhole::NUM_NOC_PORTS_PER_DRAM_BANK;

    for (size_t harvesting_mask = 0; harvesting_mask < (1 << max_num_banks_harvested); harvesting_mask++) {
        if (CoordinateManager::get_num_harvested(harvesting_mask) <= 1) {
            continue;
        }

        EXPECT_THROW(
            CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE, 0, harvesting_mask), std::runtime_error);
    }
}

// Test that virtual, physical and translated coordinates are the same for all logical PCIE coordinates.
TEST(CoordinateManager, CoordinateManagerBlackholePCIETranslation) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE);
    const tt_xy_pair pcie_grid_size = tt::umd::blackhole::PCIE_GRID_SIZE;

    for (size_t x = 0; x < pcie_grid_size.x; x++) {
        for (size_t y = 0; y < pcie_grid_size.y; y++) {
            const CoreCoord arc_logical = CoreCoord(x, y, CoreType::PCIE, CoordSystem::LOGICAL);
            const CoreCoord arc_virtual = coordinate_manager->to(arc_logical, CoordSystem::VIRTUAL);
            const CoreCoord arc_physical = coordinate_manager->to(arc_logical, CoordSystem::PHYSICAL);

            EXPECT_EQ(arc_virtual.x, arc_physical.x);
            EXPECT_EQ(arc_virtual.y, arc_physical.y);
        }
    }
}

// Test that virtual, physical and translated coordinates are the same for all logical ARC coordinates.
TEST(CoordinateManager, CoordinateManagerBlackholeARCTranslation) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE);
    const tt_xy_pair arc_grid_size = tt::umd::blackhole::ARC_GRID_SIZE;

    for (size_t x = 0; x < arc_grid_size.x; x++) {
        for (size_t y = 0; y < arc_grid_size.y; y++) {
            const CoreCoord arc_logical = CoreCoord(x, y, CoreType::ARC, CoordSystem::LOGICAL);
            const CoreCoord arc_virtual = coordinate_manager->to(arc_logical, CoordSystem::VIRTUAL);
            const CoreCoord arc_physical = coordinate_manager->to(arc_logical, CoordSystem::PHYSICAL);
            const CoreCoord arc_translated = coordinate_manager->to(arc_logical, CoordSystem::TRANSLATED);

            EXPECT_EQ(arc_virtual.x, arc_physical.x);
            EXPECT_EQ(arc_virtual.y, arc_physical.y);

            EXPECT_EQ(arc_virtual.x, arc_translated.x);
            EXPECT_EQ(arc_virtual.y, arc_translated.y);
        }
    }
}

// Test ethernet coordinate translation.
TEST(CoordinateManager, CoordinateManagerBlackholeETHTranslation) {
    std::shared_ptr<CoordinateManager> coordinate_manager =
        CoordinateManager::create_coordinate_manager(tt::ARCH::BLACKHOLE);
    const tt_xy_pair eth_grid_size = tt::umd::blackhole::ETH_GRID_SIZE;

    const size_t eth_translated_coordinate_start_x = 20;
    const size_t eth_translated_coordinate_start_y = 25;

    for (size_t x = 0; x < eth_grid_size.x; x++) {
        for (size_t y = 0; y < eth_grid_size.y; y++) {
            const CoreCoord eth_logical = CoreCoord(x, y, CoreType::ETH, CoordSystem::LOGICAL);
            const CoreCoord eth_virtual = coordinate_manager->to(eth_logical, CoordSystem::VIRTUAL);
            const CoreCoord eth_physical = coordinate_manager->to(eth_logical, CoordSystem::PHYSICAL);
            const CoreCoord eth_translated = coordinate_manager->to(eth_logical, CoordSystem::TRANSLATED);

            EXPECT_EQ(eth_virtual.x, eth_physical.x);
            EXPECT_EQ(eth_virtual.y, eth_physical.y);

            EXPECT_EQ(eth_translated.x, x + eth_translated_coordinate_start_x);
            EXPECT_EQ(eth_translated.y, eth_translated_coordinate_start_y);
        }
    }
}