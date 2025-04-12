FetchContent_Declare(doctest
	GIT_REPOSITORY https://github.com/doctest/doctest.git
	GIT_TAG origin/master)
FetchContent_GetProperties(doctest)
FetchContent_MakeAvailable(doctest)