## River

River is a design for a concurrent, hierarchical key-value store in the embedded
systems domain. A river is essentially a RAM file system for small pieces of
data that collectively define the state of some part of an application. A river
is decomposed into rivulets that are stored contiguously in memory. Rivulets are
decomposed into smaller rivulets and key-value pairs called channels. Data in a
river can be addressed on the scale of individual channels as fixed-size, typed
data, or entire rivulets as untyped memory ranges.

Locks can be attached to rivulets to make them thread-safe. This allows for
variably-grained locking, e.g., by attaching locks to some data and not others,
individual channels or entire rivulets, the entire river, etc. In the current
implementation, locks cannot overlap (i.e., at most one lock can exist along
each path from the river root to each leaf).

## Example

Suppose we want to create the following river:

```cpp
.
├──system
│  ├──uint64_t time
│  └──bool abort
└──control
   ├──double pressure
   │  └──bool valid
   └──bool valve_open
```

This code creates the `system` rivulet:

```cpp
using namespace river;

Channel<uint64_t> time;
Channel<bool> abort, pressure_valid, valve_open;
Channel<double> pressure;

Builder builder;
builder.channel("system.time", 0ul, time);
builder.channel("system.abort", false, abort);
```

This code creates the `control` rivulet using a "rivulet builder" branched off
of the root builder:

```cpp
Builder control_builder;
builder.sub("control", control_builder);
control_builder.channel("pressure", 14.7, pressure);
control_builder.channel("pressure.valid", true, pressure_valid);
control_builder.channel("valve_open", false, valve_open);
```

`Channel`s are handles to individual channels. A `Rivulet` is a handle to the
memory backing an entire rivulet:

```cpp
Rivulet control_rivulet;
builder.rivulet("control", control_rivulet);
```

Maybe the `control` rivulet will be accessed from multiple threads at once, so
we want to make that rivulet thread-safe:

```cpp
builder.lock("control", std::shared_ptr<Lock>(new YourLockType));
```

Then the river can be built, and the `Channel`s and `Rivulet`s used to read and
write the river:

```cpp
builder.build();

time.set(1e9);
valve_open.set(pressure.get() > 14.7 || !pressure_valid.get());

std::vector<uint8_t> control_data(control_rivulet.size());
control_rivulet.read(control_data.data());
```

The river has this layout in memory:

Rivulet            | Channel      | Byte Offset
------------------ | ------------ | -----------
`system`           | `time`       | `0x0`
`system`           | `abort`      | `0x8`
`control`          | `pressure`   | `0x9`
`control.pressure` | `valid`      | `0x11`
`control`          | `valve_open` | `0x12`
