module Components {
    @ Ignites grounded Burnwire
    passive component Burnwire {

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------
        # Commands and schedIn are guarded: they run on different threads
        # (command dispatcher vs. rate group) and share the burn state.

        @ START_BURNWIRE turns on the burnwire for durationS seconds,
        @ clamped to the SAFETY_TIMER parameter
        guarded command START_BURNWIRE(durationS: U32)

        @ STOP_BURNWIRE turns off the burnwire immediately
        guarded command STOP_BURNWIRE

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event SetBurnwireState(burnwire_state: Fw.On) \
            severity activity high \
            format "Burnwire State: {}"

        event SafetyTimerState(burnwire_status: U32) \
            severity activity high \
            format "Safety Timer Will Burn For: {} Seconds"

        event BurnwireEndCount(end_count: U32) \
            severity activity low \
            format "Burnwire Burned for {} Seconds"

        event GpioWriteFailed(requested: Fw.Logic, status: Drv.GpioStatus) \
            severity warning high \
            format "Burnwire GPIO write {} failed with status {}"

        # ----------------------------------------------------------------------
        # Ports
        # ----------------------------------------------------------------------

        @ Input Port to get the rate group (nominally 1 Hz; one tick = one second)
        guarded input port schedIn: Svc.Sched

        @ Port sending calls to the GPIO driver to start the burnwire
        output port gpioSet: Drv.GpioWrite

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ SAFETY_TIMER parameter is the maximum time (seconds) that the burn component will run
        param SAFETY_TIMER: U32 default 30

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}
