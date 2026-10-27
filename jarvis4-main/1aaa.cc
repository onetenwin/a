
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace std;
using namespace ns3;

void RunExperiment()
{
    // 1. Create four nodes
    NodeContainer nodes;
    nodes.Create(4);

    // 2. Create three point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NodeContainer link1(nodes.Get(0), nodes.Get(1));
    NodeContainer link2(nodes.Get(1), nodes.Get(2));
    NodeContainer link3(nodes.Get(1), nodes.Get(3));

    NetDeviceContainer d1 = p2p.Install(link1);
    NetDeviceContainer d2 = p2p.Install(link2);
    NetDeviceContainer d3 = p2p.Install(link3);

    // 3. Install Internet Stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. Assign IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i1 = address.Assign(d1);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i2 = address.Assign(d2);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = address.Assign(d3);

    // 5. Configure routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 6. Flow 1: n0 -> n2, 1 Mbps
    uint16_t port1 = 5000;

    PacketSinkHelper sink1(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port1));

    ApplicationContainer sinkApps1 =
        sink1.Install(nodes.Get(2));

    sinkApps1.Start(Seconds(0.0));
    sinkApps1.Stop(Seconds(10.0));

    OnOffHelper flow1(
        "ns3::UdpSocketFactory",
        InetSocketAddress(i2.GetAddress(1), port1));

    flow1.SetAttribute("DataRate", StringValue("1Mbps"));
    flow1.SetAttribute("PacketSize", UintegerValue(1000));

    ApplicationContainer flowApps1 =
        flow1.Install(nodes.Get(0));

    flowApps1.Start(Seconds(1.0));
    flowApps1.Stop(Seconds(9.0));

    // 7. Flow 2: n3 -> n0, 500 Kbps
    uint16_t port2 = 6000;

    PacketSinkHelper sink2(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port2));

    ApplicationContainer sinkApps2 =
        sink2.Install(nodes.Get(0));

    sinkApps2.Start(Seconds(0.0));
    sinkApps2.Stop(Seconds(10.0));

    OnOffHelper flow2(
        "ns3::UdpSocketFactory",
        InetSocketAddress(i1.GetAddress(0), port2));

    flow2.SetAttribute("DataRate", StringValue("500Kbps"));
    flow2.SetAttribute("PacketSize", UintegerValue(1000));

    ApplicationContainer flowApps2 =
        flow2.Install(nodes.Get(3));

    flowApps2.Start(Seconds(1.0));
    flowApps2.Stop(Seconds(9.0));

    // 8. Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // 9. Calculate total bytes received
    Ptr<PacketSink> s1 =
        DynamicCast<PacketSink>(sinkApps1.Get(0));

    Ptr<PacketSink> s2 =
        DynamicCast<PacketSink>(sinkApps2.Get(0));

    double bytes1 = s1->GetTotalRx();
    double bytes2 = s2->GetTotalRx();

    // 10. Calculate throughput over 8 seconds
    double throughput1 = bytes1 * 8.0 / (8.0 * 1000000.0);
    double throughput2 = bytes2 * 8.0 / (8.0 * 1000000.0);

    cout << "Flow 1 received bytes: " << bytes1 << endl;
    cout << "Flow 2 received bytes: " << bytes2 << endl;
    cout << "Total bytes received: " << bytes1 + bytes2 << endl;

    cout << "Flow 1 throughput: " << throughput1 << " Mbps" << endl;
    cout << "Flow 2 throughput: " << throughput2 << " Mbps" << endl;

    Simulator::Destroy();
}

int main()
{
    RunExperiment();
    return 0;
}
