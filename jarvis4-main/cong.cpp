#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <map>

using namespace std;
using namespace ns3;

void RunExperiment(string algorithm, string tcpType)
{
    cout << "\n===== TCP " << algorithm << " =====" << endl;

    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TypeId::LookupByName(tcpType)));

    // 1. Create Nodes
    NodeContainer vehicles, routers, server;
    vehicles.Create(5);
    routers.Create(2);
    server.Create(1);

    // 2. Configure Point-to-Point Links
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("20ms"));
    bottleneck.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize", StringValue("50p"));

    PointToPointHelper serverLink;
    serverLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    serverLink.SetChannelAttribute("Delay", StringValue("2ms"));

    // 3. Install Devices
    NetDeviceContainer d0 = access.Install(vehicles.Get(0), routers.Get(0));
    NetDeviceContainer d1 = access.Install(vehicles.Get(1), routers.Get(0));
    NetDeviceContainer d2 = access.Install(vehicles.Get(2), routers.Get(0));
    NetDeviceContainer d3 = access.Install(vehicles.Get(3), routers.Get(0));
    NetDeviceContainer d4 = access.Install(vehicles.Get(4), routers.Get(0));

    NetDeviceContainer db =
        bottleneck.Install(routers.Get(0), routers.Get(1));

    NetDeviceContainer ds =
        serverLink.Install(routers.Get(1), server.Get(0));

    // 4. Install Internet Stack
    InternetStackHelper internet;
    internet.Install(vehicles);
    internet.Install(routers);
    internet.Install(server);

    // 5. Assign IP Addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(d0);

    address.SetBase("10.1.2.0", "255.255.255.0");
    address.Assign(d1);

    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(d2);

    address.SetBase("10.1.4.0", "255.255.255.0");
    address.Assign(d3);

    address.SetBase("10.1.5.0", "255.255.255.0");
    address.Assign(d4);

    address.SetBase("10.1.6.0", "255.255.255.0");
    address.Assign(db);

    address.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer serverInterfaces = address.Assign(ds);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 6. TCP Sink
    uint16_t port = 5000;

    PacketSinkHelper sink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApps = sink.Install(server.Get(0));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(Seconds(21));

    // 7. TCP Sources
    BulkSendHelper source(
        "ns3::TcpSocketFactory",
        InetSocketAddress(serverInterfaces.GetAddress(1), port));

    source.SetAttribute("SendSize", UintegerValue(1024));
    source.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer a0 = source.Install(vehicles.Get(0));
    ApplicationContainer a1 = source.Install(vehicles.Get(1));
    ApplicationContainer a2 = source.Install(vehicles.Get(2));
    ApplicationContainer a3 = source.Install(vehicles.Get(3));
    ApplicationContainer a4 = source.Install(vehicles.Get(4));

    a0.Start(Seconds(1)); a0.Stop(Seconds(20));
    a1.Start(Seconds(1)); a1.Stop(Seconds(20));
    a2.Start(Seconds(1)); a2.Stop(Seconds(20));
    a3.Start(Seconds(1)); a3.Stop(Seconds(20));
    a4.Start(Seconds(1)); a4.Stop(Seconds(20));

    // 8. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(21));
    Simulator::Run();

    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl =
        DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    map<FlowId, FlowMonitor::FlowStats> stats =
        mon->GetFlowStats();

    double totalThroughput = 0.0;
    double sumThroughput = 0.0;
    double sumSquares = 0.0;
    double totalDelay = 0.0;

    uint64_t totalRx = 0;
    uint64_t totalTx = 0;
    uint64_t totalLost = 0;
    int flowCount = 0;

    for (const auto &kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            cl->FindFlow(kv.first);

        if (t.protocol != 6)
            continue;

        const auto &s = kv.second;

        double dur =
            (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();

        double mbps = (dur > 0)
            ? (s.rxBytes * 8.0 / dur / 1e6) : 0;

        double delay = (s.rxPackets > 0)
            ? (s.delaySum.GetSeconds() / s.rxPackets * 1000.0) : 0;

        double loss = (s.txPackets > 0)
            ? (100.0 * s.lostPackets / s.txPackets) : 0;

        double pdr = (s.txPackets > 0)
            ? (100.0 * s.rxPackets / s.txPackets) : 0;

        cout << "\nFlow ID: " << kv.first << endl;
        cout << "Throughput: " << mbps << " Mbps" << endl;
        cout << "Average Delay: " << delay << " ms" << endl;
        cout << "PDR: " << pdr << " %" << endl;
        cout << "Packet Loss: " << loss << " %" << endl;

        totalThroughput += mbps;
        sumThroughput += mbps;
        sumSquares += mbps * mbps;
        totalDelay += s.delaySum.GetSeconds();
        totalRx += s.rxPackets;
        totalTx += s.txPackets;
        totalLost += s.lostPackets;
        flowCount++;
    }

    double lossRate = (totalTx > 0)
        ? 100.0 * totalLost / totalTx : 0;

    double avgDelay = (totalRx > 0)
        ? 1000.0 * totalDelay / totalRx : 0;

    double fairness = (flowCount > 0 && sumSquares > 0)
        ? (sumThroughput * sumThroughput) /
          (flowCount * sumSquares) : 0;

    cout << "\n===== Performance Metrics =====" << endl;
    cout << "Total Throughput: " << totalThroughput << " Mbps" << endl;
    cout << "Average Delay: " << avgDelay << " ms" << endl;
    cout << "Packet Loss: " << lossRate << " %" << endl;
    cout << "Fairness: " << fairness << endl;

    mon->SerializeToXmlFile(
        "flowmon_" + algorithm + ".xml", true, true);

    Simulator::Destroy();
}

int main()
{
    RunExperiment("Reno", "ns3::TcpNewReno");
    RunExperiment("Cubic", "ns3::TcpCubic");
    RunExperiment("BBR", "ns3::TcpBbr");

    return 0;
}