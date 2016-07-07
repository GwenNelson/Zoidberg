# PhysicsFS; a portable, flexible file i/o abstraction.
# Copyright (C) 2007  Ryan C. Gordon.
#
# Please see the file LICENSE.txt in the source's root directory.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

PROJECT(PhysicsFS)
SET(PHYSFS_VERSION 2.0.3)

# Increment this if/when we break backwards compatibility.
SET(PHYSFS_SOVERSION 1)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckCSourceCompiles)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/efilibc/inc 
                    ${CMAKE_CURRENT_SOURCE_DIR}/efilibc/efi/inc 
                    ${CMAKE_CURRENT_SOURCE_DIR}/efilibc/efi/inc/protocol
                    ${CMAKE_CURRENT_SOURCE_DIR}/efilibc/efi/inc/x86_64)

#INCLUDE_DIRECTORIES(platform)
#INCLUDE_DIRECTORIES(archivers)


# Add some gcc-specific command lines.
IF(CMAKE_COMPILER_IS_GNUCC)
    # Always build with debug symbols...you can strip it later.
    ADD_DEFINITIONS(-g -pipe -Wall -fsigned-char -ffreestanding)


    CHECK_C_SOURCE_COMPILES("
        #if ((defined(__GNUC__)) && (__GNUC__ >= 4))
        int main(int argc, char **argv) { int is_gcc4 = 1; return 0; }
        #else
        #error This is not gcc4.
        #endif
    " PHYSFS_IS_GCC4)

    IF(PHYSFS_IS_GCC4)
        # Not supported on several operating systems at this time.
        IF(NOT OS2 AND NOT SOLARIS AND NOT WINDOWS)
             ADD_DEFINITIONS(-fvisibility=hidden)
        ENDIF(NOT OS2 AND NOT SOLARIS AND NOT WINDOWS)
    ENDIF(PHYSFS_IS_GCC4)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Basic chunks of source code ...

SET(ZLIB_SRCS
    zlib123/adler32.c
    zlib123/compress.c
    zlib123/crc32.c
    zlib123/deflate.c
    zlib123/gzio.c
    zlib123/infback.c
    zlib123/inffast.c
    zlib123/inflate.c
    zlib123/inftrees.c
    zlib123/trees.c
    zlib123/uncompr.c
    zlib123/zutil.c
)

SET(LZMA_SRCS
    lzma/C/7zCrc.c
    lzma/C/Archive/7z/7zBuffer.c
    lzma/C/Archive/7z/7zDecode.c
    lzma/C/Archive/7z/7zExtract.c
    lzma/C/Archive/7z/7zHeader.c
    lzma/C/Archive/7z/7zIn.c
    lzma/C/Archive/7z/7zItem.c
    lzma/C/Archive/7z/7zMethodID.c
    lzma/C/Compress/Branch/BranchX86.c
    lzma/C/Compress/Branch/BranchX86_2.c
    lzma/C/Compress/Lzma/LzmaDecode.c
)

# Almost everything is "compiled" here, but things that don't apply to the
#  build are #ifdef'd out. This is to make it easy to embed PhysicsFS into
#  another project or bring up a new build system: just compile all the source
#  code and #define the things you want.
SET(PHYSFS_SRCS
    physfs.c
    physfs_byteorder.c
    physfs_unicode.c
    platform/zoidberg.c
    archivers/dir.c
    archivers/grp.c
    archivers/hog.c
    archivers/lzma.c
    archivers/mvl.c
    archivers/qpak.c
    archivers/wad.c
    archivers/zip.c
)


# platform layers ...

# Your threading support is bad, and you should feel bad
SET(PHYSFS_HAVE_THREAD_SUPPORT FALSE)
SET(PHYSFS_HAVE_CDROM_SUPPORT FALSE)
SET(PHYSFS_HAVE_PTHREAD_H FALSE)

IF(NOT PHYSFS_HAVE_CDROM_SUPPORT)
    ADD_DEFINITIONS(-DPHYSFS_NO_CDROM_SUPPORT=1)
    MESSAGE(WARNING " ***")
    MESSAGE(WARNING " *** Zoidberg is fine with this, but the warning is left in:")
    MESSAGE(WARNING " ***")
    MESSAGE(WARNING " *** There is no CD-ROM support in this build!")
    MESSAGE(WARNING " *** PhysicsFS will just pretend there are no discs.")
    MESSAGE(WARNING " *** This may be fine, depending on how PhysicsFS is used,")
    MESSAGE(WARNING " ***   but is this what you REALLY wanted?")
    MESSAGE(WARNING " *** (Maybe fix CMakeLists.txt, or write a platform driver?)")
    MESSAGE(WARNING " ***")
ENDIF(NOT PHYSFS_HAVE_CDROM_SUPPORT)

IF(NOT PHYSFS_HAVE_THREAD_SUPPORT)
    ADD_DEFINITIONS(-DPHYSFS_NO_THREAD_SUPPORT=1)
    MESSAGE(WARNING " ***")
    MESSAGE(WARNING " *** Zoidberg: Your kernel threading support is bad and you should feel bad")
    MESSAGE(WARNING " *** EVerything will still work, but the default warning has been left in:")
    MESSAGE(WARNING " ***")
    MESSAGE(WARNING " *** There is no thread support in this build!")
    MESSAGE(WARNING " *** PhysicsFS will NOT be reentrant!")
    MESSAGE(WARNING " *** This may be fine, depending on how PhysicsFS is used,")
    MESSAGE(WARNING " ***   but is this what you REALLY wanted?")
    MESSAGE(WARNING " *** (Maybe fix CMakeLists.txt, or write a platform driver?)")
    MESSAGE(WARNING " ***")
ENDIF(PHYSFS_HAVE_THREAD_SUPPORT)

CHECK_INCLUDE_FILE(assert.h HAVE_ASSERT_H)
IF(HAVE_ASSERT_H)
    ADD_DEFINITIONS(-DHAVE_ASSERT_H=1)
ENDIF(HAVE_ASSERT_H)



# Archivers ...

OPTION(PHYSFS_ARCHIVE_ZIP "Enable ZIP support" TRUE)
IF(PHYSFS_ARCHIVE_ZIP)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_ZIP=1)
    SET(PHYSFS_NEED_ZLIB TRUE)
ENDIF(PHYSFS_ARCHIVE_ZIP)

OPTION(PHYSFS_ARCHIVE_7Z "Enable 7zip support" FALSE)
IF(PHYSFS_ARCHIVE_7Z)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_7Z=1)
    # !!! FIXME: rename to 7z.c?
    SET(PHYSFS_SRCS ${PHYSFS_SRCS} ${LZMA_SRCS})
ENDIF(PHYSFS_ARCHIVE_7Z)

OPTION(PHYSFS_ARCHIVE_GRP "Enable Build Engine GRP support" FALSE)
IF(PHYSFS_ARCHIVE_GRP)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_GRP=1)
ENDIF(PHYSFS_ARCHIVE_GRP)

OPTION(PHYSFS_ARCHIVE_WAD "Enable Doom WAD support" FALSE)
IF(PHYSFS_ARCHIVE_WAD)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_WAD=1)
ENDIF(PHYSFS_ARCHIVE_WAD)

OPTION(PHYSFS_ARCHIVE_HOG "Enable Descent I/II HOG support" FALSE)
IF(PHYSFS_ARCHIVE_HOG)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_HOG=1)
ENDIF(PHYSFS_ARCHIVE_HOG)

OPTION(PHYSFS_ARCHIVE_MVL "Enable Descent I/II MVL support" FALSE)
IF(PHYSFS_ARCHIVE_MVL)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_MVL=1)
ENDIF(PHYSFS_ARCHIVE_MVL)

OPTION(PHYSFS_ARCHIVE_QPAK "Enable Quake I/II QPAK support" FALSE)
IF(PHYSFS_ARCHIVE_QPAK)
    ADD_DEFINITIONS(-DPHYSFS_SUPPORTS_QPAK=1)
ENDIF(PHYSFS_ARCHIVE_QPAK)


# See if some archiver required zlib, and see about using system version.

OPTION(PHYSFS_INTERNAL_ZLIB "Link own zlib instead of system library" TRUE)

IF(PHYSFS_NEED_ZLIB)

    SET(PHYSFS_INTERNAL_ZLIB TRUE)

    IF(PHYSFS_INTERNAL_ZLIB)
        INCLUDE_DIRECTORIES(zlib123)
        ADD_DEFINITIONS(-DZ_PREFIX=1)
        SET(PHYSFS_SRCS ${PHYSFS_SRCS} ${ZLIB_SRCS})
    ELSE(PHYSFS_INTERNAL_ZLIB)
        SET(OPTIONAL_LIBRARY_LIBS ${OPTIONAL_LIBRARY_LIBS} ${ZLIB_LIBRARY})
        INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
    ENDIF(PHYSFS_INTERNAL_ZLIB)
ENDIF(PHYSFS_NEED_ZLIB)

OPTION(PHYSFS_BUILD_STATIC "Build static library" TRUE)
IF(PHYSFS_BUILD_STATIC)
    ADD_LIBRARY(physfs-static STATIC ${PHYSFS_SRCS})
    SET_TARGET_PROPERTIES(physfs-static PROPERTIES OUTPUT_NAME "physfs")
    SET(PHYSFS_LIB_TARGET physfs-static)
    SET(PHYSFS_INSTALL_TARGETS ${PHYSFS_INSTALL_TARGETS} ";physfs-static")
ENDIF(PHYSFS_BUILD_STATIC)

OPTION(PHYSFS_BUILD_SHARED "Build shared library" FALSE)
IF(PHYSFS_BUILD_SHARED)
    ADD_LIBRARY(physfs SHARED ${PHYSFS_SRCS})
    SET_TARGET_PROPERTIES(physfs PROPERTIES VERSION ${PHYSFS_VERSION})
    SET_TARGET_PROPERTIES(physfs PROPERTIES SOVERSION ${PHYSFS_SOVERSION})
    TARGET_LINK_LIBRARIES(physfs ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    SET(PHYSFS_LIB_TARGET physfs)
    SET(PHYSFS_INSTALL_TARGETS ${PHYSFS_INSTALL_TARGETS} ";physfs")
ENDIF(PHYSFS_BUILD_SHARED)

IF(NOT PHYSFS_BUILD_SHARED AND NOT PHYSFS_BUILD_STATIC)
    MESSAGE(FATAL "Both shared and static libraries are disabled!")
ENDIF(NOT PHYSFS_BUILD_SHARED AND NOT PHYSFS_BUILD_STATIC)

# CMake FAQ says I need this...
IF(PHYSFS_BUILD_SHARED AND PHYSFS_BUILD_STATIC)
    SET_TARGET_PROPERTIES(physfs PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    SET_TARGET_PROPERTIES(physfs-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
ENDIF(PHYSFS_BUILD_SHARED AND PHYSFS_BUILD_STATIC)

OPTION(PHYSFS_BUILD_TEST "Build stdio test program." FALSE)
MARK_AS_ADVANCED(PHYSFS_BUILD_TEST)
IF(PHYSFS_BUILD_TEST)
    FIND_PATH(READLINE_H readline/readline.h)
    FIND_PATH(HISTORY_H readline/history.h)
    IF(READLINE_H AND HISTORY_H)
        FIND_LIBRARY(CURSES_LIBRARY NAMES curses ncurses)
        SET(CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARY})
        FIND_LIBRARY(READLINE_LIBRARY readline)
        IF(READLINE_LIBRARY)
            SET(HAVE_SYSTEM_READLINE TRUE)
            SET(TEST_PHYSFS_LIBS ${TEST_PHYSFS_LIBS} ${READLINE_LIBRARY} ${CURSES_LIBRARY})
            INCLUDE_DIRECTORIES(${READLINE_H} ${HISTORY_H})
            ADD_DEFINITIONS(-DPHYSFS_HAVE_READLINE=1)
        ENDIF(READLINE_LIBRARY)
    ENDIF(READLINE_H AND HISTORY_H)
    ADD_EXECUTABLE(test_physfs test/test_physfs.c)
    TARGET_LINK_LIBRARIES(test_physfs ${PHYSFS_LIB_TARGET} ${TEST_PHYSFS_LIBS} ${OTHER_LDFLAGS})
    SET(PHYSFS_INSTALL_TARGETS ${PHYSFS_INSTALL_TARGETS} ";test_physfs")
ENDIF(PHYSFS_BUILD_TEST)

OPTION(PHYSFS_BUILD_WX_TEST "Build wxWidgets test program." FALSE)
MARK_AS_ADVANCED(PHYSFS_BUILD_WX_TEST)
IF(PHYSFS_BUILD_WX_TEST)
    SET(wxWidgets_USE_LIBS base core adv)
    SET(wxWidgets_INCLUDE_DIRS_NO_SYSTEM 1)
    FIND_PACKAGE(wxWidgets)
    IF(wxWidgets_FOUND)
        INCLUDE(${wxWidgets_USE_FILE})
        ADD_EXECUTABLE(wxtest_physfs test/wxtest_physfs.cpp)
        SET_SOURCE_FILES_PROPERTIES(test/wxtest_physfs.cpp COMPILE_FLAGS ${wxWidgets_CXX_FLAGS})
        TARGET_LINK_LIBRARIES(wxtest_physfs ${PHYSFS_LIB_TARGET} ${wxWidgets_LIBRARIES} ${OTHER_LDFLAGS})
        SET(PHYSFS_INSTALL_TARGETS ${PHYSFS_INSTALL_TARGETS} ";wxtest_physfs")
    ELSE(wxWidgets_FOUND)
        MESSAGE(STATUS "wxWidgets not found. Disabling wx test app.")
        SET(PHYSFS_BUILD_WX_TEST FALSE)
    ENDIF(wxWidgets_FOUND)
ENDIF(PHYSFS_BUILD_WX_TEST)

INSTALL(TARGETS ${PHYSFS_INSTALL_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
INSTALL(FILES physfs.h DESTINATION include)

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    SET(PHYSFS_OUTPUT_DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
        "${PHYSFS_OUTPUT_DOXYFILE}"
        COPYONLY
    )
    FILE(APPEND "${PHYSFS_OUTPUT_DOXYFILE}" "\n\n# Below auto-generated by cmake...\n\n")
    FILE(APPEND "${PHYSFS_OUTPUT_DOXYFILE}" "PROJECT_NUMBER = ${PHYSFS_VERSION}\n")
    FILE(APPEND "${PHYSFS_OUTPUT_DOXYFILE}" "OUTPUT_DIRECTORY = ${CMAKE_CURRENT_BINARY_DIR}/docs\n")
    FILE(APPEND "${PHYSFS_OUTPUT_DOXYFILE}" "\n# End auto-generated section.\n\n")

    ADD_CUSTOM_TARGET(
        docs
        ${DOXYGEN_EXECUTABLE} "${PHYSFS_OUTPUT_DOXYFILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building documentation in 'docs' directory..."
    )
ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "Doxygen not found. You won't be able to build documentation.")
ENDIF(DOXYGEN_FOUND)

IF(UNIX)
    SET(PHYSFS_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/../physfs-${PHYSFS_VERSION}.tar.bz2")
    ADD_CUSTOM_TARGET(
        dist
        hg archive -t tbz2 "${PHYSFS_TARBALL}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building source tarball '${PHYSFS_TARBALL}'..."
    )
ENDIF(UNIX)

MACRO(MESSAGE_BOOL_OPTION _NAME _VALUE)
    IF(${_VALUE})
        MESSAGE(STATUS "  ${_NAME}: enabled")
    ELSE(${_VALUE})
        MESSAGE(STATUS "  ${_NAME}: disabled")
    ENDIF(${_VALUE})
ENDMACRO(MESSAGE_BOOL_OPTION)

MESSAGE(STATUS "PhysicsFS will build with the following options:")
MESSAGE_BOOL_OPTION("ZIP support" PHYSFS_ARCHIVE_ZIP)
MESSAGE_BOOL_OPTION("7zip support" PHYSFS_ARCHIVE_7Z)
MESSAGE_BOOL_OPTION("GRP support" PHYSFS_ARCHIVE_GRP)
MESSAGE_BOOL_OPTION("WAD support" PHYSFS_ARCHIVE_WAD)
MESSAGE_BOOL_OPTION("HOG support" PHYSFS_ARCHIVE_HOG)
MESSAGE_BOOL_OPTION("MVL support" PHYSFS_ARCHIVE_MVL)
MESSAGE_BOOL_OPTION("QPAK support" PHYSFS_ARCHIVE_QPAK)
MESSAGE_BOOL_OPTION("CD-ROM drive support" PHYSFS_HAVE_CDROM_SUPPORT)
MESSAGE_BOOL_OPTION("Thread safety" PHYSFS_HAVE_THREAD_SUPPORT)
MESSAGE_BOOL_OPTION("Build own zlib" PHYSFS_INTERNAL_ZLIB)
MESSAGE_BOOL_OPTION("Build static library" PHYSFS_BUILD_STATIC)
MESSAGE_BOOL_OPTION("Build shared library" PHYSFS_BUILD_SHARED)
MESSAGE_BOOL_OPTION("Build wxWidgets test program" PHYSFS_BUILD_WX_TEST)
MESSAGE_BOOL_OPTION("Build stdio test program" PHYSFS_BUILD_TEST)
IF(PHYSFS_BUILD_TEST)
    MESSAGE_BOOL_OPTION("  Use readline in test program" HAVE_SYSTEM_READLINE)
ENDIF(PHYSFS_BUILD_TEST)

# end of CMakeLists.txt ...
