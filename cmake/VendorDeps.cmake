# VendorDeps.cmake
#
# Fetches vendored dependencies into third_party/ at *configure* time, then adds
# them to the build as subdirectories.
#
# Always resolves the latest release from https://github.com/OGRECave/ogre/releases
# when TAG is set to "latest" (default).
#
# Options:
#   -DSURA_UPDATE_DEPS=ON -> re-fetch / update dependencies on configure

find_package(Git REQUIRED)

# ---------------------------------------------------------------------------
# sura_resolve_latest_ogre_release(<out_var>)
# ---------------------------------------------------------------------------
# Queries https://api.github.com/repos/OGRECave/ogre/releases/latest or
# git ls-remote to find the latest published release tag.
function(sura_resolve_latest_ogre_release OUT_TAG)
    set(_tag "")
    message(STATUS "Querying latest OGRE release from https://github.com/OGRECave/ogre/releases...")

    # 1. Try GitHub Releases API
    set(_api_file "${CMAKE_BINARY_DIR}/_ogre_latest_release.json")
    file(DOWNLOAD
        "https://api.github.com/repos/OGRECave/ogre/releases/latest"
        "${_api_file}"
        STATUS _dl_status
        TIMEOUT 10
        HTTPHEADER "User-Agent: CMake-OGRE-Fetch"
    )
    list(GET _dl_status 0 _dl_code)
    if(_dl_code EQUAL 0 AND EXISTS "${_api_file}")
        file(READ "${_api_file}" _json_content)
        if(_json_content MATCHES "\"tag_name\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"")
            set(_tag "${CMAKE_MATCH_1}")
            message(STATUS "Latest OGRE release tag resolved from GitHub API: ${_tag}")
        endif()
    endif()

    # 2. Fallback to git ls-remote if GitHub API was unreachable or rate-limited
    if(NOT _tag)
        find_package(Git QUIET)
        if(GIT_EXECUTABLE)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" ls-remote --tags --refs --sort="v:refname" https://github.com/OGRECave/ogre.git
                OUTPUT_VARIABLE _ls_out
                RESULT_VARIABLE _ls_rc
                ERROR_QUIET
            )
            if(_ls_rc EQUAL 0)
                string(REGEX MATCHALL "refs/tags/([^\r\n\t ]+)" _matches "${_ls_out}")
                if(_matches)
                    list(GET _matches -1 _last_ref)
                    string(REPLACE "refs/tags/" "" _tag "${_last_ref}")
                    message(STATUS "Latest OGRE tag resolved via git ls-remote: ${_tag}")
                endif()
            endif()
        endif()
    endif()

    # 3. Fallback to known stable release if completely offline
    if(NOT _tag)
        set(_tag "v14.6.0")
        message(STATUS "Could not query remote; falling back to pinned release tag: ${_tag}")
    endif()

    set(${OUT_TAG} "${_tag}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# sura_resolve_latest_netgen_release(<out_var>)
# ---------------------------------------------------------------------------
function(sura_resolve_latest_netgen_release OUT_TAG)
    set(_tag "")
    message(STATUS "Querying latest Netgen release from https://github.com/NGSolve/netgen/releases...")

    # 1. Try GitHub Releases API
    set(_api_file "${CMAKE_BINARY_DIR}/_netgen_latest_release.json")
    file(DOWNLOAD
        "https://api.github.com/repos/NGSolve/netgen/releases/latest"
        "${_api_file}"
        STATUS _dl_status
        TIMEOUT 10
        HTTPHEADER "User-Agent: CMake-Netgen-Fetch"
    )
    list(GET _dl_status 0 _dl_code)
    if(_dl_code EQUAL 0 AND EXISTS "${_api_file}")
        file(READ "${_api_file}" _json_content)
        if(_json_content MATCHES "\"tag_name\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"")
            set(_tag "${CMAKE_MATCH_1}")
            message(STATUS "Latest Netgen release tag resolved from GitHub API: ${_tag}")
        endif()
    endif()

    # 2. Fallback to git ls-remote
    if(NOT _tag)
        find_package(Git QUIET)
        if(GIT_EXECUTABLE)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" ls-remote --tags --refs --sort="v:refname" https://github.com/NGSolve/netgen.git
                OUTPUT_VARIABLE _ls_out
                RESULT_VARIABLE _ls_rc
                ERROR_QUIET
            )
            if(_ls_rc EQUAL 0)
                string(REGEX MATCHALL "refs/tags/v([^\r\n\t ]+)" _matches "${_ls_out}")
                if(_matches)
                    list(GET _matches -1 _last_ref)
                    string(REPLACE "refs/tags/" "" _tag "${_last_ref}")
                    message(STATUS "Latest Netgen tag resolved via git ls-remote: ${_tag}")
                endif()
            endif()
        endif()
    endif()

    # 3. Fallback to pinned release tag if offline
    if(NOT _tag)
        set(_tag "v6.2.2607")
        message(STATUS "Could not query remote; falling back to pinned release tag: ${_tag}")
    endif()

    set(${OUT_TAG} "${_tag}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# sura_vendor_dependency(<name> REPOSITORY <url> TAG <ref> DIRECTORY <dir>)
# ---------------------------------------------------------------------------
# Clones REPOSITORY at TAG into ${CMAKE_SOURCE_DIR}/third_party/<dir> if that
# directory is not already present, then add_subdirectory()s it.
# If TAG is "latest", it resolves the latest release dynamically.
function(sura_vendor_dependency NAME)
    cmake_parse_arguments(ARG "" "REPOSITORY;TAG;DIRECTORY" "" ${ARGN})

    if(NOT ARG_REPOSITORY OR NOT ARG_TAG OR NOT ARG_DIRECTORY)
        message(FATAL_ERROR
            "sura_vendor_dependency(${NAME}): REPOSITORY, TAG and DIRECTORY are all required.")
    endif()

    set(_dest     "${CMAKE_SOURCE_DIR}/third_party/${ARG_DIRECTORY}")
    set(_bindir   "${CMAKE_BINARY_DIR}/_deps/${ARG_DIRECTORY}-build")
    set(_rel      "third_party/${ARG_DIRECTORY}")

    # If TAG is "latest", dynamically resolve it from the release repository
    if(ARG_TAG STREQUAL "latest")
        if(NAME STREQUAL "OGRE")
            sura_resolve_latest_ogre_release(_resolved_tag)
            set(ARG_TAG "${_resolved_tag}")
        elseif(NAME STREQUAL "NETGEN")
            sura_resolve_latest_netgen_release(_resolved_tag)
            set(ARG_TAG "${_resolved_tag}")
        endif()
    endif()

    if(EXISTS "${_dest}/.git")
        # Check current checkout tag and branch if available
        set(_curr_tag "")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${_dest}" describe --tags --exact-match
            OUTPUT_VARIABLE _curr_tag
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(_curr_branch "")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${_dest}" rev-parse --abbrev-ref HEAD
            OUTPUT_VARIABLE _curr_branch
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        set(_need_update FALSE)
        if(SURA_UPDATE_DEPS)
            set(_need_update TRUE)
        elseif(_curr_tag AND NOT _curr_tag STREQUAL ARG_TAG)
            message(STATUS "Existing ${NAME} tag (${_curr_tag}) differs from target (${ARG_TAG}). Updating...")
            set(_need_update TRUE)
        elseif(_curr_branch AND NOT _curr_branch STREQUAL "HEAD" AND NOT _curr_branch STREQUAL ARG_TAG)
            message(STATUS "Existing ${NAME} branch (${_curr_branch}) differs from target (${ARG_TAG}). Updating...")
            set(_need_update TRUE)
        endif()

        if(_need_update)
            message(STATUS "Updating ${NAME} (${ARG_TAG}) in ${_rel}")
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" -C "${_dest}" fetch --depth 1 origin "${ARG_TAG}"
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
            if(NOT _rc EQUAL 0)
                message(FATAL_ERROR
                    "git fetch failed for ${NAME} in ${_rel}:\n${_err}\n"
                    "Delete ${_rel} and re-configure for a clean clone.")
            endif()
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" -C "${_dest}" checkout --detach FETCH_HEAD
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
            if(NOT _rc EQUAL 0)
                message(FATAL_ERROR "git checkout failed for ${NAME} in ${_rel}:\n${_err}")
            endif()
        else()
            message(STATUS "Using existing ${NAME} (${ARG_TAG}) in ${_rel}")
        endif()

    elseif(EXISTS "${_dest}")
        file(GLOB _entries "${_dest}/*")
        if(_entries)
            message(STATUS "Using existing ${NAME} source tree in ${_rel}")
        else()
            file(REMOVE_RECURSE "${_dest}")
        endif()
    endif()

    if(NOT EXISTS "${_dest}")
        message(STATUS "Cloning ${NAME} ${ARG_TAG} -> ${_rel}")
        if(ARG_TAG MATCHES "^[0-9a-fA-F]{40}$")
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" init "${_dest}"
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" -C "${_dest}" remote add origin "${ARG_REPOSITORY}"
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" -C "${_dest}" fetch --depth 1 origin "${ARG_TAG}"
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" -C "${_dest}" checkout --detach FETCH_HEAD
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
        else()
            execute_process(
                COMMAND       "${GIT_EXECUTABLE}" clone --depth 1 --branch "${ARG_TAG}"
                              --recurse-submodules --shallow-submodules
                              "${ARG_REPOSITORY}" "${_dest}"
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _out
                ERROR_VARIABLE  _err)
        endif()
        if(NOT _rc EQUAL 0)
            file(REMOVE_RECURSE "${_dest}")
            message(FATAL_ERROR
                "git clone failed for ${NAME} (${ARG_TAG}):\n${_err}\n\n"
                "Check network access and that the tag/branch ${ARG_TAG} exists in "
                "${ARG_REPOSITORY}")
        endif()
    endif()

    add_subdirectory("${_dest}" "${_bindir}")
endfunction()
