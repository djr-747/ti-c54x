"""Static source-boundary policy data owned by the quality gate.

Owns: path-owner sets, size thresholds, and forbidden host/lineage names.
Does not own: C lexical scanning, report formatting, or build configuration.
"""

MAX_REVIEW_LINES = 500
MAX_SOURCE_LINES = 800

ALLOWED_SRC_OWNERS = {
    "address",
    "bus",
    "cpu",
    "debug",
    "devices",
    "foundation",
    "sequencer",
    "state",
}
ALLOWED_ISA_OWNERS = {"catalog", "decode", "ops"}
# Unit-test-only owners with no src/ counterpart. Hosted tooling
# peers live under tools/ (outside the scanned production tree)
# and their tests live under tests/unit/.
ALLOWED_UNIT_ONLY_OWNERS = {"hosted"}
PRODUCTION_SUFFIXES = {".c", ".h"}
TEST_SUFFIXES = {".c", ".h"}
TEST_ROOTS = {"contract", "differential", "fuzz"}

FORBIDDEN_API_NAMES = {
    "_popen",
    "abort",
    "atexit",
    "clearenv",
    "close",
    "creat",
    "dlclose",
    "dlopen",
    "dlsym",
    "execle",
    "execl",
    "execlp",
    "execv",
    "execve",
    "execvp",
    "fclose",
    "fdopen",
    "fflush",
    "fopen",
    "fork",
    "fprintf",
    "fputc",
    "fputs",
    "fread",
    "freopen",
    "fscanf",
    "fseek",
    "ftell",
    "fwrite",
    "getauxval",
    "getenv",
    "mkstemp",
    "open",
    "pclose",
    "perror",
    "pipe",
    "popen",
    "printf",
    "putc",
    "putenv",
    "puts",
    "read",
    "remove",
    "rename",
    "secure_getenv",
    "setenv",
    "snprintf",
    "sprintf",
    "sscanf",
    "syslog",
    "system",
    "tmpfile",
    "tmpnam",
    "unsetenv",
    "vfprintf",
    "vprintf",
    "vsnprintf",
    "vsprintf",
    "vscanf",
    "vsscanf",
    "write",
}
FORBIDDEN_INCLUDE_NAMES = {
    "<android/log.h>",
    "<dlfcn.h>",
    "<fcntl.h>",
    "<io.h>",
    "<process.h>",
    "<stdarg.h>",
    "<stdio.h>",
    "<stdlib.h>",
    "<syslog.h>",
    "<sys/mman.h>",
    "<sys/socket.h>",
    "<sys/types.h>",
    "<unistd.h>",
    "<windows.h>",
}
FORBIDDEN_PLATFORM_NAMES = {
    "calypso",
    "dct3",
    "firmware",
    "mad2",
    "nokia",
    "nokix",
    "osmocom",
    "qemu",
}
ENV_API_NAMES = {
    "clearenv",
    "getauxval",
    "getenv",
    "putenv",
    "secure_getenv",
    "setenv",
    "unsetenv",
}
