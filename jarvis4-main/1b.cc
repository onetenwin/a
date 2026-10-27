
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace std;
using namespace ns3;

void RunExperiment(Time delay)
{
    // 1. Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // 2. Create P2P link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(delay));

    NetDeviceContainer devices = p2p.Install(nodes);

    // Enable packet capture
    p2p.EnablePcapAll("udp-trace");

    // 3. Install Internet Stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    // 5. Install UDP Server 1 on Node 1
    uint16_t port1 = 5000;

    UdpServerHelper server1(port1);
    ApplicationContainer serverApps1 =
        server1.Install(nodes.Get(1));

    serverApps1.Start(Seconds(0.0));
    serverApps1.Stop(Seconds(10.0));

    // 6. Install UDP Server 2 on Node 1
    uint16_t port2 = 6000;

    UdpServerHelper server2(port2);
    ApplicationContainer serverApps2 =
        server2.Install(nodes.Get(1));

    serverApps2.Start(Seconds(0.0));
    serverApps2.Stop(Seconds(10.0));

    // 7. Configure UDP Client 1
    uint32_t packetSize = 1000;

    UdpClientHelper client1(
        interfaces.GetAddress(1), port1);

    client1.SetAttribute("MaxPackets",
                         UintegerValue(100000));
    client1.SetAttribute("PacketSize",
                         UintegerValue(packetSize));
    client1.SetAttribute("Interval",
                         TimeValue(MilliSeconds(8)));

    ApplicationContainer clientApps1 =
        client1.Install(nodes.Get(0));

    clientApps1.Start(Seconds(1.0));
    clientApps1.Stop(Seconds(9.0));

    // 8. Configure UDP Client 2
    UdpClientHelper client2(
        interfaces.GetAddress(1), port2);

    client2.SetAttribute("MaxPackets",
                         UintegerValue(100000));
    client2.SetAttribute("PacketSize",
                         UintegerValue(packetSize));
    client2.SetAttribute("Interval",
                         TimeValue(MilliSeconds(8)));

    ApplicationContainer clientApps2 =
        client2.Install(nodes.Get(0));

    clientApps2.Start(Seconds(1.0));
    clientApps2.Stop(Seconds(9.0));

    // 9. Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // 10. Calculate Flow 1 throughput
    Ptr<UdpServer> s1 =
        DynamicCast<UdpServer>(serverApps1.Get(0));

    double throughput1 =
        s1->GetReceived() * packetSize * 8.0
        / (8.0 * 1000000.0);

    // 11. Calculate Flow 2 throughput
    Ptr<UdpServer> s2 =
        DynamicCast<UdpServer>(serverApps2.Get(0));

    double throughput2 =
        s2->GetReceived() * packetSize * 8.0
        / (8.0 * 1000000.0);

    // 12. Print results
    cout << "Latency: " << delay.GetMilliSeconds()
         << " ms" << endl;

    cout << "Flow 1 Throughput: "
         << throughput1 << " Mbps" << endl;

    cout << "Flow 2 Throughput: "
         << throughput2 << " Mbps" << endl;

    cout << "------------------------" << endl;

    Simulator::Destroy();
}

int main()
{
    RunExperiment(MilliSeconds(2));
    RunExperiment(MilliSeconds(5));
    RunExperiment(MilliSeconds(10));
    RunExperiment(MilliSeconds(20));

    return 0;
}
