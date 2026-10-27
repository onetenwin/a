// Assignment 11 : Heterogeneous smart-campus network (Wi-Fi + "Bluetooth" + Zigbee) vs node speed
// NOTE: ns-3 has no Bluetooth module. Bluetooth (802.15.1, 1 Mbps, short range, 2.4 GHz) is approximated
//       by a low-power 802.11b DSSS 1 Mbps link on its own channel with a high-loss propagation model.
// Zigbee = IEEE 802.15.4 (lr-wpan) + 6LoWPAN + IPv6.  Rx power for Zigbee is computed from the
//       LogDistance model used by lr-wpan (ref loss 46.6777 dB @1 m, exponent 3, Tx = 0 dBm).
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace ns3;

enum { WIFI = 0, BT = 1, ZIG = 2 };
static double   g_sigSum[3];
static uint64_t g_sigCnt[3];
static const double SIM_END = 40.0, APP_START = 2.0;

// Wi-Fi/BT: received signal strength from PHY sniffer (cheat sheet section 10)
static void SniffRx(int tech, Ptr<const Packet>, uint16_t, WifiTxVector, MpduInfo, SignalNoiseDbm sn, uint16_t)
{
    g_sigSum[tech] += sn.signal;
    g_sigCnt[tech]++;
}
// Zigbee: sample distance every 0.5 s, compute Rx power analytically
static void SampleZigbee(Ptr<Node> coord, NodeContainer mobiles)
{
    Ptr<MobilityModel> cm = coord->GetObject<MobilityModel>();
    for (uint32_t i = 0; i < mobiles.GetN(); i++)
    {
        double d = std::max(1.0, mobiles.Get(i)->GetObject<MobilityModel>()->GetDistanceFrom(cm));
        g_sigSum[ZIG] += 0.0 - (46.6777 + 30.0 * std::log10(d));
        g_sigCnt[ZIG]++;
    }
    if (Simulator::Now().GetSeconds() < SIM_END - 0.5)
        Simulator::Schedule(Seconds(0.5), &SampleZigbee, coord, mobiles);
}

struct Agg { double thrKbps, loss, delayMs; };

template <typename ClassifierT>
static Agg Collect(Ptr<FlowMonitor> mon, Ptr<ClassifierT> cl)
{
    mon->CheckForLostPackets();
    double tx = 0, rx = 0, rxB = 0, dsum = 0;
    for (const auto &kv : mon->GetFlowStats())
    {
        if (cl->FindFlow(kv.first).protocol != 17) continue; // UDP data only (skips ND/ICMPv6)
        tx += kv.second.txPackets; rx += kv.second.rxPackets;
        rxB += kv.second.rxBytes;  dsum += kv.second.delaySum.GetSeconds();
    }
    Agg a;
    a.thrKbps = rxB * 8.0 / (SIM_END - APP_START) / 1e3;
    a.loss = tx > 0 ? 100.0 * (tx - rx) / tx : 0;
    a.delayMs = rx > 0 ? dsum / rx * 1000.0 : 0;
    return a;
}

static void SetupRandomWalk(MobilityHelper &m, double speed, double bound, double gridStart, double gridDelta)
{
    m.SetPositionAllocator("ns3::GridPositionAllocator",
                           "MinX", DoubleValue(gridStart), "MinY", DoubleValue(gridStart),
                           "DeltaX", DoubleValue(gridDelta), "DeltaY", DoubleValue(gridDelta),
                           "GridWidth", UintegerValue(3), "LayoutType", StringValue("RowFirst"));
    m.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                       "Bounds", RectangleValue(Rectangle(-bound, bound, -bound, bound)),
                       "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
}

static void RunOne(double speed)
{
    for (int i = 0; i < 3; i++) { g_sigSum[i] = 0; g_sigCnt[i] = 0; }
    const uint32_t NM = 3; // mobile nodes per technology

    // ================= WI-FI (802.11n) =================
    NodeContainer wifiAp, wifiSta; wifiAp.Create(1); wifiSta.Create(NM);
    // ================= BLUETOOTH (approximated) =================
    NodeContainer btAp, btSta; btAp.Create(1); btSta.Create(NM);
    // ================= ZIGBEE =================
    NodeContainer zigCoord, zigMob; zigCoord.Create(1); zigMob.Create(NM);
    NodeContainer zigAll; zigAll.Add(zigCoord); zigAll.Add(zigMob);

    // ---- mobility (installed first) ----
    MobilityHelper fixedM;
    fixedM.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixedM.Install(wifiAp); fixedM.Install(btAp); fixedM.Install(zigCoord);
    MobilityHelper mw; SetupRandomWalk(mw, speed, 30.0, 5.0, 5.0);  mw.Install(wifiSta);
    MobilityHelper mb; SetupRandomWalk(mb, speed, 12.0, 2.0, 2.0);  mb.Install(btSta);
    MobilityHelper mz; SetupRandomWalk(mz, speed, 50.0, 5.0, 5.0);  mz.Install(zigMob);

    // ---- Wi-Fi devices ----
    YansWifiChannelHelper wch = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wphy;
    wphy.SetChannel(wch.Create());
    wphy.Set("TxPowerStart", DoubleValue(16.0)); wphy.Set("TxPowerEnd", DoubleValue(16.0));
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    WifiMacHelper wmac; Ssid wssid("campus-wifi");
    wmac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(wssid));
    NetDeviceContainer wApDev = wifi.Install(wphy, wmac, wifiAp);
    wmac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(wssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer wStaDev = wifi.Install(wphy, wmac, wifiSta);

    // ---- Bluetooth-like devices ----
    YansWifiChannelHelper bch;
    bch.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    bch.AddPropagationLoss("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(3.5), "ReferenceLoss", DoubleValue(40.05));
    YansWifiPhyHelper bphy;
    bphy.SetChannel(bch.Create());
    bphy.Set("TxPowerStart", DoubleValue(0.0)); bphy.Set("TxPowerEnd", DoubleValue(0.0));
    WifiHelper bt;
    bt.SetStandard(WIFI_STANDARD_80211b);
    bt.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("DsssRate1Mbps"),
                               "ControlMode", StringValue("DsssRate1Mbps"));
    WifiMacHelper bmac; Ssid bssid("campus-bt");
    bmac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(bssid));
    NetDeviceContainer bApDev = bt.Install(bphy, bmac, btAp);
    bmac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(bssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer bStaDev = bt.Install(bphy, bmac, btSta);

    // ---- Zigbee devices ----
    LrWpanHelper lrWpan;
    NetDeviceContainer lrDev = lrWpan.Install(zigAll);
    lrWpan.AssociateToPan(lrDev, 0);
    SixLowPanHelper sixLow;
    NetDeviceContainer sixDev = sixLow.Install(lrDev);

    // ---- Internet stacks + addressing ----
    InternetStackHelper v4;
    v4.Install(wifiAp); v4.Install(wifiSta); v4.Install(btAp); v4.Install(btSta);
    InternetStackHelper v6; v6.SetIpv4StackInstall(false); v6.Install(zigAll);

    Ipv4AddressHelper a4;
    a4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wApIf = a4.Assign(wApDev); a4.Assign(wStaDev);
    a4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer bApIf = a4.Assign(bApDev); a4.Assign(bStaDev);
    Ipv6AddressHelper a6;
    a6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer zIf = a6.Assign(sixDev);

    // ---- applications: every mobile node -> its gateway ----
    // Wi-Fi: 1 Mbps/node, 1024 B ; BT: 100 kbps/node, 512 B ; Zigbee: 20 kbps/node, 60 B
    struct T { NodeContainer gw; NodeContainer mob; Address addr; uint16_t port; uint32_t size; double bps; };
    T techs[3] = {
        {wifiAp, wifiSta, Address(wApIf.GetAddress(0)), 5001, 1024, 1e6},
        {btAp,   btSta,   Address(bApIf.GetAddress(0)), 5002, 512,  1e5},
        {zigCoord, zigMob, Address(zIf.GetAddress(0, 1)), 5003, 60, 2e4}};
    for (int k = 0; k < 3; k++)
    {
        UdpServerHelper srv(techs[k].port);
        ApplicationContainer sa = srv.Install(techs[k].gw.Get(0));
        sa.Start(Seconds(0.0)); sa.Stop(Seconds(SIM_END));
        UdpClientHelper cli(techs[k].addr, techs[k].port);
        cli.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        cli.SetAttribute("PacketSize", UintegerValue(techs[k].size));
        cli.SetAttribute("Interval", TimeValue(Seconds(techs[k].size * 8.0 / techs[k].bps)));
        ApplicationContainer ca = cli.Install(techs[k].mob);
        ca.Start(Seconds(APP_START)); ca.Stop(Seconds(SIM_END));
    }

    // ---- signal strength probes ----
    std::string base = "/NodeList/";
    Config::ConnectWithoutContext(base + std::to_string(wifiAp.Get(0)->GetId()) + "/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
                                  MakeBoundCallback(&SniffRx, (int)WIFI));
    Config::ConnectWithoutContext(base + std::to_string(btAp.Get(0)->GetId()) + "/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
                                  MakeBoundCallback(&SniffRx, (int)BT));
    Simulator::Schedule(Seconds(APP_START), &SampleZigbee, zigCoord.Get(0), zigMob);

    // ---- one FlowMonitor per technology ----
    FlowMonitorHelper fmW, fmB, fmZ;
    NodeContainer wAll; wAll.Add(wifiAp); wAll.Add(wifiSta);
    NodeContainer bAll; bAll.Add(btAp);   bAll.Add(btSta);
    Ptr<FlowMonitor> mW = fmW.Install(wAll);
    Ptr<FlowMonitor> mB = fmB.Install(bAll);
    Ptr<FlowMonitor> mZ = fmZ.Install(zigAll);

    Simulator::Stop(Seconds(SIM_END));
    Simulator::Run();

    Agg res[3];
    res[WIFI] = Collect(mW, DynamicCast<Ipv4FlowClassifier>(fmW.GetClassifier()));
    res[BT]   = Collect(mB, DynamicCast<Ipv4FlowClassifier>(fmB.GetClassifier()));
    res[ZIG]  = Collect(mZ, DynamicCast<Ipv6FlowClassifier>(fmZ.GetClassifier6()));

    const char *names[3] = {"Wi-Fi", "Bluetooth*", "Zigbee"};
    for (int k = 0; k < 3; k++)
    {
        double rxp = g_sigCnt[k] ? g_sigSum[k] / g_sigCnt[k] : 0;
        std::cout << std::fixed << std::setprecision(2)
                  << std::left << std::setw(8) << (std::to_string((int)speed) + " m/s") << std::setw(12) << names[k] << std::right
                  << std::setw(12) << rxp << std::setw(14) << res[k].thrKbps
                  << std::setw(10) << res[k].loss << std::setw(12) << res[k].delayMs << "\n";
    }
    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    std::cout << std::left << std::setw(8) << "Speed" << std::setw(12) << "Tech" << std::right
              << std::setw(12) << "RxPwr(dBm)" << std::setw(14) << "Thr(kbps)"
              << std::setw(10) << "Loss(%)" << std::setw(12) << "Delay(ms)" << "\n";
    std::cout << std::string(68, '-') << "\n";
    double speeds[] = {1.0, 5.0, 10.0};
    for (double s : speeds) RunOne(s);

    std::cout << "\n* Bluetooth is approximated (ns-3 has no Bluetooth module).\n"
              << "Analysis guide: higher speed -> node reaches cell edge faster/more often -> lower & more variable Rx power,\n"
              << "lower SNR -> frame errors -> more retransmissions -> lower throughput, higher loss, larger delay.\n"
              << "Wi-Fi (rate adaptation) degrades gracefully; Bluetooth-like (1 Mbps, tiny range) loses most when a node leaves range;\n"
              << "Zigbee (low rate, long range in ns-3) is least affected but has highest per-packet delay (CSMA/CA + 250 kbps).\n";
    return 0;
}
