
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace std;
using namespace ns3;

void RunExperiment(Time delay)
{
    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Create point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(delay));

    NetDeviceContainer devices = p2p.Install(nodes);

    // Install Internet Stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    // Install UDP Server on Node 2
    uint16_t port = 5000;

    UdpServerHelper server(port);
    ApplicationContainer serverApps = server.Install(nodes.Get(1));

    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));

    // Install UDP Client on Node 1
    uint32_t packetSize = 1000;

    UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", UintegerValue(100000));
    client.SetAttribute("PacketSize", UintegerValue(packetSize));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(8)));

    ApplicationContainer clientApps = client.Install(nodes.Get(0));

    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(9.0));

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Calculate throughput
    Ptr<UdpServer> serverPtr =
        DynamicCast<UdpServer>(serverApps.Get(0));

    uint32_t receivedPackets = serverPtr->GetReceived();

    double receivedBytes = receivedPackets * packetSize;
    double throughput =
        (receivedBytes * 8.0) / (8.0 * 1000000.0);

    cout << "Latency: " << delay.GetMilliSeconds()
         << " ms, Throughput: " << throughput
         << " Mbps" << endl;

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
