/**
 * @file test_channel_calibration.cpp
 * @brief Unit test per model/ChannelCalibration: conversione lineare G = a*V + b.
 */
#include "TestFramework.h"
#include "model/ChannelCalibration.h"

using am::ChannelCalibration;

TEST_CASE("ChannelCalibration: default (a=1,b=0) e' l'identita'")
{
    const ChannelCalibration c{};
    CHECK_NEAR(c.convert(0.0), 0.0, 1e-12);
    CHECK_NEAR(c.convert(2.5), 2.5, 1e-12);
    CHECK_NEAR(c.convert(5.0), 5.0, 1e-12);
    CHECK_EQ(c.unit, std::string("V"));
}

TEST_CASE("ChannelCalibration: conversione lineare generica")
{
    // Es. un sensore di temperatura LM35-like: 10 mV/°C -> G = 100*V.
    ChannelCalibration c;
    c.a = 100.0;
    c.b = 0.0;
    CHECK_NEAR(c.convert(0.25), 25.0, 1e-9);
    CHECK_NEAR(c.convert(1.0), 100.0, 1e-9);
}

TEST_CASE("ChannelCalibration: offset (b) applicato correttamente")
{
    ChannelCalibration c;
    c.a = 2.0;
    c.b = -1.0;
    CHECK_NEAR(c.convert(0.0), -1.0, 1e-9);
    CHECK_NEAR(c.convert(1.0), 1.0, 1e-9);
    CHECK_NEAR(c.convert(3.0), 5.0, 1e-9);
}

TEST_CASE("ChannelCalibration: a=0 collassa alla costante b")
{
    ChannelCalibration c;
    c.a = 0.0;
    c.b = 7.5;
    CHECK_NEAR(c.convert(0.0), 7.5, 1e-9);
    CHECK_NEAR(c.convert(999.0), 7.5, 1e-9);
}
