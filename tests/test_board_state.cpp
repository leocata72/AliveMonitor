/**
 * @file test_board_state.cpp
 * @brief Unit test per model/BoardState: contatori statistiche, stato
 *        connessione/acquisizione, snapshot coerente e clamp della frequenza
 *        richiesta.
 */
#include <thread>

#include "TestFramework.h"
#include "model/BoardState.h"
#include "Version.h"

using am::AcquisitionState;
using am::BoardState;
using am::ConnectionState;

TEST_CASE("BoardState: stato iniziale")
{
    BoardState state;
    const auto snap = state.snapshot();
    CHECK(snap.connection == ConnectionState::Scanning);
    CHECK(snap.acquisition == AcquisitionState::Stopped);
    CHECK_EQ(snap.stats.packetsReceived, std::uint64_t{ 0 });
    CHECK_EQ(snap.stats.packetsLost, std::uint64_t{ 0 });
    CHECK_NEAR(snap.connectedSeconds, 0.0, 1e-9);
}

TEST_CASE("BoardState: setConnected/setDisconnected aggiornano connection e portName")
{
    BoardState state;
    state.setConnected("COM7", 115200U);
    auto snap = state.snapshot();
    CHECK(snap.connection == ConnectionState::Connected);
    CHECK_EQ(snap.portName, std::string("COM7"));
    CHECK_EQ(snap.baudRate, 115200U);

    state.setDisconnected();
    snap = state.snapshot();
    CHECK(snap.connection == ConnectionState::Disconnected);
    CHECK_EQ(snap.portName, std::string(""));
}

TEST_CASE("BoardState: setScanning azzera portName e firmwareVersion")
{
    BoardState state;
    state.setConnected("COM3", 115200U);
    state.setFirmwareVersion("1.0.0");
    state.setScanning();
    const auto snap = state.snapshot();
    CHECK(snap.connection == ConnectionState::Scanning);
    CHECK_EQ(snap.portName, std::string(""));
    CHECK_EQ(snap.firmwareVersion, std::string(""));
}

TEST_CASE("BoardState: contatori statistiche incrementano indipendentemente")
{
    BoardState state;
    state.incPacketsReceived();
    state.incPacketsReceived();
    state.incPacketsReceived();
    state.addPacketsLost(5);
    state.addPacketsLost(2);
    state.incCrcErrors();
    state.incSerialErrors();
    state.incSerialErrors();

    const auto snap = state.snapshot();
    CHECK_EQ(snap.stats.packetsReceived, std::uint64_t{ 3 });
    CHECK_EQ(snap.stats.packetsLost, std::uint64_t{ 7 });
    CHECK_EQ(snap.stats.crcErrors, std::uint64_t{ 1 });
    CHECK_EQ(snap.stats.serialErrors, std::uint64_t{ 2 });
}

TEST_CASE("BoardState: resetStatistics azzera i contatori e la frequenza misurata")
{
    BoardState state;
    state.incPacketsReceived();
    state.addPacketsLost(3);
    state.setMeasuredRate(123.4);
    state.resetStatistics();

    const auto snap = state.snapshot();
    CHECK_EQ(snap.stats.packetsReceived, std::uint64_t{ 0 });
    CHECK_EQ(snap.stats.packetsLost, std::uint64_t{ 0 });
    CHECK_NEAR(snap.measuredRateHz, 0.0, 1e-9);
}

TEST_CASE("BoardState: setRequestedRate clampa a [kMinSampleRateHz, kMaxSampleRateHz]")
{
    BoardState state;
    state.setRequestedRate(-10);
    CHECK_EQ(state.requestedRate(), am::kMinSampleRateHz);

    state.setRequestedRate(99999);
    CHECK_EQ(state.requestedRate(), am::kMaxSampleRateHz);

    state.setRequestedRate(100);
    CHECK_EQ(state.requestedRate(), 100);
}

TEST_CASE("BoardState: setConfirmedRate/confirmedRate")
{
    BoardState state;
    CHECK_EQ(state.confirmedRate(), 0);
    state.setConfirmedRate(250);
    CHECK_EQ(state.confirmedRate(), 250);
}

TEST_CASE("BoardState: setAcquisition(non Running) azzera measuredRateHz")
{
    BoardState state;
    state.setMeasuredRate(200.0);
    state.setAcquisition(AcquisitionState::Running);
    CHECK_NEAR(state.snapshot().measuredRateHz, 200.0, 1e-9);  // Running: non toccata

    state.setAcquisition(AcquisitionState::Paused);
    CHECK_NEAR(state.snapshot().measuredRateHz, 0.0, 1e-9);  // non Running: azzerata
}

TEST_CASE("BoardState: acquisitionElapsed cresce nel tempo dopo restartAcquisitionClock")
{
    BoardState state;
    state.restartAcquisitionClock();
    const double t0 = state.acquisitionElapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const double t1 = state.acquisitionElapsed();
    CHECK(t1 > t0);
    CHECK(t0 >= 0.0);
}
