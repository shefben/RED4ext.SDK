# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop;F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop")
set(CPACK_CMAKE_GENERATOR "Visual Studio 17 2022")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "C:/Python39/Lib/site-packages/cmake/data/share/cmake-4.0/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "cp2077-coop built using CMake")
set(CPACK_GENERATOR "TGZ")
set(CPACK_INNOSETUP_ARCHITECTURE "x64")
set(CPACK_INSTALL_CMAKE_PROJECTS "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop;cp2077-coop;ALL;/")
set(CPACK_INSTALL_PREFIX "C:/Program Files (x86)/cp2077-coop")
set(CPACK_MODULE_PATH "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop/cmake;F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop/third_party/opus/cmake")
set(CPACK_NSIS_DISPLAY_NAME "cp2077-coop 0.1.1")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_PACKAGE_NAME "cp2077-coop 0.1.1")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OUTPUT_CONFIG_FILE "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop/CPackConfig.cmake")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "C:/Python39/Lib/site-packages/cmake/data/share/cmake-4.0/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "cp2077-coop built using CMake")
set(CPACK_PACKAGE_FILE_NAME "cp2077-coop-0.1.1-win64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "cp2077-coop 0.1.1")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "cp2077-coop 0.1.1")
set(CPACK_PACKAGE_NAME "cp2077-coop")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Humanity")
set(CPACK_PACKAGE_VERSION "0.1.1")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_RESOURCE_FILE_LICENSE "C:/Python39/Lib/site-packages/cmake/data/share/cmake-4.0/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "C:/Python39/Lib/site-packages/cmake/data/share/cmake-4.0/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "C:/Python39/Lib/site-packages/cmake/data/share/cmake-4.0/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_7Z "ON")
set(CPACK_SOURCE_GENERATOR "7Z;ZIP")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop/CPackSourceConfig.cmake")
set(CPACK_SOURCE_ZIP "ON")
set(CPACK_SYSTEM_NAME "win64")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "win64")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "F:/development/steam/emulator_bot/RED4ext.SDK.codex/cp2077-coop/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
