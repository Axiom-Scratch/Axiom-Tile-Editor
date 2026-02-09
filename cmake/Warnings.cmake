function(te_set_warnings target)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wshadow
      -Wconversion
      -Wsign-conversion
    )
  endif()
endfunction()
