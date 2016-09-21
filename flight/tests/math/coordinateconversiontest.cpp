#include "gtest/gtest.h"
#define __STDC_WANT_DEC_FP__ /* Tell implementation that we want Decimal FP */

#include <stdio.h> /* printf */
#include <stdlib.h> /* abort */
#include <string.h> /* memset */

extern "C" {
#include <inc/CoordinateConversions.h>
}

#define epsilon_deg        0.00001f
#define epsilon_int_deg    ((int32_t)(epsilon_deg * 1e7))
#define epsilon_metric     0.2f
#define epsilon_int_metric ((int32_t)(epsilon_metric * 1e4))

// To use a test fixture, derive a class from testing::Test.
class CoordinateConversionsTestRaw : public testing::Test {};

// ****** convert Lat,Lon,Alt to ECEF  ************
// void LLA2ECEF(const int32_t LLAi[3], float ECEF[3]);

// ****** convert ECEF to Lat,Lon,Alt *********
// void ECEF2LLA(const float ECEF[3], int32_t LLA[3]);

// void RneFromLLA(const int32_t LLAi[3], float Rne[3][3]);

// ****** Express LLA in a local NED Base Frame and back ********
// void LLA2Base(const int32_t LLAi[3], const float BaseECEF[3], float Rne[3][3], float NED[3]);
// void Base2LLA(const float NED[3], const float BaseECEF[3], float Rne[3][3], int32_t LLAi[3]);

// ****** Express ECEF in a local NED Base Frame and back ********
// void ECEF2Base(const float ECEF[3], const float BaseECEF[3], float Rne[3][3], float NED[3]);
// void Base2ECEF(const float NED[3], const float BaseECEF[3], float Rne[3][3], float ECEF[3]

TEST_F(CoordinateConversionsTestRaw, LLA2ECEF) {
    int32_t LLAi[3] = {
        419291818,
        125571688,
        50 * 1e4
    };
    int32_t LLAfromECEF[3];

    float ecef[3];

    LLA2ECEF(LLAi, ecef);
    ECEF2LLA(ecef, LLAfromECEF);

    EXPECT_NEAR(LLAi[0], LLAfromECEF[0], epsilon_int_deg);
    EXPECT_NEAR(LLAi[1], LLAfromECEF[1], epsilon_int_deg);
    EXPECT_NEAR(LLAi[2], LLAfromECEF[2], epsilon_int_metric);
}


TEST_F(CoordinateConversionsTestRaw, LLA2NED) {
    int32_t LLAi[3] = {
        419291818,
        125571688,
        50 * 1e4
    };
    int32_t LLAfromNED[3];

    int32_t HomeLLAi[3] = {
        419291600,
        125571300,
        24 * 1e4
    };

    float Rne[3][3];
    float baseECEF[3];
    float NED[3];

    RneFromLLA(HomeLLAi, Rne);
    LLA2ECEF(HomeLLAi, baseECEF);

    LLA2Base(LLAi, baseECEF, Rne, NED);
    Base2LLA(NED, baseECEF, Rne, LLAfromNED);

    EXPECT_NEAR(LLAi[0], LLAfromNED[0], epsilon_int_deg);
    EXPECT_NEAR(LLAi[1], LLAfromNED[1], epsilon_int_deg);
    EXPECT_NEAR(LLAi[2], LLAfromNED[2], epsilon_int_metric);
}
