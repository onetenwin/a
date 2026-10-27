// Assignment 10 : RIP vs OSPF (link-state / global routing) vs AODV
// Conditions: Normal, Congestion, Link-failure.  Writes routing_results.csv for plotting.
// NOTE: ns-3 has no OSPF module; OSPF is emulated with global (link-state SPF) routing that is
//       recomputed 1 s after the failure (models hello-dead detection + LSA flood + SPF).
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/aodv-module.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
using namespace ns3;

static uint32_t g_ctrl = 0;      // RIP(udp/520) or AODV(udp/654) control messages sent
static uint32_t g_recompute = 0; // OSPF SPF recomputations

static void L3Tx(Ptr<const Packet> pkt, Ptr<Ipv4>, uint32_t)
{
    Ptr<Packet> p = pkt->Copy();
    Ipv4Header ih;
    if (p->PeekHeader(ih) == 0) return;
    p->RemoveHeader(ih);
    if (ih.GetProtocol() == 17)
    {
        UdpHeader uh;
        if (p->PeekHeader(uh) == 0) return;
        if (uh.GetDestinationPort() == 520 || uh.GetDestinationPort() == 654) g_ctrl++;
    }
}
static void Recompute()
{
    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    g_recompute++;
}

struct Out { double pdr; double delay; double updates; };
struct Flow { uint32_t src; uint32_t dst; uint16_t port; double mbps; };

// cond: 0 = normal, 1 = congestion, 2 = link failure
static Out RunOne(std::string proto, int cond)
{
    g_ctrl = 0; g_recompute = 0;
    Config::SetDefault("ns3::Rip::UnsolicitedRoutingUpdate", TimeValue(Seconds(3)));

    NodeContainer hosts;   hosts.Create(4);   // 0:S0 1:S1 2:D0 3:D1
    NodeContainer routers; routers.Create(5); // R0 (ingress) R1 (primary mid) R2,R3 (backup) R4 (egress)

    // ---- routing protocol selection ----
    InternetStackHelper stack;
    if (proto == "RIP")
    {
        RipHelper rip;
        Ipv4ListRoutingHelper list;
        list.Add(rip, 0);
        stack.SetRoutingHelper(list);
    }
    else if (proto == "AODV")
    {
        AodvHelper aodv;
        stack.SetRoutingHelper(aodv);
    }
    stack.Install(hosts);
    stack.Install(routers);

    PointToPointHelper acc, core, backup;
    acc.SetDeviceAttribute("DataRate", StringValue("10Mbps"));  acc.SetChannelAttribute("Delay", StringValue("2ms"));
    core.SetDeviceAttribute("DataRate", StringValue("5Mbps"));  core.SetChannelAttribute("Delay", StringValue("5ms"));
    backup.SetDeviceAttribute("DataRate", StringValue("5Mbps")); backup.SetChannelAttribute("Delay", StringValue("10ms"));

    Ipv4AddressHelper ip;
    int sub = 1;
    auto assign = [&](NetDeviceContainer d) {
        ip.SetBase(("10.1." + std::to_string(sub++) + ".0").c_str(), "255.255.255.0");
        return ip.Assign(d);
    };
    Ipv4InterfaceContainer iS0 = assign(acc.Install(hosts.Get(0), routers.Get(0)));
    Ipv4InterfaceContainer iS1 = assign(acc.Install(hosts.Get(1), routers.Get(0)));
    Ipv4InterfaceContainer iFail = assign(core.Install(routers.Get(0), routers.Get(1))); // link that will fail
    assign(core.Install(routers.Get(1), routers.Get(4)));                                  // primary path R0-R1-R4
    assign(backup.Install(routers.Get(0), routers.Get(2)));                                // backup path R0-R2-R3-R4
    assign(backup.Install(routers.Get(2), routers.Get(3)));
    assign(backup.Install(routers.Get(3), routers.Get(4)));
    Ipv4InterfaceContainer iD0 = assign(acc.Install(routers.Get(4), hosts.Get(2)));
    Ipv4InterfaceContainer iD1 = assign(acc.Install(routers.Get(4), hosts.Get(3)));

    if (proto == "OSPF")
    {
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
        g_recompute = 1; // initial SPF
    }

    // ---- traffic ----
    Ipv4Address dst[2] = {iD0.GetAddress(1), iD1.GetAddress(1)};
    std::vector<Flow> flows;
    flows.push_back({0, 0, 5000, 1.0});
    flows.push_back({1, 1, 5001, 1.0});
    if (cond == 1) // congestion: 4 extra simultaneous flows, total offered 8 Mbps > 5 Mbps core
    {
        flows.push_back({0, 0, 5002, 1.5});
        flows.push_back({0, 0, 5003, 1.5});
        flows.push_back({1, 1, 5004, 1.5});
        flows.push_back({1, 1, 5005, 1.5});
    }
    const double tStart = 10.0, tEnd = 60.0;
    for (auto &f : flows)
    {
        UdpServerHelper srv(f.port);
        ApplicationContainer sa = srv.Install(hosts.Get(2 + f.dst));
        sa.Start(Seconds(0.0)); sa.Stop(Seconds(tEnd + 1));
        UdpClientHelper cli(dst[f.dst], f.port);
        cli.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        cli.SetAttribute("PacketSize", UintegerValue(512));
        cli.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / (f.mbps * 1e6))));
        ApplicationContainer ca = cli.Install(hosts.Get(f.src));
        ca.Start(Seconds(tStart)); ca.Stop(Seconds(tEnd));
    }

    // ---- link failure at t=30 s (both ends of R0-R1) ----
    if (cond == 2)
    {
        Simulator::Schedule(Seconds(30.0), &Ipv4::SetDown, iFail.Get(0).first, iFail.Get(0).second);
        Simulator::Schedule(Seconds(30.0), &Ipv4::SetDown, iFail.Get(1).first, iFail.Get(1).second);
        if (proto == "OSPF") Simulator::Schedule(Seconds(31.0), &Recompute);
    }

    // ---- routing table snapshots before / after failure ----
    std::string logName = "routes-" + proto + "-cond" + std::to_string(cond) + ".log";
    Ptr<OutputStreamWrapper> rt = Create<OutputStreamWrapper>(logName, std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(25.0), rt);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(40.0), rt);

    Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&L3Tx));

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(tEnd + 1));
    Simulator::Run();
    mon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    double tx = 0, rx = 0, dsum = 0;
    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        if (t.destinationPort < 5000 || t.destinationPort > 5005) continue; // ignore RIP/AODV control flows
        tx += kv.second.txPackets; rx += kv.second.rxPackets; dsum += kv.second.delaySum.GetSeconds();
    }
    Out o;
    o.pdr = tx > 0 ? 100.0 * rx / tx : 0;
    o.delay = rx > 0 ? dsum / rx * 1000.0 : 0;
    // OSPF: every router rebuilds its table at each SPF run -> recomputations x nodes
    o.updates = (proto == "OSPF") ? g_recompute * 9.0 : g_ctrl;
    Simulator::Destroy();
    return o;
}

int main(int argc, char *argv[])
{
    std::string protos[] = {"RIP", "OSPF", "AODV"};
    std::string conds[] = {"Normal", "Congestion", "LinkFailure"};
    std::ofstream csv("routing_results.csv");
    csv << "protocol,condition,pdr,delay_ms,updates\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left << std::setw(8) << "Proto" << std::setw(13) << "Condition" << std::right
              << std::setw(10) << "PDR(%)" << std::setw(12) << "Delay(ms)" << std::setw(18) << "RoutingUpdates" << "\n";
    std::cout << std::string(61, '-') << "\n";
    for (auto &p : protos)
        for (int c = 0; c < 3; c++)
        {
            Out o = RunOne(p, c);
            std::cout << std::left << std::setw(8) << p << std::setw(13) << conds[c] << std::right
                      << std::setw(10) << o.pdr << std::setw(12) << o.delay << std::setw(18) << o.updates << "\n";
            csv << p << "," << conds[c] << "," << o.pdr << "," << o.delay << "," << o.updates << "\n";
        }
    csv.close();
    std::cout << "\nSaved routing_results.csv  -> run:  python3 plot_assign10.py   (makes the 3 required graphs)\n"
              << "Routing tables before/after failure are in routes-<PROTO>-cond<N>.log\n"
              << "RoutingUpdates = RIP/AODV control messages (udp 520/654) sent; OSPF = SPF recomputations x 9 nodes.\n";
    return 0;
}
