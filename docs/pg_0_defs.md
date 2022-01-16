\page pg_defs Header File

The implementation uses `defs.h` to store all macros and define structs. The File Documentation for `defs.h` can provide a complete overview. This section serves as quick overview.

[defs.h:6-45] Defines the various macros used
- [defs.h:6-9] Macros for enabling/disabling debugging
- [defs.h:11-16] Macros for global file locks
- [defs.h:18-21] Macros for user input max sizes
- [defs.h:23-34] Macros for file accesses
- [defs.h:36-45] Macros for logging construct formatting

[defs.h:48-62] `command` is a struct used to represent the user input. Refer to `command` File Documentation for a complete overview. It will be explained in the following sections.

[defs.h:64-82] `args_t` is a struct used for synchronization. Refer to `args_t` File Documentation for a complete overview. It will be explained in the following sections.

[defs.h:84-98] `fmeta` is a struct used to track target files. Refer to `fmeta` File Documentation for a complete overview. It will be explained in the following sections.

[defs.h:100-115] `lnode_t` is a custom linked list node. It allows key, value assignments to `fmeta` structs. Refer to `fmeta` File Documentation for a complete overview. It will be explained in the following sections.

[defs.h:117-125] `list_t` custom linked list struct. Used by `tracker`. Only accessed by `master` thread. Refer to `fmeta` File Documentation for a complete overview. It will be explained in the following sections.




