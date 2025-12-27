# Poker Equity Calculator

This is a **pok**er **eq**ui**t**y/probability calculator. There are two versions:
- `2p_analytical`: Enumerates through all possible hands and board configurations of a 2-player game to give you an exact value. Runs in C++.
- `mc_cuda`: Uses Monte Carlo simulation to estimate the equity of all $n$-player games _simultaneously_, where $2\le n\le 9$. Runs in CUDA.

I also provided an in-depth 3-part write-up on the process of creating this:
- [How [not] to run a Monte Carlo simulation](https://blog.ngoc.io/posts/monte-carlo)
- [Control Variates and the Satisfying Telescoping Sum](https://blog.ngoc.io/posts/control-variate)
- [Stacking chips and CUDA cores](https://blog.ngoc.io/posts/stacking-chips)

## Analytical heads-up (2-player) calculator

This version of the code enumerates through all possible hands and board configurations to give you an exact value.

To run the code, `cd` into the `2p_analytical` directory and run:
```bash
g++ -std=c++20 -o main main.cpp cards.cpp cards_dev.cpp -I. && ./main
```

Everything is written in branchless code to avoid any performance hit.
Change the parallelization by modifying the `MAX_JOBS` constant in `main.cpp`; by default it is set to the number of physical CPU cores you have via `std::thread::hardware_concurrency()`.

The precomputed results are stored in the `results` directory. You can run `query_matchup.py` to query the heads-up outcomes of any two canonical hands.

## Monte Carlo $n$-player estimator

This version of the code uses Monte Carlo simulation to estimate the probabilities of all $n$-player games simultaneously.

To run the code, `cd` into the `mc_cuda` directory and run:
```bash
nvcc -std=c++20 -rdc=true -o main main.cu cards.cu -I. && ./main
```

You can change the grid/block sizes (`THREADS_PER_BLOCK`, `BLOCKS_PER_GRID`) to tune for maximum utilization, and the number of simulations done per thread (`SIMS_PER_THREAD`) depending on how long you want it to run before it writes out to files. The way it's running is, every thread/grid/block is launched at the same time on the default stream, then we synchronise and write out the result at the end of each such iteration — so if you want to space out disk I/O, make each thread run more simulations, and vice versa.