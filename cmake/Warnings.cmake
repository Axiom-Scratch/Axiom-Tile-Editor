if (MSVC)
  set(TE_WARNINGS_STRICT /W4 /WX)
  set(TE_WARNINGS_DEPS /W4)
else()
  set(TE_WARNINGS_STRICT
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Werror
  )
  set(TE_WARNINGS_DEPS
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

function(te_set_warnings target)
  target_compile_options(${target} PRIVATE ${TE_WARNINGS_STRICT})
endfunction()
