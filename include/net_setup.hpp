#pragma once

#include "subprocess.hpp"
#include "id.hpp"
#include <string>

class Network
{
    bool active_ = false;
    std::string subnet_;   // the dtor needs this to delete the right rule

public:
    Network(int child_pid, uint16_t id)
    {
        const std::string pid = std::to_string(child_pid);
        const std::string h   = id_hex(id);

        const std::string vh = "veth-" + h + "-h";   // e.g. veth-3f9a-h
        const std::string vc = "veth-" + h + "-c";

        // slice the 16-bit id into the middle two octets: 10.<hi>.<lo>.0/24
        const int o2 = (id >> 8) & 0xff;
        const int o3 = id & 0xff;
        const std::string base    = "10." + std::to_string(o2) + "." + std::to_string(o3);
        const std::string host_ip = base + ".1";
        const std::string cont_ip = base + ".2";
        subnet_ = base + ".0/24";

        run_cmd({"ip", "link", "add", vh, "type", "veth", "peer", "name", vc});
        run_cmd({"ip", "link", "set", vc, "netns", pid});

        run_cmd({"ip", "addr", "add", host_ip + "/24", "dev", vh});
        run_cmd({"ip", "link", "set", vh, "up"});

        run_cmd({"nsenter", "-t", pid, "-n", "ip", "addr", "add", cont_ip + "/24", "dev", vc});
        run_cmd({"nsenter", "-t", pid, "-n", "ip", "link", "set", vc, "up"});
        run_cmd({"nsenter", "-t", pid, "-n", "ip", "route", "add", "default", "via", host_ip});

        run_cmd({"sysctl", "-w", "net.ipv4.ip_forward=1"});
        run_cmd({"iptables", "-t", "nat", "-A", "POSTROUTING",
                 "-s", subnet_, "-j", "MASQUERADE"});

        active_ = true;
    }

    Network(const Network &) = delete;
    Network &operator=(const Network &) = delete;

    ~Network()
    {
        if (!active_)
            return;
        try
        {
            run_cmd({"iptables", "-t", "nat", "-D", "POSTROUTING",
                     "-s", subnet_, "-j", "MASQUERADE"});
        }
        catch (...)
        {
        }
    }
};
