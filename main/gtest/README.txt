
On your system (one time install of 'gtest'): 

1. Install package "libgtest-dev" 
2. Compile from /usr/src/gtest  (cmake/make) 
3. Copy two gtest library files into /usr/lib  
      (cp /usr/src/gtest/lib/libgtest*.a /usr/lib) 

In this workspace: 

1. cd to main/gtest 
2. cmake .  / make   (creates led-test executable) 
3. ./led-test

NOTE:  "led-test" can now be run with -i option, which
       will start an interactive simulation of charging
       animation.

