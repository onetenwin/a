#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <map>

using namespace ns3;
using namespace std;

int main()
{
    // 1. Create Nodes
    NodeContainer apNode, staNodes;
    apNode.Create(1);
    staNodes.Create(1);

    // 2. Wi-Fi Configuration
    YansWifiChannelHelper wifiChannel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(16.0));
    wifiPhy.Set("TxPowerEnd", DoubleValue(16.0));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("HtMcs7"),
        "ControlMode", StringValue("HtMcs0"));

    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("campus-wifi");

    wifiMac.SetType("ns3::ApWifiMac",
        "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice =
        wifi.Install(wifiPhy, wifiMac, apNode);

    wifiMac.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices =
        wifi.Install(wifiPhy, wifiMac, staNodes);

    // 3. Mobility Models
    MobilityHelper mobility;

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(apNode);

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(10.0),
        "MinY", DoubleValue(10.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(5.0),
        "GridWidth", UintegerValue(5),
        "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(
            Rectangle(0, 100, 0, 100)),
        "Speed", StringValue(
            "ns3::ConstantRandomVariable[Constant=5.0]"));

    mobility.Install(staNodes);

    // 4. Internet Stack
    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);

    // 5. IP Addresses
    NetDeviceContainer devices;
    devices.Add(apDevice);
    devices.Add(staDevices);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    // 6. UDP Echo Server
    uint16_t port = 9;

    UdpEchoServerHelper server(port);

    ApplicationContainer serverApp =
        server.Install(staNodes.Get(0));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // 7. UDP Echo Client
    UdpEchoClientHelper client(
        interfaces.GetAddress(1), port);

    client.SetAttribute("MaxPackets",
        UintegerValue(1000));

    client.SetAttribute("Interval",
        TimeValue(Seconds(0.1)));

    client.SetAttribute("PacketSize",
        UintegerValue(512));

    ApplicationContainer clientApp =
        client.Install(apNode.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // 8. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl =
        DynamicCast<Ipv4FlowClassifier>(
            fmh.GetClassifier());

    map<FlowId, FlowMonitor::FlowStats> stats =
        mon->GetFlowStats();

    for (const auto &kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
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

        cout << "\nPackets Sent: " << s.txPackets << endl;
        cout << "Packets Received: " << s.rxPackets << endl;
        cout << "Throughput: " << mbps << " Mbps" << endl;
        cout << "Average Delay: " << delay << " ms" << endl;
        cout << "PDR: " << pdr << " %" << endl;
        cout << "Packet Loss: " << loss << " %" << endl;
    }

    Simulator::Destroy();

    return 0;
}