# Event reconstruction. See ./Musae ReconLGA --help for more information
./Musae ReconLGA input1.root input2.root input3.root -o recon.root
# Event display, display 0--5 8--9 and 10--12 events. See ./Musae VisLGA --help for more information
./Musae VisLGA recon.root -n 0..5 10..12
