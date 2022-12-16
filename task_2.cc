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
#include <iostream>

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HW2_Task_2_Team_54");      // INFRASTRUCTURE MODE

int
main(int argc, char* argv[])
{
    bool useRtsCts = false;  // Forza l'utilizzo dell'handshake RTS/CTS
    bool verbose = false;    // Abilita l'uso dei LOG (SRV e CLT) per UDP application
    bool useNetAnim = false; // Genera file per l'analisi su NetAnim
    string ssid = "TLC2022";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts","Force the use of Rts and Cts",useRtsCts); // Scelta di useRtsCts da CMD
    cmd.AddValue("verbose","Enable the use of Logs on SRV and CLI",verbose); // Scelta di verbose da CMD
    cmd.AddValue("useNetAnim","Enable file generation for NetAnim",useNetAnim); // Scelta di useNetAnim da CMD
    cmd.AddValue("ssid","Choose the network name",ssid); // Scelta del SSID da CMD

    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS); // Riferito a NS_LOG_INFO

    ///////////////////////////////////////////////////////////////////////////////

    UintegerValue ctsThreshold = (useRtsCts ? UintegerValue(100) : UintegerValue(2346));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThreshold);

    ///////////////////////////////////////////////////////////////////////////////

    NS_LOG_INFO("Creazione dei nodi e dei relativi container"); // STATUS LOG INFO LEVEL

    uint32_t staNodesNum = 5;       // Numero di nodi sta

    NodeContainer allNodes;         // Contenitore di tutti i nodi della rete WiFi Infrastructure
    NodeContainer wifiStaNodes;       // Contenitore di tutti gli sta nodes
    wifiStaNodes.Create(staNodesNum);
    allNodes.Add(wifiStaNodes);
    NodeContainer wifiApNode;       //Contenitore dell'AP
    wifiApNode.Create(1);
    allNodes.Add(wifiApNode);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();   // Definizione canale di comunicazione tra i nodi
    YansWifiPhyHelper phy;      // Definizione physical layer tra i nodi
    phy.SetChannel(channel.Create());

    WifiHelper wifi;     // Helper per creare ed installare WiFi devices

    WifiMacHelper macInfrastructureMod;
    Ssid  SSID= Ssid(ssid);
    macInfrastructureMod.SetType("ns3::StaWifiMac","Ssid",SsidValue(SSID),"ActiveProbing",BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, macInfrastructureMod, wifiStaNodes);
    macInfrastructureMod.SetType("ns3::ApWifiMac", "Ssid", SsidValue(SSID));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, macInfrastructureMod, wifiApNode);

    NS_LOG_INFO("Fine creazione topologia di rete");        //STATUS LOG INFO LEVEL

    ///////////////////////////////////////////////////////////////////////////////

    NS_LOG_INFO("Creazione del mobility model");

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-90, 90, -90, 90)));
    mobility.Install(wifiStaNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    NS_LOG_INFO("Fine creazione del mobility model");
    
    ///////////////////////////////////////////////////////////////////////////////

    NS_LOG_INFO("Creazione del blocco di indirizzi IP per ogni container definito");

    InternetStackHelper stack;      //InternetStackHelper su tutti i nodi
    stack.Install(allNodes);

    //------ IP Address -------------

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevices);

    NS_LOG_INFO("Fine definizione blocco di indirizzi IP");        //STATUS LOG INFO LEVEL

    ///////////////////////////////////////////////////////////////////////////////

    NS_LOG_INFO("START");        //STATUS LOG INFO LEVEL

    //--------------------------------------

    UdpEchoServerHelper echoServer(21);     // UDP Echo Server
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(0));  // UDP Echo Server installato su n0
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));         //Tempo di run del server

    //--------------------------------------

    UdpEchoClientHelper echoClientN3(staInterfaces.GetAddress(0), 21);  // UDP Echo Client verso n0
    echoClientN3.SetAttribute("MaxPackets", UintegerValue(2));
    echoClientN3.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClientN3.SetAttribute("PacketSize", UintegerValue(512));
    
    ApplicationContainer clientAppsN3 = echoClientN3.Install(wifiStaNodes.Get(3)); // UDP Echo Client installato su n3
    clientAppsN3.Start(Seconds(2.0));
    clientAppsN3.Stop(Seconds(4.5));

    //--------------------------------------

    UdpEchoClientHelper echoClientN4(staInterfaces.GetAddress(0),21);  // UDP Echo Client verso n0
    echoClientN4.SetAttribute("MaxPackets", UintegerValue(2));
    echoClientN4.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClientN4.SetAttribute("PacketSize", UintegerValue(512));
    
    ApplicationContainer clientAppsN4 = echoClientN4.Install(wifiStaNodes.Get(4)); // UDP Echo Client installato su n4
    clientAppsN4.Start(Seconds(1.0));
    clientAppsN4.Stop(Seconds(4.5));

    ///////////////////////////////////////////////////////////////////////////////

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(15.0));

    ///////////////////////////////////////////////////////////////////////////////

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

    ///////////////////////////////////////////////////////////////////////////////

    if(useNetAnim) {
        if(useRtsCts) {
            AnimationInterface anim ("wireless-task2-rts-on.xml"); 
            anim.UpdateNodeDescription(wifiStaNodes.Get(0), "SRV-0");   // Nodo sta n0 SRV
            anim.UpdateNodeColor(wifiStaNodes.Get(0), 255, 0, 0);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(1),"STA-1");    // Nodo sta n1 
            anim.UpdateNodeColor(wifiStaNodes.Get(1), 0, 0, 255);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(2),"STA-2");    // Nodo sta n2
            anim.UpdateNodeColor(wifiStaNodes.Get(2), 0, 0, 255);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(3),"CLI-3");    // Nodo sta client n3
            anim.UpdateNodeColor(wifiStaNodes.Get(3), 0, 255, 0);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(4),"CLI-4");    // Nodo sta client n4
            anim.UpdateNodeColor(wifiStaNodes.Get(4), 0, 255, 0);   // R, G, B

       
    
            anim.UpdateNodeDescription(wifiApNode.Get(0), "AP");    //Nodo AP
            anim.UpdateNodeColor(wifiApNode.Get(0), 66, 49, 137);   // R, G, B

            anim.EnablePacketMetadata();    // Packet Metadata
            anim.EnableWifiMacCounters(Seconds(0), Seconds(10));    // Tracing MAC
            anim.EnableWifiPhyCounters(Seconds(0), Seconds(10));    // Tracing PHY

            Simulator::Stop(Seconds(12.0));

            Simulator::Run();
            Simulator::Destroy();

            return 0;
            }
        else {
            AnimationInterface anim ("wireless-task2-rts-off.xml");
            anim.UpdateNodeDescription(wifiStaNodes.Get(0), "SRV-0");   // Nodo sta n0 SRV
            anim.UpdateNodeColor(wifiStaNodes.Get(0), 255, 0, 0);  // R, G, B 

            anim.UpdateNodeDescription(wifiStaNodes.Get(1),"STA-1");    // Nodo sta n1 
            anim.UpdateNodeColor(wifiStaNodes.Get(1), 0, 0, 255);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(2),"STA-2");    // Nodo sta n2
            anim.UpdateNodeColor(wifiStaNodes.Get(2), 0, 0, 255);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(3),"CLI-3");    // Nodo sta client n3
            anim.UpdateNodeColor(wifiStaNodes.Get(3), 0, 255, 0);   // R, G, B

            anim.UpdateNodeDescription(wifiStaNodes.Get(4),"CLI-4");    // Nodo sta client n4
            anim.UpdateNodeColor(wifiStaNodes.Get(4), 0, 255, 0);   // R, G, B

    
            anim.UpdateNodeDescription(wifiApNode.Get(0), "AP");    //Nodo AP
            anim.UpdateNodeColor(wifiApNode.Get(0), 66, 49, 137);   // R, G, B

            anim.EnablePacketMetadata();    // Packet Metadata
            anim.EnableWifiMacCounters(Seconds(0), Seconds(10));    // Tracing MAC
            anim.EnableWifiPhyCounters(Seconds(0), Seconds(10));    // Tracing PHY

            Simulator::Stop(Seconds(12.0));

            Simulator::Run();
            Simulator::Destroy();

            return 0;
            }
        }
   
    else{

        Simulator::Stop(Seconds(12.0));

        Simulator::Run();
        Simulator::Destroy();

        return 0;
        
    }   
}
