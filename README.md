### Reproduction UCMP
Reproduction of packet-level simulation code used in SIGCOMM 2024 paper (UCMP) "Uniform-Cost Multi-Path Routing for Reconfigurable Data Center Networks"
You can find the original git depot [here](#https://github.com/ht-gong/UCMP.git)

### Building
We compile using `g++ 10.2.1` on Debian 11.

### Reference hardware
We ran all of our simulations on a Debian 11 machine with a Intel Xeon Gold 6130	16 cores/CPU x86_64 and 192GiB RAM. 

## Description

The /src directory contains the packet simulator source code. There is a separate simulator for each network type (i.e., UCMP, Opera). The packet simulator is an extension of the htsim Opera simulator (https://github.com/TritonNetworking/opera-sim).

### Repo Structure:
```
/
├─ topologies/ -- network topology files
├─ src/ -- source for the htsim simulator
│  ├─ optiroute/ -- for UCMP, ksp
│  ├─ opera/ -- for Opera(NSDI '20), RotorNet(SIGCOMM '17)
├─ run/ -- where simulator runs are initiated, results and plotting scripts are stored
├─ traffic --  for generating synthetic traffic traces
```

## Build instructions:

If you want to rerun the simulation, from the main directory, you should compile all the executables using:

#### Opera

```
cd src/opera
make
cd datacenter
make
```

#### UCMP

```
cd src/optiroute
make
cd datacenter
make
```

## Running the simulations and reproducing the results

### Getting the topology files

Compared to the original we didn't run the Opera k=5 algorithme, because it didn't seem to work, all the topologie necessary are present in this depot.

### Running the simulations via automated script

If you just want acces to the data they are in **run/FigureX/figures** directory, replacing X by the figure you want to see, name of figure are explicit. 
If you want to run the simulation go to **run/FigureX** and run the desired script, 
if you have limited ressource and can't run the simulation for too long you have our script that run each simulation on their own, or you can run directly all the script at the same time.
Because simulation were way too long (10h minimum per simulation) we only run the simulation for 0.1s **simulation** time instead of 0.8 done in the original, if you are in the same case make sur to add the paramater -s follow by your desired simulation time. 

### Estimation of Running Time and Memory Consumption

Figure 6ac and 7
| Run Name                                    | Time  | RAM  |
|---------------------------------------------|-------|------|
| OptiRoute_websearch_ndp, SIMTIME 0.8sec    | 10hrs | 3GB  |
| OptiRoute_websearch_dctcp, SIMTIME 0.8sec  | 10hrs | 3GB  |
| Opera_5paths_websearch_ndp, SIMTIME 0.8sec | 15hrs | 3GB  |
| Opera_1path_websearch_ndp, SIMTIME 0.8sec  | 15hrs | 3GB  |
| VLB_websearch_rotorlb, SIMTIME 0.8sec      | 10hrs | 3GB  |
| ksp_topK=1_websearch_dctcp, SIMTIME 0.8sec | 10hrs | 3GB  |
| ksp_topK=5_websearch_dctcp, SIMTIME 0.8sec | 10hrs | 3GB  |

Figure 6bd
| Run Name                                    | Time  | RAM  |
|---------------------------------------------|-------|------|
| OptiRoute_datamining_ndp, SIMTIME 0.8sec    | 10hrs | **100GB**  |
| OptiRoute_datamining_dctcp, SIMTIME 0.8sec  | 10hrs | **100GB**   |
| Opera_5paths_datamining_ndp, SIMTIME 0.8sec | 15hrs | **100GB**   |
| Opera_1path_datamining_ndp, SIMTIME 0.8sec  | 15hrs | **100GB**   |
| VLB_datamining_rotorlb, SIMTIME 0.8sec      | 10hrs | **80GB**   |
| ksp_topK=1_datamining_dctcp, SIMTIME 0.8sec | 10hrs | **80GB**   |
| ksp_topK=5_datamining_dctcp, SIMTIME 0.8sec | 10hrs | **80GB**   |

Figure 8

| Run Name                                    | Time  | RAM  |
|---------------------------------------------|-------|------|
| OptiRoute_websearch_dctcp, SIMTIME 0.8sec    | 10hrs | 3GB  |
| OptiRoute_websearch_dctcp_aging, SIMTIME 0.8sec  | 10hrs | 3GB   |


Figure 9

| Run Name                                    | Time  | RAM  |
|---------------------------------------------|-------|------|
| OptiRoute_websearch_dctcp_cfg_10ns, SIMTIME 0.8sec    | 10hrs | 3GB  |
| OptiRoute_websearch_dctcp_cfg_1us, SIMTIME 0.8sec  | 10hrs | 3GB   |
| OptiRoute_websearch_dctcp_cfg_10us, SIMTIME 0.8sec  | 10hrs | 3GB   |


Figure 10

| Run Name                                    | Time  | RAM  |
|---------------------------------------------|-------|------|
| OptiRoute_websearch_dctcp_alpha_0.3, SIMTIME 0.8sec  | 10hrs | 3GB  |
| OptiRoute_websearch_dctcp_alpha_0.5, SIMTIME 0.8sec  | 10hrs | 3GB   |
| OptiRoute_websearch_dctcp_alpha_0.7, SIMTIME 0.8sec  | 10hrs | 3GB   |
