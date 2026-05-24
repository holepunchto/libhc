list(APPEND benches
  append
  append_batched
)

foreach(bench IN LISTS benches)
  add_executable(bench_${bench} ${bench}.c)

  set_target_properties(
    bench_${bench}
    PROPERTIES
    C_STANDARD 99
  )

  target_link_libraries(
    bench_${bench}
    PRIVATE
      hc_static
  )
endforeach()
