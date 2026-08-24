// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ethics_ai_ldm7_focused.cpp
 * @brief LDM-7: Māori Ethics and Latin-American Liberation Theology school descriptors.
 *
 * ## Test families (LDM7-01..LDM7-04)
 *
 *   LDM7-01  LDM7Schools constants are non-empty and distinct
 *   LDM7-02  CulturalEthicsSchoolDescriptor default-constructs correctly
 *   LDM7-03  Descriptor for MAORI_ETHICS has expected cultural context fields
 *   LDM7-04  Descriptor for LATIN_LIBERATION_THEOLOGY has expected norm sources
 *
 * @see include/ethics_ai/ethics_ai_types.h — LDM-7 types
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"

#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ─────────────────────────────────────────────────────────────────────────────
// LDM7-01 — Constants
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM7-01: LDM7Schools constants are non-empty and distinct.
 */
TEST(LDM7Schools, ConstantsAreNonEmptyAndDistinct) {
    EXPECT_FALSE(std::string(LDM7Schools::MAORI_ETHICS).empty());
    EXPECT_FALSE(std::string(LDM7Schools::LATIN_LIBERATION_THEOLOGY).empty());
    EXPECT_NE(std::string(LDM7Schools::MAORI_ETHICS),
              std::string(LDM7Schools::LATIN_LIBERATION_THEOLOGY));
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM7-02 — Default construction
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM7-02: CulturalEthicsSchoolDescriptor default-constructs correctly.
 */
TEST(CulturalEthicsSchoolDescriptor, DefaultConstructed) {
    CulturalEthicsSchoolDescriptor desc;
    EXPECT_TRUE(desc.school_id.empty());
    EXPECT_TRUE(desc.display_name.empty());
    EXPECT_TRUE(desc.primary_norm_sources.empty());
    EXPECT_DOUBLE_EQ(desc.bias_correction_factor, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM7-03 — Māori Ethics descriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM7-03: A Māori ethics descriptor can be constructed with required fields.
 */
TEST(CulturalEthicsSchoolDescriptor, MaoriEthicsDescriptor) {
    CulturalEthicsSchoolDescriptor maori;
    maori.school_id             = LDM7Schools::MAORI_ETHICS;
    maori.display_name          = "Māori Ethics";
    maori.cultural_context      = "Relational ethics grounded in whakapapa (genealogy), "
                                  "kaitiakitanga (guardianship), and mana (authority/dignity).";
    maori.primary_norm_sources  = {"Te Tiriti o Waitangi", "UN Declaration on Rights of Indigenous Peoples"};
    maori.bias_correction_factor = 1.2;  // initial boost to counter historical under-representation

    EXPECT_EQ(maori.school_id, std::string(LDM7Schools::MAORI_ETHICS));
    EXPECT_EQ(maori.primary_norm_sources.size(), 2u);
    EXPECT_DOUBLE_EQ(maori.bias_correction_factor, 1.2);
    EXPECT_FALSE(maori.cultural_context.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM7-04 — Latin Liberation Theology descriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM7-04: A Latin Liberation Theology descriptor can be constructed.
 */
TEST(CulturalEthicsSchoolDescriptor, LatinLiberationTheologyDescriptor) {
    CulturalEthicsSchoolDescriptor llth;
    llth.school_id             = LDM7Schools::LATIN_LIBERATION_THEOLOGY;
    llth.display_name          = "Latin-American Liberation Theology";
    llth.cultural_context      = "Preferential option for the poor (Gutierrez); "
                                  "ethics of liberation from structural oppression (Dussel).";
    llth.primary_norm_sources  = {
        "Gutierrez, G. (1971). A Theology of Liberation",
        "Dussel, E. (1973). Ethics of Liberation",
        "Boff, L. (1986). Church: Charism and Power"
    };
    llth.bias_correction_factor = 1.15;

    EXPECT_EQ(llth.school_id, std::string(LDM7Schools::LATIN_LIBERATION_THEOLOGY));
    EXPECT_EQ(llth.primary_norm_sources.size(), 3u);
    EXPECT_DOUBLE_EQ(llth.bias_correction_factor, 1.15);
}
