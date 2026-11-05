// ======================================================================
// \title  BurnwireTestMain.cpp
// \brief  cpp file for Burnwire component test main function
// ======================================================================

#include "BurnwireTester.hpp"

#define BURNWIRE_TEST(name)                \
    TEST(Burnwire, name) {                 \
        Components::BurnwireTester tester; \
        tester.test##name();               \
    }

BURNWIRE_TEST(IdleDoesNothing)
BURNWIRE_TEST(NominalBurn)
BURNWIRE_TEST(DefaultSafetyTimerClamp)
BURNWIRE_TEST(ConfiguredSafetyTimerClamp)
BURNWIRE_TEST(ZeroDurationRejected)
BURNWIRE_TEST(StartWhileBurningRejected)
BURNWIRE_TEST(StopDuringBurn)
BURNWIRE_TEST(StopBeforeFirstTick)
BURNWIRE_TEST(StopWhileIdle)
BURNWIRE_TEST(BurnAgainAfterCompletion)
BURNWIRE_TEST(GpioFailureReported)

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
