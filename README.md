# Map API fork

Forked from ETHZ-ASL's MAP API ([Link](https://github.com/ethz-asl/map_api)), a distributed multi-robot mapping system.

Contains my contributions to the distributed database backend. I worked on robustness to loss of connectivity or robot failures using the [Raft consensus protocol](https://raft.github.io/), as well as [Chord lookup protocol](https://en.wikipedia.org/wiki/Chord_(peer-to-peer)).

Related publications:

1. A. Quraishi, T. Cieslewski, S. Lynen, and R. Siegwart, **Robustness to connectivity loss for collaborative mapping**, in 2016 IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS). 
2. T. Cieslewski, S. Lynen, M. Dymczyk, S. Magnenat, and R. Siegwart, **Map API - scalable decentralized map building for robots**, in 2015 IEEE International Conference on Robotics and Automation (ICRA).
