#include "fs_setup.hpp"

#include<sys/mount.h>
#include<sys/syscall.h>
#include<unistd.h>

#include "util.hpp"

namespace fs = std::filesystem;

void setup_rootfs(const fs::path &rootfs)
{
    // our mounts must not propagate back to the host
    checked(mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr), "make / private");

    // pivot_root needs a mount point, not a directory, so bind it onto itself
    checked(mount(rootfs.c_str(), rootfs.c_str(), nullptr, MS_BIND | MS_REC, nullptr), "bind rootfs");

    fs::path oldroot = rootfs / ".oldroot";
    fs::create_directories(oldroot);

    checked(static_cast<int>(syscall(SYS_pivot_root, rootfs.c_str(), oldroot.c_str())), "pivot_root");
    checked(chdir("/"), "chdir /");

    // detach the host tree so there is no path back to it
    checked(umount2("/.oldroot", MNT_DETACH), "umount oldroot");
    fs::remove("/.oldroot");

    // fresh procfs, mounted in our pid namespace this time
    checked(mount("proc", "/proc", "proc", 0, nullptr), "mount /proc");

    // fresh devtmpfs, mounted in our new mount namespace
    checked(mount("dev", "/dev", "devtmpfs", 0, nullptr), "mount /dev");

}