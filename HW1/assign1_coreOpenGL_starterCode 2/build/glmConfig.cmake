set(GLM_VERSION "0.9.7")
set(GLM_INCLUDE_DIRS "/Users/davidwang/Documents/CSCI420/HW1/assign1_coreOpenGL_starterCode/external/glm")

if (NOT CMAKE_VERSION VERSION_LESS "3.0")
    include("${CMAKE_CURRENT_LIST_DIR}/glmTargets.cmake")
endif()
