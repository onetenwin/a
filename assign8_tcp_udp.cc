// Assignment 8 : TCP + UDP over a 2-router dumbbell, with Experiments 1-4
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
using namespace ns3;

// n0 --10M,2ms-- r1 ==btl== r2 --10M,2ms-- n3     TCP: n0->n3
// n1 --10M,2ms-- r1        r2 --10M,2ms-- n2     UDP: n1->n2
static void RunSim(std::string tag, double btlMbps, double btlDelayMs, double udpMbps, uint32_t qPkts, bool trace)
{
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));

    NodeContainer n; n.Create(4);   // n0..n3
    NodeContainer r; r.Create(2);   // r1, r2
    std::string q = std::to_string(qPkts) + "p";

    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    access.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(q));

    PointToPointHelper btl;
    btl.SetDeviceAttribute("DataRate", StringValue(std::to_string(btlMbps) + "Mbps"));
    btl.SetChannelAttribute("Delay", StringValue(std::to_string(btlDelayMs) + "ms"));
    btl.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(q));

    NetDeviceContainer d0  = access.Install(n.Get(0), r.Get(0));
    NetDeviceContainer d1  = access.Install(n.Get(1), r.Get(0));
    NetDeviceContainer db  = btl.Install(r.Get(0), r.Get(1));
    NetDeviceContainer d2  = access.Install(r.Get(1), n.Get(2));
    NetDeviceContainer d3  = access.Install(r.Get(1), n.Get(3));

    InternetStackHelper internet;
    internet.Install(n);
    internet.Install(r);

    // remove default FQ-CoDel queue-disc so the 50-packet DropTail queue is what drops packets
    TrafficControlHelper tch;
    tch.Uninstall(d0); tch.Uninstall(d1); tch.Uninstall(db); tch.Uninstall(d2); tch.Uninstall(d3);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0"); ip.Assign(d0);
    ip.SetBase("10.1.2.0", "255.255.255.0"); ip.Assign(d1);
    ip.SetBase("10.1.3.0", "255.255.255.0"); ip.Assign(db);
    ip.SetBase("10.1.4.0", "255.255.255.0"); Ipv4InterfaceContainer if2 = ip.Assign(d2);
    ip.SetBase("10.1.5.0", "255.255.255.0"); Ipv4InterfaceContainer if3 = ip.Assign(d3);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ---- TCP flow n0 -> n3 (port 8080, 1024 B) ----
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 8080));
    ApplicationContainer tSink = tcpSink.Install(n.Get(3));
    tSink.Start(Seconds(0.0)); tSink.Stop(Seconds(20.0));
    BulkSendHelper tcpSrc("ns3::TcpSocketFactory", InetSocketAddress(if3.GetAddress(1), 8080));
    tcpSrc.SetAttribute("SendSize", UintegerValue(1024));
    tcpSrc.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tSrc = tcpSrc.Install(n.Get(0));
    tSrc.Start(Seconds(1.0)); tSrc.Stop(Seconds(20.0));

    // ---- UDP flow n1 -> n2 (port 9000, 512 B) ----
    UdpServerHelper udpSrv(9000);
    ApplicationContainer uSrv = udpSrv.Install(n.Get(2));
    uSrv.Start(Seconds(0.0)); uSrv.Stop(Seconds(20.0));
    UdpClientHelper udpCli(if2.GetAddress(1), 9000);
    udpCli.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    udpCli.SetAttribute("PacketSize", UintegerValue(512));
    udpCli.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / (udpMbps * 1e6))));
    ApplicationContainer uCli = udpCli.Install(n.Get(1));
    uCli.Start(Seconds(1.0)); uCli.Stop(Seconds(20.0));

    // ---- Tracing (packet-level tx/rx/drop) ----
    if (trace)
    {
        AsciiTraceHelper ascii;
        access.EnableAsciiAll(ascii.CreateFileStream("assign8-" + tag + ".tr"));
        access.EnablePcapAll("assign8-" + tag);
    }

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    double tThr = 0, tDel = 0, tLoss = 0, tRetx = 0, uThr = 0, uDel = 0, uLoss = 0;
    uint64_t tcpUniqueBytes = DynamicCast<PacketSink>(tSink.Get(0))->GetTotalRx();

    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        if (t.destinationPort != 8080 && t.destinationPort != 9000) continue;
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double mbps = dur > 0 ? s.rxBytes * 8.0 / dur / 1e6 : 0;
        double delay = s.rxPackets > 0 ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0 : 0;
        double loss = s.txPackets > 0 ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets : 0;
        if (t.destinationPort == 8080)
        {
            tThr = mbps; tDel = delay; tLoss = loss;
            // retransmission estimate: segments sent minus unique segments delivered (minus SYN/ACK/FIN)
            long uniq = (long)((tcpUniqueBytes + 1023) / 1024);
            long retx = std::max<long>(0, (long)s.txPackets - uniq - 3);
            tRetx = s.txPackets > 0 ? 100.0 * retx / s.txPackets : 0;
        }
        else { uThr = mbps; uDel = delay; uLoss = loss; }
    }

    std::cout << std::fixed << std::setprecision(3)
              << std::setw(22) << std::left << tag << std::right
              << std::setw(10) << tThr << std::setw(10) << tDel << std::setw(9) << tLoss << std::setw(9) << tRetx
              << std::setw(10) << uThr << std::setw(10) << uDel << std::setw(9) << uLoss << "\n";
    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    std::cout << std::setw(22) << std::left << "Experiment" << std::right
              << std::setw(10) << "TCP Mbps" << std::setw(10) << "TCP ms" << std::setw(9) << "TCP loss%" << std::setw(9) << "Retx%"
              << std::setw(10) << "UDP Mbps" << std::setw(10) << "UDP ms" << std::setw(9) << "UDP loss%" << "\n";
    std::cout << std::string(89, '-') << "\n";

    //          tag               btlBw btlDelay udpRate queue trace
    RunSim("baseline(2M,10ms)",     2,   10,      1.0,    50,  true);

    std::cout << "--- Exp1: bottleneck bandwidth ---\n";
    RunSim("btl=2Mbps",  2, 10, 1.0, 50, false);
    RunSim("btl=5Mbps",  5, 10, 1.0, 50, false);
    RunSim("btl=10Mbps", 10, 10, 1.0, 50, false);

    std::cout << "--- Exp2: UDP rate ---\n";
    RunSim("udp=0.5Mbps", 2, 10, 0.5, 50, false);
    RunSim("udp=1Mbps",   2, 10, 1.0, 50, false);
    RunSim("udp=2Mbps",   2, 10, 2.0, 50, false);
    RunSim("udp=5Mbps",   2, 10, 5.0, 50, false);

    std::cout << "--- Exp3: queue size ---\n";
    RunSim("queue=20p",  2, 10, 1.0, 20, false);
    RunSim("queue=50p",  2, 10, 1.0, 50, false);
    RunSim("queue=100p", 2, 10, 1.0, 100, false);

    std::cout << "--- Exp4: bottleneck propagation delay ---\n";
    RunSim("delay=5ms",  2, 5,  1.0, 50, false);
    RunSim("delay=10ms", 2, 10, 1.0, 50, false);
    RunSim("delay=20ms", 2, 20, 1.0, 50, false);
    RunSim("delay=50ms", 2, 50, 1.0, 50, false);

    std::cout << "\nNotes: Retx% is estimated as (TCP segments sent - unique segments delivered)/segments sent.\n"
              << "Bigger queue -> lower loss but higher delay (bufferbloat); bigger bottleneck -> TCP+UDP both fit;\n"
              << "UDP rate >= bottleneck starves TCP; larger propagation delay slows TCP ramp-up (RTT-bound).\n";
    return 0;
}
