#include "net_setup.hpp"
#include "subprocess.hpp"
#include <string>

void setup_network(int child_pid)
{
    const std::string pid = std::to_string(child_pid);

    // 1. create the veth pair
    run_cmd({"ip", "link", "add", "veth-host", "type", "veth",
             "peer", "name", "veth-cont"});

    // 2. move one end into the container's netns
    run_cmd({"ip", "link", "set", "veth-cont", "netns", pid});

    // 3. host end: address + up
    run_cmd({"ip", "addr", "add", "10.0.0.1/24", "dev", "veth-host"});
    run_cmd({"ip", "link", "set", "veth-host", "up"});

    // 4. container end: address + up + default route, via nsenter
    run_cmd({"nsenter", "-t", pid, "-n", "ip", "addr", "add",
             "10.0.0.2/24", "dev", "veth-cont"});
    run_cmd({"nsenter", "-t", pid, "-n", "ip", "link", "set", "veth-cont", "up"});
    run_cmd({"nsenter", "-t", pid, "-n", "ip", "route", "add",
             "default", "via", "10.0.0.1"});

    // 5. host: forwarding + NAT
    run_cmd({"sysctl", "-w", "net.ipv4.ip_forward=1"});
    run_cmd({"iptables", "-t", "nat", "-A", "POSTROUTING",
             "-s", "10.0.0.0/24", "-j", "MASQUERADE"});
}
