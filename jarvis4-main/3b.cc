
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <map>
#include <string>

using namespace ns3;
using namespace std;

void RunExperiment(string protocol, string condition)
{
    cout << "\n================================\n";
    cout << "Protocol: " << protocol << endl;
    cout << "Condition: " << condition << endl;
    cout << "================================\n";

    // 1. Create four routers, one source and one destination
    NodeContainer routers, source, destination;
    routers.Create(4);
    source.Create(1);
    destination.Create(1);

    // 2. Install Internet Stack
    InternetStackHelper internet;
    internet.Install(routers);
    internet.Install(source);
    internet.Install(destination);

    // 3. Configure links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue<Packet>",
                 "MaxSize", StringValue("50p"));

    // 4. Router links
    NetDeviceContainer d01 =
        p2p.Install(routers.Get(0), routers.Get(1));

    NetDeviceContainer d12 =
        p2p.Install(routers.Get(1), routers.Get(2));

    NetDeviceContainer d23 =
        p2p.Install(routers.Get(2), routers.Get(3));

    NetDeviceContainer d30 =
        p2p.Install(routers.Get(3), routers.Get(0));

    NetDeviceContainer d02 =
        p2p.Install(routers.Get(0), routers.Get(2));

    // 5. Source and destination links
    NetDeviceContainer ds =
        p2p.Install(source.Get(0), routers.Get(0));

    NetDeviceContainer dd =
        p2p.Install(routers.Get(2), destination.Get(0));

    // 6. Assign IP addresses
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer i01, i12, i23, i30, i02, is, id;

    address.SetBase("10.0.1.0", "255.255.255.0");
    i01 = address.Assign(d01);

    address.SetBase("10.0.2.0", "255.255.255.0");
    i12 = address.Assign(d12);

    address.SetBase("10.0.3.0", "255.255.255.0");
    i23 = address.Assign(d23);

    address.SetBase("10.0.4.0", "255.255.255.0");
    i30 = address.Assign(d30);

    address.SetBase("10.0.5.0", "255.255.255.0");
    i02 = address.Assign(d02);

    address.SetBase("10.0.6.0", "255.255.255.0");
    is = address.Assign(ds);

    address.SetBase("10.0.7.0", "255.255.255.0");
    id = address.Assign(dd);

    // 7. Configure global shortest-path routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 8. UDP receiver
    uint16_t port = 5000;

    Address destAddress(
        InetSocketAddress(id.GetAddress(1), port));

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sink.Install(destination.Get(0));

    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(20));

    // 9. Configure main UDP sender
    OnOffHelper sender(
        "ns3::UdpSocketFactory", destAddress);

    string rate = "256Kbps";

    if (condition == "CONGESTION")
    {
        rate = "2Mbps";
    }

    sender.SetAttribute("DataRate", StringValue(rate));
    sender.SetAttribute("PacketSize", UintegerValue(512));

    sender.SetAttribute(
        "OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));

    sender.SetAttribute(
        "OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer app =
        sender.Install(source.Get(0));

    app.Start(Seconds(1));
    app.Stop(Seconds(20));

    // 10. Additional traffic during congestion
    if (condition == "CONGESTION")
    {
        OnOffHelper extraSender(
            "ns3::UdpSocketFactory", destAddress);

        extraSender.SetAttribute(
            "DataRate", StringValue("2Mbps"));

        extraSender.SetAttribute(
            "PacketSize", UintegerValue(512));

        extraSender.SetAttribute(
            "OnTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=1]"));

        extraSender.SetAttribute(
            "OffTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer extra =
            extraSender.Install(routers.Get(1));

        extra.Start(Seconds(2));
        extra.Stop(Seconds(20));
    }

    // 11. Link failure: disable the R0-R2 link
    if (condition == "LINK_FAILURE")
    {
        Ptr<Ipv4> ip0 =
            routers.Get(0)->GetObject<Ipv4>();

        Ptr<Ipv4> ip2 =
            routers.Get(2)->GetObject<Ipv4>();

        uint32_t interface0 = i02.GetInterfaceIndex(0);
        uint32_t interface2 = i02.GetInterfaceIndex(1);

        Simulator::Schedule(
            Seconds(8),
            [ip0, ip2, interface0, interface2]()
            {
                ip0->SetDown(interface0);
                ip2->SetDown(interface2);

                // Recalculate global routes
                Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

                cout << "\nDirect R0-R2 link disabled at "
                     << Simulator::Now().GetSeconds()
                     << " seconds\n";
            });
    }

    // 12. Routing table snapshots
    Ipv4RoutingHelper::PrintRoutingTableAllAt(
        Seconds(5), "routes-before.txt");

    Ipv4RoutingHelper::PrintRoutingTableAllAt(
        Seconds(12), "routes-after.txt");

    // 13. Install FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // 14. Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    // 15. Calculate performance metrics
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    uint64_t totalTx = 0;
    uint64_t totalRx = 0;
    uint64_t totalLost = 0;
    double totalDelay = 0.0;
    uint64_t totalRxBytes = 0;
    double firstTx = 20.0;
    double lastRx = 0.0;

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        // Select UDP flows going to port 5000
        if (tuple.protocol != 17 ||
            tuple.destinationPort != port)
        {
            continue;
        }

        FlowMonitor::FlowStats st = it->second;

        totalTx += st.txPackets;
        totalRx += st.rxPackets;
        totalLost += st.lostPackets;
        totalDelay += st.delaySum.GetSeconds();
        totalRxBytes += st.rxBytes;

        if (st.txPackets > 0 &&
            st.timeFirstTxPacket.GetSeconds() < firstTx)
        {
            firstTx = st.timeFirstTxPacket.GetSeconds();
        }

        if (st.rxPackets > 0 &&
            st.timeLastRxPacket.GetSeconds() > lastRx)
        {
            lastRx = st.timeLastRxPacket.GetSeconds();
        }

        cout << "\nFlow ID: " << it->first << endl;
        cout << "Source IP: " << tuple.sourceAddress << endl;
        cout << "Destination IP: " << tuple.destinationAddress << endl;
        cout << "Packets Sent: " << st.txPackets << endl;
        cout << "Packets Received: " << st.rxPackets << endl;
        cout << "Packets Lost: " << st.lostPackets << endl;
    }

    cout << "\n===== PERFORMANCE RESULTS =====\n";
    cout << "Routing method: " << protocol << endl;
    cout << "Condition: " << condition << endl;
    cout << "Total Transmitted: " << totalTx << endl;
    cout << "Total Received: " << totalRx << endl;
    cout << "Total Lost: " << totalLost << endl;

    if (totalTx > 0)
    {
        cout << "PDR: "
             << (double)totalRx / totalTx * 100
             << " %\n";

        cout << "Packet Loss Rate: "
             << (double)totalLost / totalTx * 100
             << " %\n";
    }

    if (totalRx > 0)
    {
        cout << "Average End-to-End Delay: "
             << totalDelay / totalRx * 1000
             << " ms\n";
    }

    if (lastRx > firstTx)
    {
        cout << "Throughput: "
             << totalRxBytes * 8.0 /
                ((lastRx - firstTx) * 1000000.0)
             << " Mbps\n";
    }

    cout << "Routing table snapshots: routes-before.txt, "
         << "routes-after.txt\n";

    Simulator::Destroy();
}

int main()
{
    // Global shortest-path routing experiments
    RunExperiment("GLOBAL", "NORMAL");
    RunExperiment("GLOBAL", "CONGESTION");
    RunExperiment("GLOBAL", "LINK_FAILURE");

    return 0;
}
