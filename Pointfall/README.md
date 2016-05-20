# Pointfall

Brute-force parallel particle simulation, solving approximately 8000 - 10,000 local & inter-system collisions per frame (excluding collisions within constraint solvers).

**AsyncSolver:** The core of this sample, handling the distribution and synchronisation of all parts of the physics simulation. Currently uses a fixed radius for all query elements.

