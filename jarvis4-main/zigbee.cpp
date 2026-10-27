#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <map>

using namespace ns3;
using namespace std;

int main()
{
    // 1. Create Nodes
    NodeContainer zigbeeNodes;
    zigbeeNodes.Create(2);

    // 2. Install 802.15.4 NetDevices
    LrWpanHelper lrWpan;

    NetDeviceContainer lrDevices =
        lrWpan.Install(zigbeeNodes);

    lrWpan.AssociateToPan(lrDevices, 0);

    // 3. Install 6LoWPAN
    SixLowPanHelper sixLowPan;

    NetDeviceContainer sixDevices =
        sixLowPan.Install(lrDevices);

    // 4. Install IPv6 Internet Stack
    InternetStackHelper ipStack6;
    ipStack6.SetIpv4StackInstall(false);
    ipStack6.Install(zigbeeNodes);

    // 5. Assign IPv6 Addresses
    Ipv6AddressHelper ipv6;

    ipv6.SetBase(
        Ipv6Address("2001:1::"),
        Ipv6Prefix(64));

    Ipv6InterfaceContainer interfaces =
        ipv6.Assign(sixDevices);

    // 6. UDP Server
    uint16_t port = 5000;

    UdpServerHelper server(port);

    ApplicationContainer serverApp =
        server.Install(zigbeeNodes.Get(1));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(20.0));

    // 7. UDP Client
    UdpClientHelper client(
        interfaces.GetAddress(1, 1), port);

    client.SetAttribute(
        "MaxPackets", UintegerValue(100));

    client.SetAttribute(
        "PacketSize", UintegerValue(50));

    client.SetAttribute(
        "Interval", TimeValue(Seconds(1.0)));

    ApplicationContainer clientApp =
        client.Install(zigbeeNodes.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(20.0));

    // 8. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(21.0));
    Simulator::Run();

    mon->CheckForLostPackets();

    Ptr<Ipv6FlowClassifier> cl =
        DynamicCast<Ipv6FlowClassifier>(
            fmh.GetClassifier6());

    map<FlowId, FlowMonitor::FlowStats> stats =
        mon->GetFlowStats();

    for (const auto &kv : stats)
    {
        Ipv6FlowClassifier::FiveTuple t =
            cl->FindFlow(kv.first);

        const auto &s = kv.second;

        if (t.destinationPort != port)
            continue;

        double dur =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double mbps = (dur > 0)
            ? (s.rxBytes * 8.0 / dur / 1e6) : 0;

        double delay = (s.rxPackets > 0)
            ? (s.delaySum.GetSeconds() /
               s.rxPackets * 1000.0) : 0;

        double loss = (s.txPackets > 0)
            ? (100.0 * (s.txPackets - s.rxPackets) /
               s.txPackets) : 0;

        double pdr = (s.txPackets > 0)
            ? (100.0 * s.rxPackets / s.txPackets) : 0;

        cout << "\n===== Zigbee Results =====" << endl;
        cout << "Packets Sent: " << s.txPackets << endl;
        cout << "Packets Received: " << s.rxPackets << endl;
        cout << "Throughput: " << mbps << " Mbps" << endl;
        cout << "Average Delay: " << delay << " ms" << endl;
        cout << "Packet Loss: " << loss << " %" << endl;
        cout << "PDR: " << pdr << " %" << endl;
    }

    Simulator::Destroy();

    return 0;
}