Multiplayer game and networking system.
 
https://github.com/user-attachments/assets/d2aae1ee-09c7-4955-8802-7b7f2feb5af9

*2 clients with 100ms of simulated ping in debug mode. Shown entities are predicted ahead, blue highlighted is last received snapshot from server.*

Almost everything in the game undergoes client side prediction and rollback which allows for instantly responsive player input no matter the ping, and perfect dodging of enemy projectiles. The prediction even works for the spawning and destruction of entities. The only thing which is no longer predicted is enemy players because any change in enemy inputs always results in a misprediction leading to lots of teleporting and erratic movement. To solve this I added flag which allows any entity's replication type to be swapped between predicted and snapshot. The major game events such as round ending are also delayed to authoritative messages because a misprediction of that wouldn't be acceptable.

Features:
- Pure server authority
- Client side prediction and full rollback for inputs, events, and physics
- Dynamic client update speed to maintain constant input buffer size on server

Other Systems:
- Code hot reloading
- Dynamic layout Imgui
- 2D renderer with draw call batching

## Building
```
build_p.bat build
cd build
build_compile.bat
build.exe
```

## Media
![image](https://github.com/user-attachments/assets/dabf0d4d-973d-4f16-bcd5-efdfe50f9935)

![image](https://github.com/user-attachments/assets/7d57bf70-ab80-4a8a-b3e2-2ab123f5988d)


