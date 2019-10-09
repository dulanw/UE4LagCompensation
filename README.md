# UE4LagCompensation
This repository contains the components used for lag compensation, these are for projectile based weapons and not hit scans and maybe used a massive performance hit if used with a lot of actors.

It uses object pool to create actors in the scene which creates collision boxes at the location of the player and when a projectile collides, it will check to see if it should have hit, this is done using the difference in the network clock which gives the player ping.

## Update
Currently rework in progress, object pooling and using phsyics for hit detection will be removed, replaced by rewinding using maths which will give better accuracy.