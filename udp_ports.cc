// UDP traffic + port based delivery, effect of link latency on throughput
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <iostream>
#include <iomanip>
using namespace ns3;

struct Res { double t1; double t2; };

// Rate1 = 1 Mbps (port 5000), Rate2 = 2 Mbps (port 6000), packet = 512 B
static Res RunOnce(std::string latency, bool twoFlows)
{
    const double rate1 = 1e6, rate2 = 2e6;
    const uint32_t pktSize = 512;
    const double appStart = 1.0, appStop = 11.0;

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue(latency));
    NetDeviceContainer dev = p2p.Install(nodes.Get(0), nodes.Get(1));

    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface = ipv4.Assign(dev);

    // ---- Flow 1 : port 5000 ----
    UdpServerHelper server1(5000);
    ApplicationContainer sApp1 = server1.Install(nodes.Get(1));
    sApp1.Start(Seconds(0.0));
    sApp1.Stop(Seconds(appStop + 1));

    UdpClientHelper client1(iface.GetAddress(1), 5000);
    client1.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client1.SetAttribute("PacketSize", UintegerValue(pktSize));
    client1.SetAttribute("Interval", TimeValue(Seconds(pktSize * 8.0 / rate1)));
    ApplicationContainer cApp1 = client1.Install(nodes.Get(0));
    cApp1.Start(Seconds(appStart));
    cApp1.Stop(Seconds(appStop));

    ApplicationContainer sApp2;
    if (twoFlows)
    {
        // ---- Flow 2 : port 6000 ----
        UdpServerHelper server2(6000);
        sApp2 = server2.Install(nodes.Get(1));
        sApp2.Start(Seconds(0.0));
        sApp2.Stop(Seconds(appStop + 1));

        UdpClientHelper client2(iface.GetAddress(1), 6000);
        client2.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        client2.SetAttribute("PacketSize", UintegerValue(pktSize));
        client2.SetAttribute("Interval", TimeValue(Seconds(pktSize * 8.0 / rate2)));
        ApplicationContainer cApp2 = client2.Install(nodes.Get(0));
        cApp2.Start(Seconds(appStart));
        cApp2.Stop(Seconds(appStop));
    }

    // PCAP for Wireshark (filter: udp.port==5000 / udp.port==6000)
    p2p.EnablePcapAll(std::string("udp-ports-") + (twoFlows ? "two-" : "one-") + latency);

    Simulator::Stop(Seconds(appStop + 1));
    Simulator::Run();

    double dur = appStop - appStart;
    Res r;
    r.t1 = DynamicCast<UdpServer>(sApp1.Get(0))->GetReceived() * pktSize * 8.0 / dur / 1e6;
    r.t2 = twoFlows ? DynamicCast<UdpServer>(sApp2.Get(0))->GetReceived() * pktSize * 8.0 / dur / 1e6 : 0;
    Simulator::Destroy();
    return r;
}

int main(int argc, char *argv[])
{
    std::string lats[] = {"2ms", "5ms", "10ms", "20ms"};
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "\n=== Step 3: ONE UDP flow (port 5000, 1 Mbps) ===\n";
    std::cout << "Latency\tThroughput\n";
    for (auto &l : lats)
    {
        Res r = RunOnce(l, false);
        std::cout << l << "\t" << r.t1 << " Mbps\n";
    }

    std::cout << "\n=== Step 5/7: TWO UDP flows (port 5000 @1Mbps, port 6000 @2Mbps) ===\n";
    std::cout << "Latency\tFlow1 Thr\tFlow2 Thr\n";
    for (auto &l : lats)
    {
        Res r = RunOnce(l, true);
        std::cout << l << "\t" << r.t1 << " Mbps\t" << r.t2 << " Mbps\n";
    }

    std::cout << "\nObservation: UDP is not window-controlled, so throughput stays ~equal to the offered rate\n"
              << "for any latency (latency only shifts packets in time). Open the generated\n"
              << "udp-ports-two-*-0-0.pcap files in Wireshark: dst port 5000 -> Server 1, 6000 -> Server 2.\n";
    return 0;
}
