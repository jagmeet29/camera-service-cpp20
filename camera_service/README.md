For running the code 
```cmd
env CCACHE_DISABLE=1 cmake -S camera_service -B build
env CCACHE_DISABLE=1 cmake --build build
./build/camera_view
```