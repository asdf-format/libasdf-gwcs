include(AddressAnalyzer)

option(ENABLE_TESTING "Enable unit tests" OFF)
option(ASDF_GWCS_ENABLE_AST "Enable AST evaluation backend" ON)

set(CPACK_PACKAGE_VENDOR "STScI")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
        \\.git/
        \\.github/
        \\.idea/
        "cmake-.*/"
        build/
        ".*~$"
)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
