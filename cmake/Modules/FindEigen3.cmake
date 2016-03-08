# - Try to find eigen3
# Once done, this will define
#
#  Eigen3_INCLUDE_DIRS    - the Eigen3 include directories
#  Eigen3_LIBRARIES       - link these to use Eigen3

find_package(PkgConfig)

  # find Eigen3 via pkg-config
  # for details see http://www.cmake.org/cmake/help/cmake2.6docs.html#module:FindPkgConfig
pkg_check_modules (Eigen3 REQUIRED eigen3)