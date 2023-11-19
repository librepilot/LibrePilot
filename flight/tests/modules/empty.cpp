/**
 ******************************************************************************
 * @addtogroup SANTY_GTEST
 * @{
 * @addtogroup  SANTY_GETST FUNCTIONS
 * @brief Deals with module mock & test
 * @{
 * @file       empty.cpp
 * @author     The SantyPilot Team, Copyright (C) 2023.
 * @brief      gtest usage demo
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "gtest/gtest.h"

#include <stdio.h> /* printf */
#include <stdlib.h> /* abort */
#include <string.h> /* memset */

extern "C" {

} // extern "C"


class EmptyTest: public testing::Test {
protected:
	virtual void SetUp() {}
	virtual void TearDown() {}
};

// EMPTY TEST
TEST(EmptyTest, Expect) {
    EXPECT_EQ(1, 1);
}

TEST(EmptyTest, SensorsGenerator) {
}
