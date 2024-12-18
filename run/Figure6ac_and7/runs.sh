#!/bin/bash

#get relative path
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$PARENT_PATH"
source ../run_util.sh

echo "Running simulations for ${SIMTIME_FULL}s simtime"
printf "\n"

FNAME=OptiRoute_websearch_ndp_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/optiroute/datacenter/htsim_ndp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 150000000000 -norlb -rlbflow 0 -cwnd 20 -q 65 -pullrate 1 -routing OptiRoute -topfile ../../topologies/slice50us_portion1_queue0_optiroute_adaptive_paths_alpha_0.5.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim -slicedur 50000000 > $FNAME"
run_sim
printf "\n"

FNAME=OptiRoute_websearch_dctcp_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/optiroute/datacenter/htsim_dctcp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 150000000000 -norlb -rlbflow 0 -cwnd 20 -q 300 -routing OptiRoute -topfile ../../topologies/slice50us_portion1_queue0_optiroute_adaptive_paths_alpha_0.5.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim -dctcpmarking 32 -slicedur 50000000 > $FNAME"
run_sim
printf "\n"

FNAME=Opera_1path_websearch_ndp_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/opera/datacenter/htsim_ndp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 150000000000 -norlb -cwnd 20 -q 65 -pullrate 1 -topfile ../../topologies/dynexp_50us_10nsrc_1path.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim > $FNAME"
run_sim
printf "\n"

FNAME=VLB_websearch_rotorlb_opera_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/opera/datacenter/htsim_ndp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 0 -cwnd 20 -q 65 -pullrate 1 -topfile ../../topologies/dynexp_50us_10nsrc_1path.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim > $FNAME"
run_sim
printf "\n"

FNAME=ksp_topK=1_websearch_dctcp_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/optiroute/datacenter/htsim_dctcp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 150000000000 -norlb -rlbflow 0 -cwnd 20 -q 300 -routing KShortest -topfile ../../topologies/general_from_dynexp_N=108_topK=1.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim -dctcpmarking 32 -slicedur 50000000 > $FNAME"
run_sim
printf "\n"

FNAME=ksp_topK=5_websearch_dctcp_0.5sec_${SIMTIME}sec_50us.txt
FILES+=($FNAME)
COMMAND="../../src/optiroute/datacenter/htsim_dctcp_dynexpTopology -utiltime 5e-04 -simtime $SIMTIME_FULL -cutoff 150000000000 -norlb -rlbflow 0 -cwnd 20 -q 300 -routing KShortest -topfile ../../topologies/general_from_dynexp_N=108_topK=5.txt -flowfile ../../traffic/websearch_uniform_40percLoad_0.5sec_648hosts.htsim -dctcpmarking 32 -slicedur 50000000 > $FNAME"
run_sim
printf "\n"

echo "Finished running simulations, plotting figures..."
if [[ $SUCCESS = false ]]; then
    echo "Warning: some simulations seem to have failed!!! Figures may not plot successfully!"
fi

python3 set_config.py $SIMTIME_MS ${FILES[@]}
python3 FCT.py
python3 BE_CDF.py
python3 BE_BAR.py
python3 avg_link_util_agg.py

echo "Done!"
