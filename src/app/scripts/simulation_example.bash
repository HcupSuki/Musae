mpirun ./Musae GenCRMu 1000000000 -o primary_crmu.root -h 300 -t 0 0 -200 -r 5 -b 'exp(-200/p[GeV/c]-(t[deg]/45)^2)' -p 10 -z 60
mpirun ./Musae SimFlux
