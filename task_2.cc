// Team N.54

//------------------------------------
#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/gnuplot.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
//------ RNG -------------------------
// #include "rng-seed-manager.h"
// Il parametro RngRun viene passato da cmd ( --RngRun=<..>)
// come indicato da una risposta per email, ad RngRun si deve
// passare la somma delle matricole dei componenti del gruppo;
// per questo gruppo il parametro risulta:
// --RngRun = somma numeri di matricola
//------ WiFi ------------------------
#include "ns3/mobility-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
// #include "wifi-phy-standard.h"
//------ NetAnim ---------------------
#include "ns3/netanim-module.h"
//------ Trace -----------------------
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HW2_Task_2_Team_54"); // AD-HOC MODE

int
main(int argc, char* argv[])
{
    bool useRtsCts = false;  // Forza l'utilizzo dell'handshake RTS/CTS
    bool verbose = false;    // Abilita l'uso dei LOG (SRV e CLT) per UDP application
    bool useNetAnim = false; // Genera file per l'analisi su NetAnim
    string ssid = "TLC2022";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts",
                 "Force the use of Rts and Cts",
                 useRtsCts); // Scelta di useRtsCts da CMD
    cmd.AddValue("verbose",
                 "Enable the use of Logs on SRV and CLI",
                 verbose); // Scelta di verbose da CMD
    cmd.AddValue("useNetAnim",
                 "Enable file generation for NetAnim",
                 useNetAnim); // Scelta di useNetAnim da CMD
    cmd.AddValue("ssid",
                 "Choose the network name",
                 ssid); // Scelta del SSID da CMD

    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS); // Riferito a NS_LOG_INFO

    ///////////////////////////////////////////////////////////////////////////////

    UintegerValue ctsThreshold = (useRtsCts ? UintegerValue(100) : UintegerValue(2346));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThreshold);

    ///////////////////////////////////////////////////////////////////////////////

    NS_LOG_INFO("Creazione dei nodi e dei relativi container"); // STATUS LOG INFO LEVEL

    uint32_t nodesNum = 5;

    NodeContainer allNodes;
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nodesNum);
    allNodes.Add(wifiStaNodes);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);
    allNodes.Add(wifiApNode);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;

    WifiMacHelper macInfrastructureMod;
    Ssid ssid = Ssid("ns-3-ssid");
    macInfrastructureMod.SetType("ns3::StaWifiMac",
                                 "Ssid",
                                 SsidValue(ssid),
                                 "ActiveProbing",
                                 BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, macInfrastructureMod, wifiStaNodes);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, macInfrastructureMod, wifiApNode);

    NodeContainer p2pNodes;
    p2pNodes.Add(wifiApNode);
    p2pNodes.Create(1);
    allNodes.Add(p2pNodes.Get(1));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(1);
    allNodes.Add(csmaNodes.Get(1));

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    // Mobility
    NS_LOG_INFO("Creazione del mobility model");

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(10.0),
                                  "MinY",
                                  DoubleValue(10.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(2.0),
                                  "GridWidth",
                                  UintegerValue(5),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-90, 90, -90, 90)));
    mobility.Install(wifiStaNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    // AnimationInterface::SetConstantPosition(p2pNodes.Get(1), 10, 30);
    // AnimationInterface::SetConstantPosition(csmaNodes.Get(1), 10, 33);

    NS_LOG_INFO("Fine creazione del mobility model");

    Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
    Ptr<WifiRadioEnergyModel> energyModel = CreateObject<WifiRadioEnergyModel>();

    energySource->SetInitialEnergy(300);
    energyModel->SetEnergySource(energySource);
    energySource->AppendDeviceEnergyModel(energyModel);

    // aggregate energy source to node
    wifiApNode.Get(0)->AggregateObject(energySource);

    // Install internet stack

    InternetStackHelper stack;
    stack.Install(allNodes);

    // Install Ipv4 addresses

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevices);

    // Install applications

    UdpEchoServerHelper echoServer(21);
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(0)); // da controllare
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClientN3(staInterfaces.GetAddress(3), 21);
    echoClient.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientAppsN3 = echoClientN3.Install(wifiStaNodes.Get(3)); // da controllare
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(4.5));

    UdpEchoClientHelper echoClientN4(staInterfaces.GetAddress(4),21); // UDP Echo Client verso n0
    echoClientN3.SetAttribute("MaxPackets", UintegerValue(2));
    echoClientN3.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClientN3.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientAppsN4 = echoClientN4.Install(wifiStaNodes.Get(4)); // UDP Echo Client installato su n3
    cltAppN4.Start(Seconds(1.0));
    cltAppN4.Stop(Seconds(4.5));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(15.0));

    if(useRtsCts) {
        AnimationInterface.anim("wireless-task2-rts-on.xml"); 
    }
    else {
        AnimationInterface.anim("wireless-task2-rts-off.xml");
    }

    anim.UpdateNodeDescription(wifiStaNodes.Get(0), "SRV-0");
    anim.UpdateNodeColor(wifiStaNodes.Get(0), 255, 0, 0);   

    anim.UpdateNodeDescription(wifiStaNodes.Get(3),"CLI-3");
    anim.UpdagteNodeColor(wifiStaNodes.Get(3), 0, 255, 0);

    anim.UpdateNodeDescription(wifiStaNodes.Get(4),"CLI-4");
    anim.UpdateNodeColor(wifiStaNodes.Get(4), 0, 255, 0); 


    anim.UpdateNodeDescription(wifiStaNodes.Get(1),"STA-1");
    anim.UpdagteNodeColor(wifiStaNodes.Get(1), 0, 0, 255);

    anim.UpdateNodeDescription(wifiStaNodes.Get(2),"STA-2");
    anim.UpdateNodeColor(wifiStaNodes.Get(2), 0, 0, 255);
    
    
    anim.UpdateNodeDescription(wifiApNode.Get(0), "AP"); 
    anim.UpdateNodeColor(wifiApNode.Get(i), 66, 49, 137); 

    anim.EnablePacketMetadata(); 
    anim.EnableWifiMacCounters(Seconds(0), Seconds(10)); 
    anim.EnableWifiPhyCounters(Seconds(0), Seconds(10)); 

    if(verbose){
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);     //LOG abilitato per UDP SERVER (n0)
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);     //LOG abilitato per UDP CLIENT (n3, n4)
    }

        

    NS_LOG_INFO("END");        //STATUS LOG INFO LEVEL

    ///////////////////////////////////////////////////////////////////////////////

    if(useRtsCts) {  
        phy.EnablePcap("task2-on-n4.pcap", staDevices.Get(4), true, true);
        phy.EnablePcap("task2-on-ap.pcap", apDevices.Get(0), true, true);           
    }

    else {
        phy.EnablePcap("task2-off-n4.pcap", staDevices.Get(4), true, true);  
        phy.EnablePcap("task2-off-ap.pcap", apDevices.Get(0), true, true);         
    }


    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
