// ======================================================================
// \title  Burnwire.hpp
// \author valedictorian101
// \brief  hpp file for Burnwire component implementation class
// ======================================================================

#ifndef Components_Burnwire_HPP
#define Components_Burnwire_HPP

#include "LionessSw/Components/Burnwire/BurnwireComponentAc.hpp"

namespace Components {

class Burnwire final : public BurnwireComponentBase {
  public:
    //! Fallback burn limit if SAFETY_TIMER was never loaded. Keep in sync with Burnwire.fpp
    static constexpr U32 DEFAULT_SAFETY_TIMER_S = 30;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct Burnwire object
    Burnwire(const char* const compName  //!< The component name
    );

    //! Destroy Burnwire object
    ~Burnwire();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for schedIn
    //!
    //! Input Port to get the rate group
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command START_BURNWIRE
    //!
    //! START_BURNWIRE turns on the burnwire
    void START_BURNWIRE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                   U32 cmdSeq,           //!< The command sequence number
                                   U32 durationS) override;

    //! Handler implementation for command STOP_BURNWIRE
    //!
    //! STOP_BURNWIRE turns off the burnwire immediately
    void STOP_BURNWIRE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq            //!< The command sequence number
                                  ) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Drive the burnwire GPIO, emitting a warning if the driver reports a failure
    void writeGpio(Fw::Logic state);

    //! Drive the GPIO low, report the burn duration, and reset to idle
    void stopBurn();

    Fw::On m_state = Fw::On::OFF;  //!< ON while a burn is commanded (including before the first tick)
    U32 m_timer = 0;               //!< Ticks the GPIO has been held HIGH
    U32 m_timeout = 0;             //!< Commanded burn duration in ticks (seconds at 1 Hz)
};

}  // namespace Components

#endif
