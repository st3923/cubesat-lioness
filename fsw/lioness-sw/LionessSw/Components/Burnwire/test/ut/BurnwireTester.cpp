// ======================================================================
// \title  BurnwireTester.cpp
// \brief  cpp file for Burnwire component test harness implementation class
// ======================================================================

#include "BurnwireTester.hpp"

namespace Components {

namespace {
const U32 DEFAULT_SAFETY_TIMER = Burnwire::DEFAULT_SAFETY_TIMER_S;
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

BurnwireTester ::BurnwireTester()
    : BurnwireGTestBase("BurnwireTester", BurnwireTester::MAX_HISTORY_SIZE),
      component("Burnwire"),
      m_gpioStatus(Drv::GpioStatus::OP_OK) {
    this->initComponents();
    this->connectPorts();
}

BurnwireTester ::~BurnwireTester() {}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void BurnwireTester::startBurn(U32 durationS, U32 cmdSeq) {
    this->sendCmd_START_BURNWIRE(TEST_INSTANCE_ID, cmdSeq, durationS);
}

void BurnwireTester::tick(U32 n) {
    for (U32 i = 0; i < n; i++) {
        this->invoke_to_schedIn(0, 0);
    }
}

void BurnwireTester::assertFullBurn(U32 durationS) {
    // First tick turns the GPIO on
    this->tick(1);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH);
    ASSERT_EVENTS_SetBurnwireState_SIZE(1);
    ASSERT_EVENTS_SetBurnwireState(0, Fw::On::ON);

    // GPIO stays HIGH for the rest of the duration
    this->tick(durationS - 1);
    ASSERT_from_gpioSet_SIZE(1);

    // Exactly durationS ticks after going HIGH, it goes LOW
    this->tick(1);
    ASSERT_from_gpioSet_SIZE(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);
    ASSERT_EVENTS_BurnwireEndCount_SIZE(1);
    ASSERT_EVENTS_BurnwireEndCount(0, durationS);
    ASSERT_EVENTS_SetBurnwireState_SIZE(2);
    ASSERT_EVENTS_SetBurnwireState(1, Fw::On::OFF);

    // And stays off
    this->tick(5);
    ASSERT_from_gpioSet_SIZE(2);
}

Drv::GpioStatus BurnwireTester::from_gpioSet_handler(FwIndexType portNum, const Fw::Logic& state) {
    this->pushFromPortEntry_gpioSet(state);
    return this->m_gpioStatus;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void BurnwireTester::testIdleDoesNothing() {
    this->tick(10);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void BurnwireTester::testNominalBurn() {
    this->startBurn(5, 7);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, BurnwireComponentBase::OPCODE_START_BURNWIRE, 7, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SafetyTimerState_SIZE(1);
    ASSERT_EVENTS_SafetyTimerState(0, 5);

    // The command alone doesn't drive the pin; the rate group does
    ASSERT_from_gpioSet_SIZE(0);

    this->assertFullBurn(5);
}

void BurnwireTester::testDefaultSafetyTimerClamp() {
    // Parameter never set: the FPP default (30 s) applies
    this->startBurn(1000);
    ASSERT_CMD_RESPONSE(0, BurnwireComponentBase::OPCODE_START_BURNWIRE, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SafetyTimerState(0, DEFAULT_SAFETY_TIMER);
    this->assertFullBurn(DEFAULT_SAFETY_TIMER);
}

void BurnwireTester::testConfiguredSafetyTimerClamp() {
    this->paramSet_SAFETY_TIMER(4, Fw::ParamValid::VALID);
    this->paramSend_SAFETY_TIMER(TEST_INSTANCE_ID, 0);
    this->clearHistory();

    this->startBurn(10);
    ASSERT_EVENTS_SafetyTimerState(0, 4);
    this->assertFullBurn(4);
}

void BurnwireTester::testZeroDurationRejected() {
    this->startBurn(0);
    ASSERT_CMD_RESPONSE(0, BurnwireComponentBase::OPCODE_START_BURNWIRE, 0, Fw::CmdResponse::VALIDATION_ERROR);
    this->tick(5);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void BurnwireTester::testStartWhileBurningRejected() {
    this->startBurn(5);
    this->tick(2);

    this->startBurn(30, 1);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, BurnwireComponentBase::OPCODE_START_BURNWIRE, 1, Fw::CmdResponse::BUSY);
    ASSERT_EVENTS_SafetyTimerState_SIZE(1);

    // Original burn finishes on schedule: HIGH at tick 1, LOW at tick 6
    this->tick(3);
    ASSERT_from_gpioSet_SIZE(1);
    this->tick(1);
    ASSERT_from_gpioSet_SIZE(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);
    ASSERT_EVENTS_BurnwireEndCount(0, 5);
}

void BurnwireTester::testStopDuringBurn() {
    this->startBurn(30);
    this->tick(3);  // HIGH for 3 ticks

    this->sendCmd_STOP_BURNWIRE(TEST_INSTANCE_ID, 2);
    ASSERT_CMD_RESPONSE(1, BurnwireComponentBase::OPCODE_STOP_BURNWIRE, 2, Fw::CmdResponse::OK);
    ASSERT_from_gpioSet_SIZE(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);
    ASSERT_EVENTS_BurnwireEndCount(0, 3);
    ASSERT_EVENTS_SetBurnwireState(1, Fw::On::OFF);

    // No further GPIO activity
    this->tick(40);
    ASSERT_from_gpioSet_SIZE(2);
}

void BurnwireTester::testStopBeforeFirstTick() {
    this->startBurn(10);
    this->sendCmd_STOP_BURNWIRE(TEST_INSTANCE_ID, 0);
    ASSERT_CMD_RESPONSE(1, BurnwireComponentBase::OPCODE_STOP_BURNWIRE, 0, Fw::CmdResponse::OK);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_EVENTS_BurnwireEndCount(0, 0);

    // The pending burn must not start afterwards
    this->tick(5);
    ASSERT_from_gpioSet_SIZE(1);
}

void BurnwireTester::testStopWhileIdle() {
    this->sendCmd_STOP_BURNWIRE(TEST_INSTANCE_ID, 0);
    ASSERT_CMD_RESPONSE(0, BurnwireComponentBase::OPCODE_STOP_BURNWIRE, 0, Fw::CmdResponse::OK);
    // Still forces the pin low, but there's no burn to report
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_EVENTS_SIZE(0);
}

void BurnwireTester::testBurnAgainAfterCompletion() {
    this->startBurn(2);
    this->tick(4);
    this->clearHistory();

    this->startBurn(3);
    ASSERT_CMD_RESPONSE(0, BurnwireComponentBase::OPCODE_START_BURNWIRE, 0, Fw::CmdResponse::OK);
    this->assertFullBurn(3);
}

void BurnwireTester::testGpioFailureReported() {
    this->m_gpioStatus = Drv::GpioStatus::NOT_OPENED;
    this->startBurn(2);
    this->tick(1);
    ASSERT_EVENTS_GpioWriteFailed_SIZE(1);
    ASSERT_EVENTS_GpioWriteFailed(0, Fw::Logic::HIGH, Drv::GpioStatus::NOT_OPENED);

    // Timer still runs, so the component still commands LOW at the end
    this->tick(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);
    ASSERT_EVENTS_GpioWriteFailed_SIZE(2);
    ASSERT_EVENTS_GpioWriteFailed(1, Fw::Logic::LOW, Drv::GpioStatus::NOT_OPENED);
}

}  // namespace Components
