sim-binary:
	verilator --binary -j 0 -Wall our.v
	obj_dir/Vour

sim-C++:
	verilator --cc --exe --build -j 0 -Wall sim_main.cpp our.v
	obj_dir/Vour

clean:
	rm -rf obj_dir