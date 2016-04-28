Kiwano - Configurable Portable Music Player

Work in progress...

Dependencies:
* packages to install `libasound2-dev libfreetype zlib1g-dev flex bison`
* libcue (https://github.com/lipnitsk/libcue)
  compile as static - remove `SHARED` from CMakeLists.txt
  ```cmake
  ADD_LIBRARY(cue ${CUE_SOURCES}
  	${BISON_CueParser_OUTPUTS}
	${FLEX_CueScanner_OUTPUTS})
	```
* taglib (https://github.com/taglib/taglib)