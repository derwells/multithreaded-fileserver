\page pg_appendix Appendix

# Automated test test.in {#sampletestin}
\snippet docs/snippets/snippets.txt test.in
# Automated test read.txt {#sampleread}
\snippet docs/snippets/snippets.txt read.txt
# Automated test gen_read.txt {#genread}
\snippet docs/snippets/snippets.txt gen_read.txt
# Automated test empty.txt {#sampleempty}
\snippet docs/snippets/snippets.txt empty.txt
# Automated test gen_empty.txt {#genempty}
\snippet docs/snippets/snippets.txt gen_empty.txt
# Automated test commands.txt {#samplecommands}
\snippet docs/snippets/snippets.txt commands.txt
# Valgrind output {#sampleval}
```
==13176== Memcheck, a memory error detector
==13176== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==13176== Using Valgrind-3.15.0-608cb11914-20190413 and LibVEX; rerun with -h for copyright info
==13176== Command: ./file_server
==13176== Parent PID: 6844
==13176== 
--13176-- 
--13176-- Valgrind options:
--13176--    --leak-check=full
--13176--    --show-leak-kinds=all
--13176--    --track-origins=yes
--13176--    --verbose
--13176--    --log-file=valgrind-out.txt
--13176-- Contents of /proc/version:
--13176--   Linux version 5.4.72-microsoft-standard-WSL2 (oe-user@oe-host) (gcc version 8.2.0 (GCC)) #1 SMP Wed Oct 28 23:40:43 UTC 2020
--13176-- 
--13176-- Arch and hwcaps: AMD64, LittleEndian, amd64-cx16-lzcnt-rdtscp-sse3-ssse3-avx-avx2-bmi-f16c-rdrand
--13176-- Page sizes: currently 4096, max supported 4096
--13176-- Valgrind library directory: /usr/lib/x86_64-linux-gnu/valgrind
--13176-- Reading syms from /home/derick/acad/cs140/proj2/file_server
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/ld-2.31.so
--13176--   Considering /usr/lib/x86_64-linux-gnu/ld-2.31.so ..
--13176--   .. CRC mismatch (computed 975d0390 wanted 30bd717f)
--13176--   Considering /lib/x86_64-linux-gnu/ld-2.31.so ..
--13176--   .. CRC mismatch (computed 975d0390 wanted 30bd717f)
--13176--   Considering /usr/lib/debug/lib/x86_64-linux-gnu/ld-2.31.so ..
--13176--   .. CRC is valid
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/valgrind/memcheck-amd64-linux
--13176--    object doesn't have a symbol table
--13176--    object doesn't have a dynamic symbol table
--13176-- Scheduler: using generic scheduler lock implementation.
--13176-- Reading suppressions file: /usr/lib/x86_64-linux-gnu/valgrind/default.supp
==13176== embedded gdbserver: reading from /tmp/vgdb-pipe-from-vgdb-to-13176-by-derick-on-???
==13176== embedded gdbserver: writing to   /tmp/vgdb-pipe-to-vgdb-from-13176-by-derick-on-???
==13176== embedded gdbserver: shared mem   /tmp/vgdb-pipe-shared-mem-vgdb-13176-by-derick-on-???
==13176== 
==13176== TO CONTROL THIS PROCESS USING vgdb (which you probably
==13176== don't want to do, unless you know exactly what you're doing,
==13176== or are doing some strange experiment):
==13176==   /usr/lib/x86_64-linux-gnu/valgrind/../../bin/vgdb --pid=13176 ...command...
==13176== 
==13176== TO DEBUG THIS PROCESS USING GDB: start GDB like this
==13176==   /path/to/gdb ./file_server
==13176== and then give GDB the following command
==13176==   target remote | /usr/lib/x86_64-linux-gnu/valgrind/../../bin/vgdb --pid=13176
==13176== --pid is optional if only one valgrind process is running
==13176== 
--13176-- REDIR: 0x4022e10 (ld-linux-x86-64.so.2:strlen) redirected to 0x580c9ce2 (???)
--13176-- REDIR: 0x4022be0 (ld-linux-x86-64.so.2:index) redirected to 0x580c9cfc (???)
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_core-amd64-linux.so
--13176--    object doesn't have a symbol table
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so
--13176--    object doesn't have a symbol table
==13176== WARNING: new redirection conflicts with existing -- ignoring it
--13176--     old: 0x04022e10 (strlen              ) R-> (0000.0) 0x580c9ce2 ???
--13176--     new: 0x04022e10 (strlen              ) R-> (2007.0) 0x0483f060 strlen
--13176-- REDIR: 0x401f5f0 (ld-linux-x86-64.so.2:strcmp) redirected to 0x483ffd0 (strcmp)
--13176-- REDIR: 0x4023370 (ld-linux-x86-64.so.2:mempcpy) redirected to 0x4843a20 (mempcpy)
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/libpthread-2.31.so
--13176--   Considering /usr/lib/debug/.build-id/e5/4761f7b554d0fcc1562959665d93dffbebdaf0.debug ..
--13176--   .. build-id is valid
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/libc-2.31.so
--13176--   Considering /usr/lib/x86_64-linux-gnu/libc-2.31.so ..
--13176--   .. CRC mismatch (computed 86b78530 wanted e380f01c)
--13176--   Considering /lib/x86_64-linux-gnu/libc-2.31.so ..
--13176--   .. CRC mismatch (computed 86b78530 wanted e380f01c)
--13176--   Considering /usr/lib/debug/lib/x86_64-linux-gnu/libc-2.31.so ..
--13176--   .. CRC is valid
--13176-- REDIR: 0x491a600 (libc.so.6:memmove) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919900 (libc.so.6:strncpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a930 (libc.so.6:strcasecmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919220 (libc.so.6:strcat) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919960 (libc.so.6:rindex) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491bdd0 (libc.so.6:rawmemchr) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4936e60 (libc.so.6:wmemchr) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49369a0 (libc.so.6:wcscmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a760 (libc.so.6:mempcpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a590 (libc.so.6:bcmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919890 (libc.so.6:strncmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49192d0 (libc.so.6:strcmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a6c0 (libc.so.6:memset) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4936960 (libc.so.6:wcschr) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49197f0 (libc.so.6:strnlen) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49193b0 (libc.so.6:strcspn) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a980 (libc.so.6:strncasecmp) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919350 (libc.so.6:strcpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491aad0 (libc.so.6:memcpy@@GLIBC_2.14) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49380d0 (libc.so.6:wcsnlen) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49369e0 (libc.so.6:wcscpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49199a0 (libc.so.6:strpbrk) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919280 (libc.so.6:index) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x49197b0 (libc.so.6:strlen) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4922d20 (libc.so.6:memrchr) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a9d0 (libc.so.6:strcasecmp_l) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a550 (libc.so.6:memchr) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4936ab0 (libc.so.6:wcslen) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4919c60 (libc.so.6:strspn) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a8d0 (libc.so.6:stpncpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491a870 (libc.so.6:stpcpy) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491be10 (libc.so.6:strchrnul) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x491aa20 (libc.so.6:strncasecmp_l) redirected to 0x48311d0 (_vgnU_ifunc_wrapper)
--13176-- REDIR: 0x4a02490 (libc.so.6:__strrchr_avx2) redirected to 0x483ea10 (rindex)
--13176-- REDIR: 0x4914260 (libc.so.6:malloc) redirected to 0x483b780 (malloc)
--13176-- REDIR: 0x4915c90 (libc.so.6:calloc) redirected to 0x483dce0 (calloc)
--13176-- REDIR: 0x49fe4c0 (libc.so.6:__memchr_avx2) redirected to 0x4840050 (memchr)
--13176-- REDIR: 0x4a05670 (libc.so.6:__memcpy_avx_unaligned_erms) redirected to 0x48429f0 (memmove)
--13176-- REDIR: 0x49fd7b0 (libc.so.6:__strcspn_sse42) redirected to 0x4843e10 (strcspn)
--13176-- REDIR: 0x49fda30 (libc.so.6:__strspn_sse42) redirected to 0x4843ef0 (strspn)
--13176-- REDIR: 0x4a03ba0 (libc.so.6:__strcpy_avx2) redirected to 0x483f090 (strcpy)
--13176-- REDIR: 0x49fdb60 (libc.so.6:__strcmp_avx2) redirected to 0x483fed0 (strcmp)
--13176-- REDIR: 0x4a02660 (libc.so.6:__strlen_avx2) redirected to 0x483ef40 (strlen)
--13176-- REDIR: 0x4914850 (libc.so.6:free) redirected to 0x483c9d0 (free)
--13176-- REDIR: 0x491a120 (libc.so.6:__GI_strstr) redirected to 0x4843ca0 (__strstr_sse2)
--13176-- REDIR: 0x49fec50 (libc.so.6:__memcmp_avx2_movbe) redirected to 0x48421e0 (bcmp)
--13176-- REDIR: 0x4a022a0 (libc.so.6:__strchrnul_avx2) redirected to 0x4843540 (strchrnul)
--13176-- REDIR: 0x4a02800 (libc.so.6:__strnlen_avx2) redirected to 0x483eee0 (strnlen)
--13176-- REDIR: 0x4a05650 (libc.so.6:__mempcpy_avx_unaligned_erms) redirected to 0x4843660 (mempcpy)
--13176-- Reading syms from /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
--13176--    object doesn't have a symbol table
--13176-- REDIR: 0x4a05af0 (libc.so.6:__memset_avx2_unaligned_erms) redirected to 0x48428e0 (memset)
==13176== 
==13176== Process terminating with default action of signal 2 (SIGINT)
==13176==    at 0x485ECD7: __pthread_clockjoin_ex (pthread_join_common.c:145)
==13176==    by 0x10A2CD: main (file_server.c:680)
--13176-- Discarding syms at 0x13e8d5e0-0x13e9e045 in /usr/lib/x86_64-linux-gnu/libgcc_s.so.1 (have_dinfo 1)
==13176== 
==13176== HEAP SUMMARY:
==13176==     in use at exit: 1,028 bytes in 18 blocks
==13176==   total heap usage: 1,160 allocs, 1,142 frees, 1,235,327 bytes allocated
==13176== 
==13176== Searching for pointers to 18 not-freed blocks
==13176== Checked 8,483,336 bytes
==13176== 
==13176== 8 bytes in 1 blocks are still reachable in loss record 1 of 6
==13176==    at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x10A289: main (file_server.c:672)
==13176== 
==13176== 108 bytes in 1 blocks are still reachable in loss record 2 of 6
==13176==    at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x10A105: master (file_server.c:602)
==13176==    by 0x485D608: start_thread (pthread_create.c:477)
==13176==    by 0x4999292: clone (clone.S:95)
==13176== 
==13176== 120 bytes in 5 blocks are still reachable in loss record 3 of 6
==13176==    at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x10959B: l_insert (file_server.c:51)
==13176==    by 0x10A1D9: master (file_server.c:635)
==13176==    by 0x485D608: start_thread (pthread_create.c:477)
==13176==    by 0x4999292: clone (clone.S:95)
==13176== 
==13176== 200 bytes in 5 blocks are still reachable in loss record 4 of 6
==13176==    at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x109F3C: args_init (file_server.c:520)
==13176==    by 0x10A136: master (file_server.c:607)
==13176==    by 0x485D608: start_thread (pthread_create.c:477)
==13176==    by 0x4999292: clone (clone.S:95)
==13176== 
==13176== 272 bytes in 1 blocks are possibly lost in loss record 5 of 6
==13176==    at 0x483DD99: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x40149CA: allocate_dtv (dl-tls.c:286)
==13176==    by 0x40149CA: _dl_allocate_tls (dl-tls.c:532)
==13176==    by 0x485E322: allocate_stack (allocatestack.c:622)
==13176==    by 0x485E322: pthread_create@@GLIBC_2.2.5 (pthread_create.c:660)
==13176==    by 0x10A2BC: main (file_server.c:677)
==13176== 
==13176== 320 bytes in 5 blocks are still reachable in loss record 6 of 6
==13176==    at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
==13176==    by 0x10A1A4: master (file_server.c:631)
==13176==    by 0x485D608: start_thread (pthread_create.c:477)
==13176==    by 0x4999292: clone (clone.S:95)
==13176== 
==13176== LEAK SUMMARY:
==13176==    definitely lost: 0 bytes in 0 blocks
==13176==    indirectly lost: 0 bytes in 0 blocks
==13176==      possibly lost: 272 bytes in 1 blocks
==13176==    still reachable: 756 bytes in 17 blocks
==13176==         suppressed: 0 bytes in 0 blocks
==13176== 
==13176== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0)
```
