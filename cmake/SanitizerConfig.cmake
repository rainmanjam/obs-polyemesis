# Sanitizer Configuration for obs-polyemesis
# Provides AddressSanitizer, UndefinedBehaviorSanitizer, and ThreadSanitizer options

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)

# Function to add sanitizer flags
function(add_sanitizer_flags target)
  if(ENABLE_ASAN)
    message(STATUS "Enabling AddressSanitizer for ${target}")
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=address)
  endif()

  if(ENABLE_UBSAN)
    message(STATUS "Enabling UndefinedBehaviorSanitizer for ${target}")
    target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=undefined)
  endif()

  if(ENABLE_TSAN)
    message(STATUS "Enabling ThreadSanitizer for ${target}")
    target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=thread)
  endif()

  if(ENABLE_MSAN)
    message(STATUS "Enabling MemorySanitizer for ${target}")
    target_compile_options(${target} PRIVATE -fsanitize=memory -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=memory)
  endif()

  # Add debug symbols for better stack traces
  if(ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_TSAN OR ENABLE_MSAN)
    target_compile_options(${target} PRIVATE -g -O1)
  endif()
endfunction()
