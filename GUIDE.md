# NS-3 Lab Guide (NS-3.42)

All programs are in the `code/` folder. Each assignment = one `.cc` file.

| Question | File | Run name |
|---|---|---|
| NS3_Exp1 – Simple network (4 nodes, 2 UDP flows) | exp1_simple_network.cc | `exp1_simple_network` |
| UDP traffic & port-based delivery | udp_ports.cc | `udp_ports` |
| Assignment 8 – TCP & UDP, bottleneck experiments | assign8_tcp_udp.cc | `assign8_tcp_udp` |
| Assignment 9 – TCP Reno / Cubic / BBR (VANET) | assign9_tcp_variants.cc | `assign9_tcp_variants` |
| Assignment 10 – RIP / OSPF / AODV | assign10_routing.cc (+ plot_assign10.py) | `assign10_routing` |
| Assignment 11 – Wi-Fi / Bluetooth / Zigbee mobility | assign11_wireless.cc | `assign11_wireless` |

---------------------------------------------------------------------
## 1. HOW TO RUN (Ubuntu / WSL)
---------------------------------------------------------------------
```bash
cd /mnt/c/Users/nvgok/Downloads/ns-allinone-3.42/ns-allinone-3.42/ns-3.42

# 1. scratch/ must NOT contain files with spaces or brackets, e.g. "xyz (1).cc"
ls scratch/
rm "scratch/tcp-udp-experiment1 (1).cc"        # example: delete the bad one

# 2. copy ONE assignment into scratch (all .cc in scratch get compiled!)
cp /mnt/c/Users/nvgok/Downloads/ns3/code/assign8_tcp_udp.cc scratch/

# 3. build + run  (no .cc extension)
./ns3 build
./ns3 run scratch/assign8_tcp_udp
```
After you edit a value in the .cc file, just run `./ns3 run scratch/<name>` again (it rebuilds automatically).

Command-line arguments are NOT used; all values are edited inside the .cc file (see section 2).

Output files (.pcap, .tr, .dat, .csv, .log) are created in the ns-3.42 folder.
Open `.pcap` files in Wireshark.

Assignment 10 graphs:
```bash
cp /mnt/c/Users/nvgok/Downloads/ns3/code/plot_assign10.py .
pip install matplotlib
python3 plot_assign10.py      # creates pdr.png, delay.png, updates.png
```
Assignment 9 cwnd graph:
```bash
sudo apt install gnuplot
gnuplot -e "set term png; set output 'cwnd.png'; plot 'cwnd-Reno.dat' w l, 'cwnd-Cubic.dat' w l, 'cwnd-BBR.dat' w l"
```

---------------------------------------------------------------------
## 2. WHERE TO CHANGE VALUES (per question)
---------------------------------------------------------------------
Use Ctrl+F in the file and search for the text shown in quotes.

### NS3_Exp1 – exp1_simple_network.cc
| Change | Search for | Example |
|---|---|---|
| Link data rate / delay (all 3 links) | `p2p.SetDeviceAttribute` / `p2p.SetChannelAttribute` | `"5Mbps"`, `"2ms"` |
| Flow 1 rate (n0→n2) | `source1.SetAttribute("DataRate"` | `"1Mbps"` |
| Flow 2 rate (n3→n0) | `source2.SetAttribute("DataRate"` | `"500Kbps"` |
| Packet size | `"PacketSize"` (both sources) | `512` |
| Flow start/stop time | `srcApp1.Start` / `srcApp2.Start` | `Seconds(2.0)` |
| Simulation time | `Simulator::Stop(Seconds(10.0))` and sink/source `Stop(...)` | `10.0` |
| **Throughput duration** | `double dur1`, `double dur2` | must = stop time − start time of that flow (8.0 and 7.0) |

### UDP ports – udp_ports.cc
| Change | Search for |
|---|---|
| Rate1 / Rate2 (flow 1 / flow 2) | `rate1 = 1e6, rate2 = 2e6` (bits/sec) |
| Packet size | `pktSize = 512` |
| Port numbers | `UdpServerHelper server1(5000)`, `client1(... 5000)`, `server2(6000)`, `client2(... 6000)` (change both server and client) |
| Link latency values in the table | `std::string lats[]` in `main()` e.g. `{"2ms","5ms","10ms","20ms"}` |
| Link data rate | `p2p.SetDeviceAttribute("DataRate"` → `"10Mbps"` |
| Run time | `appStart`, `appStop` |

### Assignment 8 – assign8_tcp_udp.cc
All experiments are the calls at the bottom of `main()`:
```
RunSim("tag", btlMbps, btlDelayMs, udpMbps, queuePackets, trace);
```
| Experiment | What to edit |
|---|---|
| Exp 1 – bottleneck bandwidth | 2nd argument: `2`, `5`, `10` |
| Exp 2 – UDP traffic rate | 4th argument: `0.5`, `1.0`, `2.0`, `5.0` (Mbps) |
| Exp 3 – queue size | 5th argument: `20`, `50`, `100` |
| Exp 4 – propagation delay | 3rd argument: `5`, `10`, `20`, `50` (ms) |
| Add/remove a run | copy/delete a `RunSim(...)` line |

Inside `RunSim`:
| Change | Search for |
|---|---|
| Access link rate/delay (10 Mbps, 2 ms) | `access.SetDeviceAttribute` / `access.SetChannelAttribute` |
| TCP packet size (1024) | `SegmentSize` and `SendSize` |
| UDP packet size (512) | `udpCli.SetAttribute("PacketSize"` |
| TCP variant (Reno) | `"ns3::TcpNewReno"` |
| Simulation time (20 s) | `Stop(Seconds(20.0))` (sinks, apps, Simulator) |
| Tracing file names | `EnableAsciiAll` / `EnablePcapAll` (only baseline run has `true`) |

### Assignment 9 – assign9_tcp_variants.cc
| Change | Search for |
|---|---|
| Number of vehicles | `const uint32_t NV = 5;` |
| Vehicle→RSU link | `wireless.SetDeviceAttribute` / `SetChannelAttribute` (10 Mbps, 2 ms) |
| Bottleneck (congestion) link & queue | `bottleneck.` lines: `5Mbps`, `10ms`, `MaxSize "100p"` |
| Core link to server | `core.SetDeviceAttribute` (100 Mbps, 20 ms) |
| Make congestion heavier/lighter | change bottleneck rate or NV |
| Simulation time | `30.0` in `Stop(...)` |
| Variants compared | `RunVariant("Reno", ...)`, `("Cubic", ...)`, `("BBR", ...)` in `main()` |
| Which vehicle's cwnd is traced | `"/NodeList/0/..."` in `ConnectCwnd()` (0 = vehicle 1) |

### Assignment 10 – assign10_routing.cc
| Change | Search for |
|---|---|
| Protocols compared | `protos[]` in `main()` |
| Link speeds/delays | `acc.`, `core.`, `backup.` (SetDeviceAttribute / SetChannelAttribute) |
| Normal-condition traffic | `flows.push_back({0, 0, 5000, 1.0});` → format `{srcHost, dstHost, port, Mbps}` |
| Congestion: extra flows / rates | the `if (cond == 1)` block (add more `flows.push_back`) |
| Traffic start / stop | `tStart = 10.0, tEnd = 60.0` |
| Link-failure time | `Seconds(30.0)` in the `if (cond == 2)` block |
| OSPF re-convergence delay | `Simulator::Schedule(Seconds(31.0), &Recompute)` (failure time + delay) |
| RIP update interval | `ns3::Rip::UnsolicitedRoutingUpdate` (3 s) |
| Which link fails | `iFail` = link R0–R1 (change which `assign(...)` it is stored from) |
| Routing-table snapshot times | `PrintRoutingTableAllAt(Seconds(25.0)...` and `(Seconds(40.0)...` |

Routing tables are saved to `routes-<PROTO>-cond<N>.log` (cond0 normal, 1 congestion, 2 link failure).

### Assignment 11 – assign11_wireless.cc
| Change | Search for |
|---|---|
| Node speeds (1, 5, 10 m/s) | `double speeds[]` in `main()` |
| Simulation / traffic start time | `SIM_END`, `APP_START` at top |
| Number of mobile nodes per technology | `const uint32_t NM = 3;` |
| Mobility area | `SetupRandomWalk(mw, speed, 30.0, ...)` → 2nd number = half-width of area (Wi-Fi 30, Bluetooth 12, Zigbee 50) |
| Traffic rate/packet size per technology | `T techs[3] = {...}` → `{.., port, packetSize, bitsPerSec}` |
| Wi-Fi tx power / standard | `wphy.Set("TxPowerStart"` / `wifi.SetStandard` |
| Bluetooth range (path loss) | `bch.AddPropagationLoss(... "Exponent" ...)` and `bphy.Set("TxPowerStart"` |
| Zigbee model | `LrWpanHelper`, `SixLowPanHelper`; Rx-power formula in `SampleZigbee` |

Note: ns-3 has no Bluetooth module, so Bluetooth = low-power 802.11b 1 Mbps link (marked `Bluetooth*` in output).

---------------------------------------------------------------------
## 3. COMMON PROBLEMS
---------------------------------------------------------------------
| Problem | Fix |
|---|---|
| `target name "scratch_xxx (1)" is reserved` | A file in scratch/ has spaces/brackets in its name – delete or rename it |
| Another assignment's compile error blocks yours | Keep only the .cc you are running inside `scratch/` |
| `./ns3 run scratch/one.cc` fails | Use `./ns3 run scratch/one` (no .cc) |
| Throughput looks wrong in Exp1 | Make sure `dur1`/`dur2` match the flow start/stop times |
| No packet drops in Assignment 8 | Queue-disc must be removed: keep the `tch.Uninstall(...)` lines |
| Compile error | Copy the first line containing `error:` and fix that line only |
