#include <river>

#include "CppUTest/TestHarness.h"

using namespace river;

/**
 * No-op lock that counts the number of times it has been acquired and released.
 */
class NoopLock final : public Lock {
public:
    uint64_t acquire_count = 0;
    uint64_t release_count = 0;

    void acquire() final override
    {
        ++acquire_count;
    }

    void release() final override
    {
        ++release_count;
    }
};

TEST_GROUP(channels) {};

/**
 * Creates the example river in the readme.
 */
TEST(channels, readme)
{
    Channel<uint64_t> time;
    Channel<bool> abort, pressure_valid, valve_open;
    Channel<double> pressure;

    // Create the `system` rivulet.
    Builder builder;
    CHECK_EQUAL(0, builder.channel("system.time", 0ul, time));
    CHECK_EQUAL(0, builder.channel("system.abort", false, abort));

    // Create the `control` rivulet.
    Builder control_builder;
    CHECK_EQUAL(0, builder.sub("control", control_builder));
    CHECK_EQUAL(0, control_builder.channel("pressure", 14.7, pressure));
    CHECK_EQUAL(0, control_builder.channel("pressure.valid", true, pressure_valid));
    CHECK_EQUAL(0, control_builder.channel("valve_open", false, valve_open));

    // Get a handle to the `control` rivulet.
    Rivulet control_rivulet;
    CHECK_EQUAL(0, builder.rivulet("control", control_rivulet));

    // Add a lock to the `control` rivulet.
    NoopLock* const raw_lock = new NoopLock;
    const std::shared_ptr<Lock> lock(raw_lock);
    CHECK_EQUAL(0, builder.lock("control", lock));

    // Build the river.
    CHECK_EQUAL(0, builder.build());

    // Check initial channel values.
    CHECK_EQUAL(0, time.get());
    CHECK_TRUE(!abort.get());
    CHECK_EQUAL(14.7, pressure.get());
    CHECK_TRUE(pressure_valid.get());
    CHECK_TRUE(!valve_open.get());

    // Mutate all channels and re-check values.
    time.set(1e9);
    abort.set(true);
    pressure.set(15.1);
    pressure_valid.set(false);
    valve_open.set(true);

    CHECK_EQUAL(1e9, time.get());
    CHECK_TRUE(abort.get());
    CHECK_EQUAL(15.1, pressure.get());
    CHECK_TRUE(!pressure_valid.get());
    CHECK_TRUE(valve_open.get());

    // Read the `control` rivulet into a vector and check its contents.
    const size_t control_size_bytes = control_rivulet.size();
    CHECK_EQUAL(10, control_size_bytes);
    std::vector<uint8_t> control_data(control_size_bytes);
    control_rivulet.read(control_data.data());

#pragma pack(push, 1)
    struct {
        double pressure = 15.1;
        bool pressure_valid = false;
        bool valve_open = true;
    } expected_control_data;
#pragma pack(pop)

    CHECK_EQUAL(control_size_bytes, sizeof(expected_control_data));
    MEMCMP_EQUAL(&expected_control_data,
                 control_data.data(),
                 control_size_bytes);

    // Check that the lock was used the expected number of times:
    //   * 6x reads of `control` channels
    //   * 3x writes of `control` channels
    //   * 1x read of the entire `control` rivulet
    static constexpr size_t expected_locks = 10;
    CHECK_EQUAL(expected_locks, raw_lock->acquire_count);
    CHECK_EQUAL(expected_locks, raw_lock->release_count);
}

/**
 * Creates a few basic, non-hierarchical channels.
 */
TEST(channels, basic)
{
    Builder builder;
    Channel<int32_t> foo;
    Channel<double> bar;
    Channel<bool> baz;

    CHECK_EQUAL(0, builder.channel("foo", 32, foo));
    CHECK_EQUAL(0, builder.channel("bar", 1.522, bar));
    CHECK_EQUAL(0, builder.channel("baz", true, baz));

    CHECK_EQUAL(0, builder.build());

    CHECK_EQUAL(32, foo.get());
    CHECK_EQUAL(1.522, bar.get());
    CHECK_TRUE(baz.get());

    foo.set(-100);
    bar.set(-9.81);
    baz.set(false);

    CHECK_EQUAL(-100, foo.get());
    CHECK_EQUAL(-9.81, bar.get());
    CHECK_TRUE(!baz.get());
}

/**
 * Creates a small hierarchy of channels.
 */
TEST(channels, hierarchy)
{
    Builder builder;
    Channel<int32_t> foo;
    Channel<int32_t> bar, bar_bar;
    Channel<int32_t> baz, baz_baz, baz_baz_baz;

    CHECK_EQUAL(0, builder.channel("foo", 1, foo));
    CHECK_EQUAL(0, builder.channel("bar", 2, bar));
    CHECK_EQUAL(0, builder.channel("bar.bar", 3, bar_bar));
    CHECK_EQUAL(0, builder.channel("baz", 4, baz));
    CHECK_EQUAL(0, builder.channel("baz.baz", 5, baz_baz));
    CHECK_EQUAL(0, builder.channel("baz.baz.baz", 6, baz_baz_baz));

    CHECK_EQUAL(0, builder.build());

    CHECK_EQUAL(1, foo.get());
    CHECK_EQUAL(2, bar.get());
    CHECK_EQUAL(3, bar_bar.get());
    CHECK_EQUAL(4, baz.get());
    CHECK_EQUAL(5, baz_baz.get());
    CHECK_EQUAL(6, baz_baz_baz.get());
}

/**
 * Attempts to create the same channel twice.
 */
TEST(channels, dupe)
{
    Builder builder;
    Channel<int32_t> foo, dupe_same_type;
    Channel<double> dupe_dif_type;

    CHECK_EQUAL(0, builder.channel("foo", 0, foo));
    CHECK_EQUAL(Builder::ERR_DUPE, builder.channel("foo", 0, dupe_same_type));
    CHECK_EQUAL(Builder::ERR_DUPE, builder.channel("foo", 0.0, dupe_dif_type));

    CHECK_EQUAL(0, builder.build());

    // Try setting all channels to different values.
    foo.set(1);
    dupe_same_type.set(2);
    dupe_dif_type.set(3.0);

    // Value only sticks to the valid channel.
    CHECK_EQUAL(1, foo.get());
    CHECK_EQUAL(0, dupe_same_type.get());
    CHECK_EQUAL(0.0, dupe_dif_type.get());
}
