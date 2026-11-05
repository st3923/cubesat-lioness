// ======================================================================
// \title  Burnwire.cpp
// \author valedictorian101
// \brief  cpp file for Burnwire component implementation class
// ======================================================================

#include "LionessSw/Components/Burnwire/Burnwire.hpp"

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

Burnwire ::Burnwire(const char* const compName) : BurnwireComponentBase(compName) {}

Burnwire ::~Burnwire() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void Burnwire::schedIn_handler(FwIndexType portNum, U32 context) {
    if (this->m_state != Fw::On::ON) {
        return;
    }

    if (this->m_timer == 0) {
        // First tick after the command: start burning
        this->writeGpio(Fw::Logic::HIGH);
        this->log_ACTIVITY_HI_SetBurnwireState(Fw::On::ON);
    } else if (this->m_timer >= this->m_timeout) {
        // GPIO has been HIGH for m_timeout full ticks: stop
        this->stopBurn();
        return;
    }

    this->m_timer++;
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void Burnwire::START_BURNWIRE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 durationS) {
    if (durationS == 0) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    if (this->m_state == Fw::On::ON) {
        // Don't silently extend or restart a burn in progress
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }

    // Before loadParameters() runs the stored value is uninitialized, so fall
    // back to the FPP default rather than trusting it
    Fw::ParamValid valid;
    U32 maxTime = this->paramGet_SAFETY_TIMER(valid);
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        maxTime = DEFAULT_SAFETY_TIMER_S;
    }

    if (durationS > maxTime) {
        durationS = maxTime;
    }

    this->m_timeout = durationS;
    this->m_timer = 0;
    this->m_state = Fw::On::ON;

    this->log_ACTIVITY_HI_SafetyTimerState(durationS);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Burnwire::STOP_BURNWIRE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_state == Fw::On::ON) {
        this->stopBurn();
    } else {
        // Idempotent: always make sure the pin is low
        this->writeGpio(Fw::Logic::LOW);
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void Burnwire::writeGpio(Fw::Logic state) {
    const Drv::GpioStatus status = this->gpioSet_out(0, state);
    if (status != Drv::GpioStatus::OP_OK) {
        this->log_WARNING_HI_GpioWriteFailed(state, status);
    }
}

void Burnwire::stopBurn() {
    this->writeGpio(Fw::Logic::LOW);
    this->log_ACTIVITY_LO_BurnwireEndCount(this->m_timer);
    this->m_state = Fw::On::OFF;
    this->m_timer = 0;
    this->log_ACTIVITY_HI_SetBurnwireState(Fw::On::OFF);
}

}  // namespace Components
