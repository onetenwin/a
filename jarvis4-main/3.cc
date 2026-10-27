
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/rip-helper.h"
#include "ns3/aodv-module.h"
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

    // Create four routers, one source and one destination
    NodeContainer routers, source, destination;
    routers.Create(4);
    source.Create(1);
    destination.Create(1);

    // Install routing protocol
    InternetStackHelper internet;

    if (protocol == "RIP")
    {
        RipHelper rip;
        internet.SetRoutingHelper(rip);
    }
    else
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
    }

    internet.Install(routers);
    internet.Install(source);
    internet.Install(destination);

    // Configure links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    // Router links
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

    // Source and destination links
    NetDeviceContainer ds =
        p2p.Install(source.Get(0), routers.Get(0));

    NetDeviceContainer dd =
        p2p.Install(routers.Get(2), destination.Get(0));

    // Assign IP addresses
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

    // UDP receiver
    uint16_t port = 5000;

    Address destAddress(
        InetSocketAddress(id.GetAddress(1), port));

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory", destAddress);

    ApplicationContainer sinkApp =
        sink.Install(destination.Get(0));

    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(20));

    // Configure the main UDP sender
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

    // Additional traffic during congestion
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

    // Link failure: disable the direct R0-R2 link at 8 seconds
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

                cout << "\nDirect R0-R2 link disabled at "
                     << Simulator::Now().GetSeconds()
                     << " seconds\n";
            });
    }

    // Print routing tables before and after the failure time
    Ipv4RoutingHelper::PrintRoutingTableAllAt(
        Seconds(5), "routes-before.txt");

    Ipv4RoutingHelper::PrintRoutingTableAllAt(
        Seconds(12), "routes-after.txt");

    // FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    uint64_t totalTx = 0;
    uint64_t totalRx = 0;
    uint64_t totalLost = 0;
    double totalDelay = 0.0;

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        // Count UDP data flows sent to port 5000
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

        cout << "\nFlow ID: " << it->first << endl;
        cout << "Source IP: " << tuple.sourceAddress << endl;
        cout << "Destination IP: " << tuple.destinationAddress << endl;
        cout << "Packets Sent: " << st.txPackets << endl;
        cout << "Packets Received: " << st.rxPackets << endl;
        cout << "Packets Lost: " << st.lostPackets << endl;
    }

    cout << "\n===== PERFORMANCE RESULTS =====\n";
    cout << "Protocol: " << protocol << endl;
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

    cout << "Routing table snapshots: routes-before.txt, "
         << "routes-after.txt\n";

    Simulator::Destroy();
}

int main()
{
    // RIP experiments
    RunExperiment("RIP", "NORMAL");
    RunExperiment("RIP", "CONGESTION");
    RunExperiment("RIP", "LINK_FAILURE");

    // AODV experiments
    RunExperiment("AODV", "NORMAL");
    RunExperiment("AODV", "CONGESTION");
    RunExperiment("AODV", "LINK_FAILURE");

    cout << "\nOSPF is not available as a standard built-in "
         << "routing helper in NS-3.42.\n";

    return 0;
}
