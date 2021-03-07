# The UNI5ON framework

This repository contains the implementation of a scenario for evaluating the UNI5ON framework, involving integrated management and orchestration of an OpenFlow backhaul network interconnecting LTE eNBs to dedicated enhanced EPC (eEPC) core network slices.

It contains the source code for the [ns-3 Network Simulator][ns-3] version 3.29 + the [OFSwitch13 module][ofswitch13] for OpenFlow 1.3 simulations.
The simulation scenario is implemented in scratch/uni5on folder. Topologies files with scenario configurations are available in the topologies folder.

## Downloading and compiling the source code.

Use a recursive clone to get all dependencies:

```bash
git clone --recurse-submodules git@github.com:ljerezchaves/uni5on.git
```

Then, configure and compile the external library for OFSwitch13 support:

```bash
make lib-config-[debug|optimized]
make lib-build
```

Finally, configure and compile the ns-3 simulator:

```bash
make sim-config-[debug|optimized]
make
```

To run a simulation, use the ./simulate.sh script. As an example:

```bash
./simulate.sh uni5on --single 1 topos/default
```

This code has been compiled and tested in Ubuntu 18.04.5 LTS.

[ns-3]: https://www.nsnam.org
[ofswitch13]: https://github.com/ljerezchaves/ofswitch13
