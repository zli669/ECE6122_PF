TEAM:
Ziwei Li
Yuyang Zhou
Wenrui Zhao


To prepare this program:
# checkout openGL branch 2.1

# replace CMakeLists.txt by:
  cp ${FINAL_PROJ_DIR}/CMakeLists.txt ${OGL_DIR}/

# replace common files by:
  cp ${FINAL_PROJ_DIR}/common/* ${OGL_DIR}common/

# replace source files and add .obj files by:
  cp ${FINAL_PROJ_DIR}/tutorial09_vbo_indexing/* ${OGL_DIR}/tutorial09_vbo_indexing/

To compile:
  mkdir -p ${OGL_DIR}/build
  cd ${OGL_DIR}/build
  cmake ..
  make -j 4

To run:
  ./launch-tutorial09_AssImp.sh

To play:
# player 1: use WSAD for movement and F to fire
# player 2: use IKJL for movement and H to fire
# camera and view: UP, DOWN, LEFT, and RIGHT





