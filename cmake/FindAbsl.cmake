# Finds the Abseil (absl) libraries in the system.
# Custom-built/"copied-then-modded" from the original CMake config files
# (abslConfig.cmake, abslTargets.cmake, abslTargets-none.cmake).
# Those are hardcoded to link SHARED libraries, with no way to toggle to STATIC.
# This one links STATIC by default, and can be toggled with `set(ABSL_FIND_SHARED ON)`.
# WORKS ON LINUX PATHS ONLY. I won't bother porting to other systems.

# Set up prefixes/suffixes and lib type
set(ABSL_INCLUDE_DIR "/usr/include")
set(ABSL_PATH_PREFIX "/usr/lib/x86_64-linux-gnu/")
if(ABSL_FIND_SHARED)
  set(ABSL_LIB_TYPE SHARED)
  set(ABSL_LIB_SUFFIX ".so")
else()
  set(ABSL_LIB_TYPE STATIC)
  set(ABSL_LIB_SUFFIX ".a")
endif()

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
set(_targetsDefined)
set(_targetsNotDefined)
set(_expectedTargets)
foreach(_expectedTarget absl::atomic_hook absl::errno_saver absl::log_severity absl::raw_logging_internal absl::spinlock_wait absl::config absl::dynamic_annotations absl::core_headers absl::malloc_internal absl::base_internal absl::base absl::throw_delegate absl::pretty_function absl::endian absl::bits absl::exponential_biased absl::periodic_sampler absl::scoped_set_env absl::strerror absl::fast_type_id absl::algorithm absl::algorithm_container absl::container absl::btree absl::compressed_tuple absl::fixed_array absl::inlined_vector_internal absl::inlined_vector absl::counting_allocator absl::flat_hash_map absl::flat_hash_set absl::node_hash_map absl::node_hash_set absl::container_memory absl::hash_function_defaults absl::hash_policy_traits absl::hashtablez_sampler absl::hashtable_debug absl::hashtable_debug_hooks absl::have_sse absl::node_hash_policy absl::raw_hash_map absl::container_common absl::raw_hash_set absl::layout absl::stacktrace absl::symbolize absl::examine_stack absl::failure_signal_handler absl::debugging_internal absl::demangle_internal absl::leak_check absl::leak_check_disable absl::debugging absl::flags_path_util absl::flags_program_name absl::flags_config absl::flags_marshalling absl::flags_commandlineflag_internal absl::flags_commandlineflag absl::flags_private_handle_accessor absl::flags_reflection absl::flags_internal absl::flags absl::flags_usage_internal absl::flags_usage absl::flags_parse absl::bind_front absl::function_ref absl::hash absl::city absl::memory absl::type_traits absl::meta absl::int128 absl::numeric absl::random_random absl::random_bit_gen_ref absl::random_internal_mock_helpers absl::random_distributions absl::random_seed_gen_exception absl::random_seed_sequences absl::random_internal_traits absl::random_internal_distribution_caller absl::random_internal_fast_uniform_bits absl::random_internal_seed_material absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_iostream_state_saver absl::random_internal_generate_real absl::random_internal_wide_multiply absl::random_internal_fastmath absl::random_internal_nonsecure_base absl::random_internal_pcg_engine absl::random_internal_randen_engine absl::random_internal_platform absl::random_internal_randen absl::random_internal_randen_slow absl::random_internal_randen_hwaes absl::random_internal_randen_hwaes_impl absl::random_internal_distribution_test_util absl::random_internal_uniform_helper absl::status absl::statusor absl::strings absl::strings_internal absl::str_format absl::str_format_internal absl::cord absl::graphcycles_internal absl::kernel_timeout_internal absl::synchronization absl::time absl::civil_time absl::time_zone absl::any absl::bad_any_cast absl::bad_any_cast_impl absl::span absl::optional absl::bad_optional_access absl::bad_variant_access absl::variant absl::compare absl::utility)
  list(APPEND _expectedTargets ${_expectedTarget})
  if(NOT TARGET ${_expectedTarget})
    list(APPEND _targetsNotDefined ${_expectedTarget})
  endif()
  if(TARGET ${_expectedTarget})
    list(APPEND _targetsDefined ${_expectedTarget})
  endif()
endforeach()
if("${_targetsDefined}" STREQUAL "${_expectedTargets}")
  unset(_targetsDefined)
  unset(_targetsNotDefined)
  unset(_expectedTargets)
  set(CMAKE_IMPORT_FILE_VERSION)
  cmake_policy(POP)
  return()
endif()
if(NOT "${_targetsDefined}" STREQUAL "")
  message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${_targetsDefined}\nTargets not yet defined: ${_targetsNotDefined}\n")
endif()
unset(_targetsDefined)
unset(_targetsNotDefined)
unset(_expectedTargets)

# Create imported target absl::atomic_hook
add_library(absl::atomic_hook INTERFACE IMPORTED)

set_target_properties(absl::atomic_hook PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::errno_saver
add_library(absl::errno_saver INTERFACE IMPORTED)

set_target_properties(absl::errno_saver PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::log_severity
add_library(absl::log_severity ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::log_severity PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_log_severity${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_log_severity${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::raw_logging_internal
add_library(absl::raw_logging_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::raw_logging_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::atomic_hook;absl::config;absl::core_headers;absl::log_severity"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_raw_logging_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_raw_logging_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::spinlock_wait
add_library(absl::spinlock_wait ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::spinlock_wait PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base_internal;absl::core_headers;absl::errno_saver"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_spinlock_wait${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_spinlock_wait${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::config
add_library(absl::config INTERFACE IMPORTED)

set_target_properties(absl::config PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::dynamic_annotations
add_library(absl::dynamic_annotations INTERFACE IMPORTED)

set_target_properties(absl::dynamic_annotations PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::core_headers
add_library(absl::core_headers INTERFACE IMPORTED)

set_target_properties(absl::core_headers PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::malloc_internal
add_library(absl::malloc_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::malloc_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::raw_logging_internal;Threads::Threads"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_malloc_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_malloc_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::base_internal
add_library(absl::base_internal INTERFACE IMPORTED)

set_target_properties(absl::base_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::base
add_library(absl::base ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::base PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::atomic_hook;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::log_severity;absl::raw_logging_internal;absl::spinlock_wait;absl::type_traits;Threads::Threads"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_base${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_base${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::throw_delegate
add_library(absl::throw_delegate ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::throw_delegate PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::raw_logging_internal"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_throw_delegate${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_throw_delegate${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::pretty_function
add_library(absl::pretty_function INTERFACE IMPORTED)

set_target_properties(absl::pretty_function PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::endian
add_library(absl::endian INTERFACE IMPORTED)

set_target_properties(absl::endian PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::config;absl::core_headers;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::bits
add_library(absl::bits INTERFACE IMPORTED)

set_target_properties(absl::bits PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::exponential_biased
add_library(absl::exponential_biased ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::exponential_biased PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_exponential_biased${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_exponential_biased${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::periodic_sampler
add_library(absl::periodic_sampler ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::periodic_sampler PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::exponential_biased"
)

# Create imported target absl::scoped_set_env
add_library(absl::scoped_set_env ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::scoped_set_env PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::raw_logging_internal"
)

# Create imported target absl::strerror
add_library(absl::strerror ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::strerror PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::errno_saver"
)

# Create imported target absl::fast_type_id
add_library(absl::fast_type_id INTERFACE IMPORTED)

set_target_properties(absl::fast_type_id PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::algorithm
add_library(absl::algorithm INTERFACE IMPORTED)

set_target_properties(absl::algorithm PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::algorithm_container
add_library(absl::algorithm_container INTERFACE IMPORTED)

set_target_properties(absl::algorithm_container PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::algorithm;absl::core_headers;absl::meta;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::container
add_library(absl::container INTERFACE IMPORTED)

set_target_properties(absl::container PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::btree
add_library(absl::btree INTERFACE IMPORTED)

set_target_properties(absl::btree PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::container_common;absl::compare;absl::compressed_tuple;absl::container_memory;absl::cord;absl::core_headers;absl::layout;absl::memory;absl::strings;absl::throw_delegate;absl::type_traits;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::compressed_tuple
add_library(absl::compressed_tuple INTERFACE IMPORTED)

set_target_properties(absl::compressed_tuple PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::fixed_array
add_library(absl::fixed_array INTERFACE IMPORTED)

set_target_properties(absl::fixed_array PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::compressed_tuple;absl::algorithm;absl::config;absl::core_headers;absl::dynamic_annotations;absl::throw_delegate;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::inlined_vector_internal
add_library(absl::inlined_vector_internal INTERFACE IMPORTED)

set_target_properties(absl::inlined_vector_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::compressed_tuple;absl::core_headers;absl::memory;absl::span;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::inlined_vector
add_library(absl::inlined_vector INTERFACE IMPORTED)

set_target_properties(absl::inlined_vector PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::algorithm;absl::core_headers;absl::inlined_vector_internal;absl::throw_delegate;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::counting_allocator
add_library(absl::counting_allocator INTERFACE IMPORTED)

set_target_properties(absl::counting_allocator PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::flat_hash_map
add_library(absl::flat_hash_map INTERFACE IMPORTED)

set_target_properties(absl::flat_hash_map PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::container_memory;absl::hash_function_defaults;absl::raw_hash_map;absl::algorithm_container;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::flat_hash_set
add_library(absl::flat_hash_set INTERFACE IMPORTED)

set_target_properties(absl::flat_hash_set PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::container_memory;absl::hash_function_defaults;absl::raw_hash_set;absl::algorithm_container;absl::core_headers;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::node_hash_map
add_library(absl::node_hash_map INTERFACE IMPORTED)

set_target_properties(absl::node_hash_map PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::container_memory;absl::hash_function_defaults;absl::node_hash_policy;absl::raw_hash_map;absl::algorithm_container;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::node_hash_set
add_library(absl::node_hash_set INTERFACE IMPORTED)

set_target_properties(absl::node_hash_set PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::hash_function_defaults;absl::node_hash_policy;absl::raw_hash_set;absl::algorithm_container;absl::memory;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::container_memory
add_library(absl::container_memory INTERFACE IMPORTED)

set_target_properties(absl::container_memory PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::memory;absl::type_traits;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::hash_function_defaults
add_library(absl::hash_function_defaults INTERFACE IMPORTED)

set_target_properties(absl::hash_function_defaults PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::cord;absl::hash;absl::strings;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::hash_policy_traits
add_library(absl::hash_policy_traits INTERFACE IMPORTED)

set_target_properties(absl::hash_policy_traits PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::meta;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::hashtablez_sampler
add_library(absl::hashtablez_sampler ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::hashtablez_sampler PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::exponential_biased;absl::have_sse;absl::synchronization"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_hashtablez_sampler${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_hashtablez_sampler${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::hashtable_debug
add_library(absl::hashtable_debug INTERFACE IMPORTED)

set_target_properties(absl::hashtable_debug PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::hashtable_debug_hooks;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::hashtable_debug_hooks
add_library(absl::hashtable_debug_hooks INTERFACE IMPORTED)

set_target_properties(absl::hashtable_debug_hooks PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::have_sse
add_library(absl::have_sse INTERFACE IMPORTED)

set_target_properties(absl::have_sse PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::node_hash_policy
add_library(absl::node_hash_policy INTERFACE IMPORTED)

set_target_properties(absl::node_hash_policy PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::raw_hash_map
add_library(absl::raw_hash_map INTERFACE IMPORTED)

set_target_properties(absl::raw_hash_map PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::container_memory;absl::raw_hash_set;absl::throw_delegate;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::container_common
add_library(absl::container_common INTERFACE IMPORTED)

set_target_properties(absl::container_common PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::raw_hash_set
add_library(absl::raw_hash_set ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::raw_hash_set PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;absl::compressed_tuple;absl::config;absl::container_common;absl::container_memory;absl::core_headers;absl::endian;absl::hash_policy_traits;absl::hashtable_debug_hooks;absl::have_sse;absl::layout;absl::memory;absl::meta;absl::optional;absl::utility;absl::hashtablez_sampler"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_raw_hash_set${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_raw_hash_set${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::layout
add_library(absl::layout INTERFACE IMPORTED)

set_target_properties(absl::layout PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::meta;absl::strings;absl::span;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::stacktrace
add_library(absl::stacktrace ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::stacktrace PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::debugging_internal;absl::config;absl::core_headers"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_stacktrace${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_stacktrace${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::symbolize
add_library(absl::symbolize ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::symbolize PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::debugging_internal;absl::demangle_internal;absl::base;absl::config;absl::core_headers;absl::dynamic_annotations;absl::malloc_internal;absl::raw_logging_internal;absl::strings"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_symbolize${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_symbolize${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::examine_stack
add_library(absl::examine_stack ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::examine_stack PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::stacktrace;absl::symbolize;absl::config;absl::core_headers;absl::raw_logging_internal"
)

# Create imported target absl::failure_signal_handler
add_library(absl::failure_signal_handler ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::failure_signal_handler PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::examine_stack;absl::stacktrace;absl::base;absl::config;absl::core_headers;absl::errno_saver;absl::raw_logging_internal"
)

# Create imported target absl::debugging_internal
add_library(absl::debugging_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::debugging_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::config;absl::dynamic_annotations;absl::errno_saver;absl::raw_logging_internal"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_debugging_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_debugging_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::demangle_internal
add_library(absl::demangle_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::demangle_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::core_headers"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_demangle_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_demangle_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::leak_check
add_library(absl::leak_check ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::leak_check PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers"
)

# Create imported target absl::leak_check_disable
add_library(absl::leak_check_disable ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::leak_check_disable PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
)

# Create imported target absl::debugging
add_library(absl::debugging INTERFACE IMPORTED)

set_target_properties(absl::debugging PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::stacktrace;absl::leak_check;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::flags_path_util
add_library(absl::flags_path_util INTERFACE IMPORTED)

set_target_properties(absl::flags_path_util PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::strings;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::flags_program_name
add_library(absl::flags_program_name ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_program_name PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::flags_path_util;absl::strings;absl::synchronization"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_program_name${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_program_name${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_config
add_library(absl::flags_config ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_config PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::flags_path_util;absl::flags_program_name;absl::core_headers;absl::strings;absl::synchronization"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_config${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_config${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_marshalling
add_library(absl::flags_marshalling ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_marshalling PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::log_severity;absl::strings;absl::str_format"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_marshalling${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_marshalling${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_commandlineflag_internal
add_library(absl::flags_commandlineflag_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_commandlineflag_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::fast_type_id"
  IMPORTED_LOCATION
  "${ABSL_PATH_PREFIX}libabsl_flags_commandlineflag_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_commandlineflag_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_commandlineflag
add_library(absl::flags_commandlineflag ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_commandlineflag PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::fast_type_id;absl::flags_commandlineflag_internal;absl::optional;absl::strings"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_commandlineflag${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_commandlineflag${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_private_handle_accessor
add_library(absl::flags_private_handle_accessor ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_private_handle_accessor PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::strings"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_private_handle_accessor${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_private_handle_accessor${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_reflection
add_library(absl::flags_reflection ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_reflection PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::flags_commandlineflag;absl::flags_private_handle_accessor;absl::flags_config;absl::strings;absl::synchronization;absl::flat_hash_map"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_reflection${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_reflection${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_internal
add_library(absl::flags_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::config;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::flags_config;absl::flags_marshalling;absl::synchronization;absl::meta;absl::utility"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags
add_library(absl::flags ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::flags_commandlineflag;absl::flags_config;absl::flags_internal;absl::flags_reflection;absl::base;absl::core_headers;absl::strings"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_flags${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_flags${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::flags_usage_internal
add_library(absl::flags_usage_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_usage_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::flags_config;absl::flags;absl::flags_commandlineflag;absl::flags_internal;absl::flags_path_util;absl::flags_private_handle_accessor;absl::flags_program_name;absl::flags_reflection;absl::strings;absl::synchronization"
)

# Create imported target absl::flags_usage
add_library(absl::flags_usage ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_usage PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::flags_usage_internal;absl::strings;absl::synchronization"
)

# Create imported target absl::flags_parse
add_library(absl::flags_parse ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::flags_parse PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::flags_config;absl::flags;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::flags_internal;absl::flags_private_handle_accessor;absl::flags_program_name;absl::flags_reflection;absl::flags_usage;absl::strings;absl::synchronization"
)

# Create imported target absl::bind_front
add_library(absl::bind_front INTERFACE IMPORTED)

set_target_properties(absl::bind_front PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base_internal;absl::compressed_tuple;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::function_ref
add_library(absl::function_ref INTERFACE IMPORTED)

set_target_properties(absl::function_ref PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base_internal;absl::meta;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::hash
add_library(absl::hash ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::hash PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::endian;absl::fixed_array;absl::meta;absl::int128;absl::strings;absl::optional;absl::variant;absl::utility;absl::city"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_hash${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_hash${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::city
add_library(absl::city ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::city PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::endian"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_city${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_city${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::memory
add_library(absl::memory INTERFACE IMPORTED)

set_target_properties(absl::memory PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::meta;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::type_traits
add_library(absl::type_traits INTERFACE IMPORTED)

set_target_properties(absl::type_traits PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::meta
add_library(absl::meta INTERFACE IMPORTED)

set_target_properties(absl::meta PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::int128
add_library(absl::int128 ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::int128 PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;absl::config;absl::core_headers"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_int128${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_int128${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::numeric
add_library(absl::numeric INTERFACE IMPORTED)

set_target_properties(absl::numeric PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::int128;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_random
add_library(absl::random_random INTERFACE IMPORTED)

set_target_properties(absl::random_random PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::random_distributions;absl::random_internal_nonsecure_base;absl::random_internal_pcg_engine;absl::random_internal_pool_urbg;absl::random_internal_randen_engine;absl::random_seed_sequences;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_bit_gen_ref
add_library(absl::random_bit_gen_ref INTERFACE IMPORTED)

set_target_properties(absl::random_bit_gen_ref PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::random_internal_distribution_caller;absl::random_internal_fast_uniform_bits;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_mock_helpers
add_library(absl::random_internal_mock_helpers INTERFACE IMPORTED)

set_target_properties(absl::random_internal_mock_helpers PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::fast_type_id;absl::optional;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_distributions
add_library(absl::random_distributions ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_distributions PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base_internal;absl::config;absl::core_headers;absl::random_internal_generate_real;absl::random_internal_distribution_caller;absl::random_internal_fast_uniform_bits;absl::random_internal_fastmath;absl::random_internal_iostream_state_saver;absl::random_internal_traits;absl::random_internal_uniform_helper;absl::random_internal_wide_multiply;absl::strings;absl::type_traits"
)

# Create imported target absl::random_seed_gen_exception
add_library(absl::random_seed_gen_exception ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_seed_gen_exception PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config"
)

# Create imported target absl::random_seed_sequences
add_library(absl::random_seed_sequences ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_seed_sequences PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::inlined_vector;absl::random_internal_nonsecure_base;absl::random_internal_pool_urbg;absl::random_internal_salted_seed_seq;absl::random_internal_seed_material;absl::random_seed_gen_exception;absl::span"
)

# Create imported target absl::random_internal_traits
add_library(absl::random_internal_traits INTERFACE IMPORTED)

set_target_properties(absl::random_internal_traits PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_distribution_caller
add_library(absl::random_internal_distribution_caller INTERFACE IMPORTED)

set_target_properties(absl::random_internal_distribution_caller PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::utility;absl::fast_type_id;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_fast_uniform_bits
add_library(absl::random_internal_fast_uniform_bits INTERFACE IMPORTED)

set_target_properties(absl::random_internal_fast_uniform_bits PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_seed_material
add_library(absl::random_internal_seed_material ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_seed_material PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::optional;absl::random_internal_fast_uniform_bits;absl::raw_logging_internal;absl::span;absl::strings"
)

# Create imported target absl::random_internal_pool_urbg
add_library(absl::random_internal_pool_urbg ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_pool_urbg PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::config;absl::core_headers;absl::endian;absl::random_internal_randen;absl::random_internal_seed_material;absl::random_internal_traits;absl::random_seed_gen_exception;absl::raw_logging_internal;absl::span"
)

# Create imported target absl::random_internal_salted_seed_seq
add_library(absl::random_internal_salted_seed_seq INTERFACE IMPORTED)

set_target_properties(absl::random_internal_salted_seed_seq PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::inlined_vector;absl::optional;absl::span;absl::random_internal_seed_material;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_iostream_state_saver
add_library(absl::random_internal_iostream_state_saver INTERFACE IMPORTED)

set_target_properties(absl::random_internal_iostream_state_saver PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::int128;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_generate_real
add_library(absl::random_internal_generate_real INTERFACE IMPORTED)

set_target_properties(absl::random_internal_generate_real PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;absl::random_internal_fastmath;absl::random_internal_traits;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_wide_multiply
add_library(absl::random_internal_wide_multiply INTERFACE IMPORTED)

set_target_properties(absl::random_internal_wide_multiply PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;absl::config;absl::int128;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_fastmath
add_library(absl::random_internal_fastmath INTERFACE IMPORTED)

set_target_properties(absl::random_internal_fastmath PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_nonsecure_base
add_library(absl::random_internal_nonsecure_base INTERFACE IMPORTED)

set_target_properties(absl::random_internal_nonsecure_base PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::optional;absl::random_internal_pool_urbg;absl::random_internal_salted_seed_seq;absl::random_internal_seed_material;absl::span;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_pcg_engine
add_library(absl::random_internal_pcg_engine INTERFACE IMPORTED)

set_target_properties(absl::random_internal_pcg_engine PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::int128;absl::random_internal_fastmath;absl::random_internal_iostream_state_saver;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_randen_engine
add_library(absl::random_internal_randen_engine INTERFACE IMPORTED)

set_target_properties(absl::random_internal_randen_engine PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::endian;absl::random_internal_iostream_state_saver;absl::random_internal_randen;absl::raw_logging_internal;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::random_internal_platform
add_library(absl::random_internal_platform ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_platform PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config"
)

# Create imported target absl::random_internal_randen
add_library(absl::random_internal_randen ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_randen PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::random_internal_platform;absl::random_internal_randen_hwaes;absl::random_internal_randen_slow"
)

# Create imported target absl::random_internal_randen_slow
add_library(absl::random_internal_randen_slow ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_randen_slow PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::random_internal_platform;absl::config"
)

# Create imported target absl::random_internal_randen_hwaes
add_library(absl::random_internal_randen_hwaes ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_randen_hwaes PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::random_internal_platform;absl::random_internal_randen_hwaes_impl;absl::config"
)

# Create imported target absl::random_internal_randen_hwaes_impl
add_library(absl::random_internal_randen_hwaes_impl ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_randen_hwaes_impl PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::random_internal_platform;absl::config"
)

# Create imported target absl::random_internal_distribution_test_util
add_library(absl::random_internal_distribution_test_util ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::random_internal_distribution_test_util PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::raw_logging_internal;absl::strings;absl::str_format;absl::span"
)

# Create imported target absl::random_internal_uniform_helper
add_library(absl::random_internal_uniform_helper INTERFACE IMPORTED)

set_target_properties(absl::random_internal_uniform_helper PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::random_internal_traits;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::status
add_library(absl::status ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::status PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::atomic_hook;absl::config;absl::core_headers;absl::raw_logging_internal;absl::inlined_vector;absl::stacktrace;absl::symbolize;absl::strings;absl::cord;absl::str_format;absl::optional"
)

# Create imported target absl::statusor
add_library(absl::statusor ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::statusor PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::status;absl::core_headers;absl::raw_logging_internal;absl::type_traits;absl::strings;absl::utility;absl::variant"
)

# Create imported target absl::strings
add_library(absl::strings ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::strings PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::strings_internal;absl::base;absl::bits;absl::config;absl::core_headers;absl::endian;absl::int128;absl::memory;absl::raw_logging_internal;absl::throw_delegate;absl::type_traits"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_strings${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_strings${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::strings_internal
add_library(absl::strings_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::strings_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::core_headers;absl::endian;absl::raw_logging_internal;absl::type_traits"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_strings_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_strings_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::str_format
add_library(absl::str_format INTERFACE IMPORTED)

set_target_properties(absl::str_format PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::str_format_internal;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::str_format_internal
add_library(absl::str_format_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::str_format_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bits;absl::strings;absl::config;absl::core_headers;absl::type_traits;absl::int128;absl::span"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_str_format_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_str_format_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::cord
add_library(absl::cord ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::cord PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::base_internal;absl::compressed_tuple;absl::core_headers;absl::endian;absl::fixed_array;absl::function_ref;absl::inlined_vector;absl::optional;absl::raw_logging_internal;absl::strings;absl::strings_internal;absl::type_traits"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_cord${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_cord${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::graphcycles_internal
add_library(absl::graphcycles_internal ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::graphcycles_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::base_internal;absl::config;absl::core_headers;absl::malloc_internal;absl::raw_logging_internal"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_graphcycles_internal${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_graphcycles_internal${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::kernel_timeout_internal
add_library(absl::kernel_timeout_internal INTERFACE IMPORTED)

set_target_properties(absl::kernel_timeout_internal PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::raw_logging_internal;absl::time;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::synchronization
add_library(absl::synchronization ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::synchronization PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::graphcycles_internal;absl::kernel_timeout_internal;absl::atomic_hook;absl::base;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::malloc_internal;absl::raw_logging_internal;absl::stacktrace;absl::symbolize;absl::time;Threads::Threads"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_synchronization${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_synchronization${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::time
add_library(absl::time ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::time PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base;absl::civil_time;absl::core_headers;absl::int128;absl::raw_logging_internal;absl::strings;absl::time_zone"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_time${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_time${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::civil_time
add_library(absl::civil_time ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::civil_time PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_civil_time${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_civil_time${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::time_zone
add_library(absl::time_zone ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::time_zone PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "\$<\$<PLATFORM_ID:Darwin>:>"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_time_zone${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_time_zone${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::any
add_library(absl::any INTERFACE IMPORTED)

set_target_properties(absl::any PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bad_any_cast;absl::config;absl::core_headers;absl::fast_type_id;absl::type_traits;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::bad_any_cast
add_library(absl::bad_any_cast INTERFACE IMPORTED)

set_target_properties(absl::bad_any_cast PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bad_any_cast_impl;absl::config;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::bad_any_cast_impl
add_library(absl::bad_any_cast_impl ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::bad_any_cast_impl PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::raw_logging_internal"
)

# Create imported target absl::span
add_library(absl::span INTERFACE IMPORTED)

set_target_properties(absl::span PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::algorithm;absl::core_headers;absl::throw_delegate;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::optional
add_library(absl::optional INTERFACE IMPORTED)

set_target_properties(absl::optional PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bad_optional_access;absl::base_internal;absl::config;absl::core_headers;absl::memory;absl::type_traits;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::bad_optional_access
add_library(absl::bad_optional_access ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::bad_optional_access PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::raw_logging_internal"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_bad_optional_access${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_bad_optional_access${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::bad_variant_access
add_library(absl::bad_variant_access ${ABSL_LIB_TYPE} IMPORTED)

set_target_properties(absl::bad_variant_access PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::config;absl::raw_logging_internal"
  IMPORTED_LOCATION "${ABSL_PATH_PREFIX}libabsl_bad_variant_access${ABSL_LIB_SUFFIX}"
  IMPORTED_SONAME "libabsl_bad_variant_access${ABSL_LIB_SUFFIX}"
)

# Create imported target absl::variant
add_library(absl::variant INTERFACE IMPORTED)

set_target_properties(absl::variant PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::bad_variant_access;absl::base_internal;absl::config;absl::core_headers;absl::type_traits;absl::utility;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::compare
add_library(absl::compare INTERFACE IMPORTED)

set_target_properties(absl::compare PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::core_headers;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Create imported target absl::utility
add_library(absl::utility INTERFACE IMPORTED)

set_target_properties(absl::utility PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${ABSL_INCLUDE_DIR}
  INTERFACE_LINK_LIBRARIES "absl::base_internal;absl::config;absl::type_traits;-Wl,--as-needed;-latomic;-Wl,--no-as-needed"
)

# Get all library paths for checking if they exist
list(APPEND _IMPORT_CHECK_TARGETS absl::log_severity)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::log_severity
  "${ABSL_PATH_PREFIX}libabsl_log_severity${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::raw_logging_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::raw_logging_internal
  "${ABSL_PATH_PREFIX}libabsl_raw_logging_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::spinlock_wait)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::spinlock_wait
  "${ABSL_PATH_PREFIX}libabsl_spinlock_wait${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::malloc_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::malloc_internal
  "${ABSL_PATH_PREFIX}libabsl_malloc_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::base)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::base
  "${ABSL_PATH_PREFIX}libabsl_base${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::throw_delegate)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::throw_delegate
  "${ABSL_PATH_PREFIX}libabsl_throw_delegate${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::exponential_biased)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::exponential_biased
  "${ABSL_PATH_PREFIX}libabsl_exponential_biased${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::periodic_sampler)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::periodic_sampler
  "${ABSL_PATH_PREFIX}libabsl_periodic_sampler${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::scoped_set_env)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::scoped_set_env
  "${ABSL_PATH_PREFIX}libabsl_scoped_set_env${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::strerror)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::strerror
  "${ABSL_PATH_PREFIX}libabsl_strerror${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::hashtablez_sampler)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::hashtablez_sampler
  "${ABSL_PATH_PREFIX}libabsl_hashtablez_sampler${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::raw_hash_set)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::raw_hash_set
  "${ABSL_PATH_PREFIX}libabsl_raw_hash_set${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::stacktrace)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::stacktrace
  "${ABSL_PATH_PREFIX}libabsl_stacktrace${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::symbolize)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::symbolize
  "${ABSL_PATH_PREFIX}libabsl_symbolize${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::examine_stack)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::examine_stack
  "${ABSL_PATH_PREFIX}libabsl_examine_stack${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::failure_signal_handler)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::failure_signal_handler
  "${ABSL_PATH_PREFIX}libabsl_failure_signal_handler${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::debugging_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::debugging_internal
  "${ABSL_PATH_PREFIX}libabsl_debugging_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::demangle_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::demangle_internal
  "${ABSL_PATH_PREFIX}libabsl_demangle_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::leak_check)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::leak_check
  "${ABSL_PATH_PREFIX}libabsl_leak_check${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::leak_check_disable)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::leak_check_disable
  "${ABSL_PATH_PREFIX}libabsl_leak_check_disable${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_program_name)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_program_name
  "${ABSL_PATH_PREFIX}libabsl_flags_program_name${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_config)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_config
  "${ABSL_PATH_PREFIX}libabsl_flags_config${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_marshalling)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_marshalling
  "${ABSL_PATH_PREFIX}libabsl_flags_marshalling${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_commandlineflag_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_commandlineflag_internal
  "${ABSL_PATH_PREFIX}libabsl_flags_commandlineflag_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_commandlineflag)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_commandlineflag
  "${ABSL_PATH_PREFIX}libabsl_flags_commandlineflag${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_private_handle_accessor)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_private_handle_accessor
  "${ABSL_PATH_PREFIX}libabsl_flags_private_handle_accessor${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_reflection)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_reflection
  "${ABSL_PATH_PREFIX}libabsl_flags_reflection${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_internal
  "${ABSL_PATH_PREFIX}libabsl_flags_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags
  "${ABSL_PATH_PREFIX}libabsl_flags${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_usage_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_usage_internal
  "${ABSL_PATH_PREFIX}libabsl_flags_usage_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_usage)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_usage
  "${ABSL_PATH_PREFIX}libabsl_flags_usage${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::flags_parse)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::flags_parse
  "${ABSL_PATH_PREFIX}libabsl_flags_parse${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::hash)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::hash
  "${ABSL_PATH_PREFIX}libabsl_hash${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::city)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::city
  "${ABSL_PATH_PREFIX}libabsl_city${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::int128)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::int128
  "${ABSL_PATH_PREFIX}libabsl_int128${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_distributions)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_distributions
  "${ABSL_PATH_PREFIX}libabsl_random_distributions${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_seed_gen_exception)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_seed_gen_exception
  "${ABSL_PATH_PREFIX}libabsl_random_seed_gen_exception${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_seed_sequences)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_seed_sequences
  "${ABSL_PATH_PREFIX}libabsl_random_seed_sequences${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_seed_material)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_seed_material
  "${ABSL_PATH_PREFIX}libabsl_random_internal_seed_material${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_pool_urbg)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_pool_urbg
  "${ABSL_PATH_PREFIX}libabsl_random_internal_pool_urbg${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_platform)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_platform
  "${ABSL_PATH_PREFIX}libabsl_random_internal_platform${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_randen)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_randen
  "${ABSL_PATH_PREFIX}libabsl_random_internal_randen${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_randen_slow)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_randen_slow
  "${ABSL_PATH_PREFIX}libabsl_random_internal_randen_slow${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_randen_hwaes)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_randen_hwaes
  "${ABSL_PATH_PREFIX}libabsl_random_internal_randen_hwaes${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_randen_hwaes_impl)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_randen_hwaes_impl
  "${ABSL_PATH_PREFIX}libabsl_random_internal_randen_hwaes_impl${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::random_internal_distribution_test_util)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::random_internal_distribution_test_util
  "${ABSL_PATH_PREFIX}libabsl_random_internal_distribution_test_util${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::status)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::status
  "${ABSL_PATH_PREFIX}libabsl_status${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::statusor)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::statusor
  "${ABSL_PATH_PREFIX}libabsl_statusor${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::strings)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::strings
  "${ABSL_PATH_PREFIX}libabsl_strings${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::strings_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::strings_internal
  "${ABSL_PATH_PREFIX}libabsl_strings_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::str_format_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::str_format_internal
  "${ABSL_PATH_PREFIX}libabsl_str_format_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::cord)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::cord
  "${ABSL_PATH_PREFIX}libabsl_cord${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::graphcycles_internal)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::graphcycles_internal
  "${ABSL_PATH_PREFIX}libabsl_graphcycles_internal${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::synchronization)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::synchronization
  "${ABSL_PATH_PREFIX}libabsl_synchronization${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::time)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::time
  "${ABSL_PATH_PREFIX}libabsl_time${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::civil_time)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::civil_time
  "${ABSL_PATH_PREFIX}libabsl_civil_time${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::time_zone)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::time_zone
  "${ABSL_PATH_PREFIX}libabsl_time_zone${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::bad_any_cast_impl)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::bad_any_cast_impl
  "${ABSL_PATH_PREFIX}libabsl_bad_any_cast_impl${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::bad_optional_access)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::bad_optional_access
  "${ABSL_PATH_PREFIX}libabsl_bad_optional_access${ABSL_LIB_SUFFIX}"
)
list(APPEND _IMPORT_CHECK_TARGETS absl::bad_variant_access)
list(APPEND _IMPORT_CHECK_FILES_FOR_absl::bad_variant_access
  "${ABSL_PATH_PREFIX}libabsl_bad_variant_access${ABSL_LIB_SUFFIX}"
)

# Loop over all imported files and verify that they actually exist
foreach(target ${_IMPORT_CHECK_TARGETS} )
  foreach(file ${_IMPORT_CHECK_FILES_FOR_${target}} )
    if(NOT EXISTS "${file}" )
      message(FATAL_ERROR "The imported target \"${target}\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    endif()
  endforeach()
  unset(_IMPORT_CHECK_FILES_FOR_${target})
endforeach()
unset(_IMPORT_CHECK_TARGETS)

