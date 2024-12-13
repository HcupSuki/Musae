cd utility/Musae/GenCRMu
# See ./GenCRMu --help for more information
mpirun ./GenCRMu 1000000000 -o primary_crmu.root -h 300 -t 0 0 -200 -r 5 -b 'exp(-200/p[GeV/c]-(t[deg]/45)^2)' -p 10 -z 60
cd ../../..

mv utility/Musae/GenCRMu/primary_crmu simulation/Musae/GenCRMu

cd simulation/Musae/SimFlux
mpirun ./SimFlux
