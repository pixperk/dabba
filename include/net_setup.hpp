#pragma once

#include "subprocess.hpp"
#include <string>

class Network
{
    bool active_ = false;

public:
    explicit Network(int child_pid)
    {
        const std::string pid = std::to_string(child_pid);

        run_cmd({"ip", "link", "add", "veth-host", "type", "veth",
                 "peer", "name", "veth-cont"});
        run_cmd({"ip", "link", "set", "veth-cont", "netns", pid});

        run_cmd({"ip", "addr", "add", "10.0.0.1/24", "dev", "veth-host"});
        run_cmd({"ip", "link", "set", "veth-host", "up"});

        run_cmd({"nsenter", "-t", pid, "-n", "ip", "addr", "add",
                 "10.0.0.2/24", "dev", "veth-cont"});
        run_cmd({"nsenter", "-t", pid, "-n", "ip", "link", "set", "veth-cont", "up"});
        run_cmd({"nsenter", "-t", pid, "-n", "ip", "route", "add",
                 "default", "via", "10.0.0.1"});

        run_cmd({"sysctl", "-w", "net.ipv4.ip_forward=1"});
        //here we add a NAT rule to allow the container to access the internet
        //we need to unmasquerade in the destructor to avoid leaving a rule that will break the host's networking if we crash
        run_cmd({"iptables", "-t", "nat", "-A", "POSTROUTING",
                 "-s", "10.0.0.0/24", "-j", "MASQUERADE"});

        active_ = true;
    }

    Network(const Network &) = delete;
    Network &operator=(const Network &) = delete;

    ~Network()
    {
        if (!active_)
            return;
        // -D removes exactly the rule -A added. never throw from a dtor,
        // and run_cmd throws, so swallow it.
        try
        {
            run_cmd({"iptables", "-t", "nat", "-D", "POSTROUTING",
                     "-s", "10.0.0.0/24", "-j", "MASQUERADE"});
        }
        catch (...)
        {
        }
    }
};
