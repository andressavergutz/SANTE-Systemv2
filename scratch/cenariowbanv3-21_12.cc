#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/point-to-point-module.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <math.h>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("CenarioWBANv2");

//DelayJitterEstimation delay_jitter;

//----------------------------------------------------------------------------------
// Set Tag ID to classify packet in EDCA categories
// Ps.: Can be used as a trace for new packets created and sent
//----------------------------------------------------------------------------------

// chamada no trace do tx
void
TagMarker(uint8_t ac, Ptr<const Packet> p)
{
	uint8_t tid;
	switch(ac)
	{
	case AC_VO: tid = 7; break;
	case AC_VI: tid = 5; break;
	case AC_BE: tid = 3; break;
	case AC_BK: tid = 2; break;
	default: tid = 0;
	}
	QosTag qosTag;			// Tag to identify access category
	qosTag.SetTid(tid);		// Set accordingly traffic type
	p->AddPacketTag (qosTag); // on each packet

}

//----------------------------------------------------------------------------------
// Get Tag ID of EDCA categories for count how many packets the AP receive of each category
//----------------------------------------------------------------------------------

uint8_t
GetTidForPacket (Ptr<const Packet> packet)
{
	QosTag qos;
    uint8_t tid = 8;
    if (packet->PeekPacketTag (qos))
    {
    	if (qos.GetTid () < 8)
        {
    		tid = qos.GetTid ();
        }else NS_LOG_INFO("Não pode obter GetTid");
	}else NS_LOG_INFO("Não pode obter tid");
    return tid;
}

//------------------------------------------------------------------------
// Statistics class -> traces
//------------------------------------------------------------------------

class Statistics
{
public:
	Statistics ();

	// Application PacketSink/Rx and OnOffApp/Tx

	// wlanNodes
	void AppRxCallback (std::string path, Ptr<const Packet> packet, const Address &from);
	void AppTxCallback (std::string path, Ptr<const Packet> packet);

	//wbanNodes
	void AppWbanRxCallback (std::string path, Ptr<const Packet> packet, const Address &from);
	void AppWbanTxCallback (std::string path, Ptr<const Packet> packet);

	uint32_t m_packetsRxTotal;
	uint32_t m_packetsTxTotal;
	uint32_t m_packetsWbanRxTotal;
	uint32_t m_packetsWbanTxTotal;
	uint32_t m_alertasPacketsRxTotal;
	double m_delayTotalAlertas; 					//total delay dos 1ºs alertas rx 
	std::map <string, uint32_t> m_acPacketsTxTotal;	// total de pacotes tx por categoria (AC)
	std::map <string, uint32_t> m_acPacketsRxTotal; // total de pacotes rx por categoria (AC)
	std::map <string, int64_t> m_acPacketsDelayTotal;// total do atraso dos pacotes rx de cada categoria (AC)

	std::map <uint32_t,int64_t> m_delayAlertas; 		// variacao delay dos pacotes de alertas rx
	// armazena delay de todos os pacotes recebidos em cada trafego
	std::vector<int64_t> m_delayAllPacketsRxAC_BE;
	std::vector<int64_t> m_delayAllPacketsRxAC_VI;
	std::vector<int64_t> m_delayAllPacketsRxAC_VO;
};


Statistics::Statistics(){
	m_packetsRxTotal=0;
	m_packetsTxTotal=0;
	m_packetsWbanRxTotal=0;
	m_packetsWbanTxTotal=0;
	m_delayTotalAlertas=0;
	m_alertasPacketsRxTotal=0;
	m_delayAlertas.clear();
	m_acPacketsDelayTotal.clear();
	m_acPacketsRxTotal.clear();
	m_acPacketsTxTotal.clear();
	m_delayAllPacketsRxAC_BE.clear();
	m_delayAllPacketsRxAC_VI.clear();
	m_delayAllPacketsRxAC_VO.clear();
}

// packets received -wlanNodes
void
Statistics::AppRxCallback (std::string path, Ptr<const Packet> packet, const Address &from)
{
	//TODO:double timeReceived = Simulator::Now().GetMilliSeconds();
	int64_t timeReceived = Simulator::Now().GetMicroSeconds();

	// Extrai o conteudo do quadro para obter tempo de envio
	uint8_t *buffer = new uint8_t[packet->GetSize()];
	packet->CopyData (buffer, packet->GetSize());
	string data = string((char*)buffer);
	int64_t timeSent = std::atoll(data.c_str());

	int64_t d = timeReceived - timeSent;

	// get tid packet to know the priority for access category
	uint8_t tid = GetTidForPacket (packet);
	string acIndex;

	switch(tid){
		case 3:
			acIndex = "AC_BE";
			m_acPacketsRxTotal[acIndex] += 1;
			m_acPacketsDelayTotal[acIndex] += d;
			m_delayAllPacketsRxAC_BE.push_back(d);
			break;
		case 5:
			acIndex = "AC_VI";
			m_acPacketsRxTotal[acIndex] += 1;
			m_acPacketsDelayTotal[acIndex] += d;
			m_delayAllPacketsRxAC_VI.push_back(d);
			break;
		case 7:
			acIndex = "AC_VO";
			m_acPacketsRxTotal[acIndex] += 1;
			m_acPacketsDelayTotal[acIndex] += d;
			m_delayAllPacketsRxAC_VO.push_back(d);
			break;
	}
	m_packetsRxTotal ++;
}

// packtes transmitted - wlanNodes
void
Statistics::AppTxCallback (std::string path, Ptr<const Packet> packet)
{
	// get tid packet to know the priority for access category
	uint8_t tid = GetTidForPacket (packet);
	string acIndex;

	switch(tid){
		case 3:
			acIndex = "AC_BE";
			m_acPacketsTxTotal[acIndex] += 1;
			break;
		case 5:
			acIndex = "AC_VI";
			m_acPacketsTxTotal[acIndex] += 1;
			break;
		case 7:
			acIndex = "AC_VO";
			m_acPacketsTxTotal[acIndex] += 1;
			break;
	}

	m_packetsTxTotal ++;
}

// packets received -wbanNodes
void
Statistics::AppWbanRxCallback (std::string path, Ptr<const Packet> packet, const Address &from)
{
	int64_t timeNow= Simulator::Now().GetMicroSeconds();

	// Extrai o conteudo do quadro para identificar origem
	uint8_t *buffer = new uint8_t[packet->GetSize()];
	packet->CopyData (buffer, packet->GetSize());
	string data = string((char*)buffer);

	uint32_t id;
	uint32_t ordem;
	int64_t timeSentPacket;
//	stringstream ss(data); (TODO)
//	string item, skip;
//	vector<string> val;

	int field = 0;	// Separa campos e converte conteudo
	char *str = strdup(data.c_str());
	char *token;
	while ((token = strsep(&str, "|")))
		{
			switch(field)
			{
			case 0: break;
			case 1: id = std::atoi(token); break;
			case 2: timeSentPacket = std::atoll(token); break;
			case 3: ordem = std::atol(token);
			}
			field++;
		}
	free(str);

	// Extrai ID, timestamp e numPacketsSent do nó que enviou alerta
//	while(getline(ss,item,'('))
//	{
//		stringstream ss(item);
//		while(getline(ss,item,')')){ val.push_back(item);}
//	}
	//id = std::atoi(val[1].c_str());
	//timeSentPacket = std::atoll(val[2].c_str());
	int64_t dl = timeNow - timeSentPacket; //calcula atraso da aplicação

	// Armazena delay de cada pacote enviado no indice referente ao ID do nó origem
	if(m_delayAlertas.find(id) == m_delayAlertas.end()){

		m_delayAlertas[id] = dl;
		m_delayTotalAlertas += dl;
		m_alertasPacketsRxTotal ++; //total dos primeiros alertas rx

		std::ostringstream msg;
		msg<<"Recebido ("<<(data)<<") aos "<<Simulator::Now().GetSeconds()<<" segs com "<<(dl/1000)<<"ms de atraso no quadro numero " << (ordem+1);
		NS_LOG_INFO (msg.str().c_str());
	}
	m_packetsWbanRxTotal ++; // num pacotes alertas medicos rx no total
}

// packtes transmitted --wbanNodes
void
Statistics::AppWbanTxCallback (std::string path, Ptr<const Packet> packet)
{
	//delay_jitter.PrepareTx(packet);  // Tag current time on packet to compute delay and jitter on reception
}

//------------------------------------------------------------------------
// Meus alertas
//------------------------------------------------------------------------

uint32_t m_alertasPacketsTxTotal;  // total de todos os alertas tx
std::map<uint32_t,uint32_t> m_fstAlertasPacketsTxTotal; // total dos primeiros alertas tx de cada estação wban

class MeuAlertaApp : public Application
{
public:

  MeuAlertaApp ();
  virtual ~MeuAlertaApp();

  void Setup (Ptr<Node> node, uint32_t wbanNodeID, Address address, uint8_t ac, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  uint32_t		  m_nodeID;
  uint32_t        m_nodeWbanID;
  Address         m_peer;
  uint8_t 		  m_ac;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  int64_t		  m_timeAlert; //TODO
  uint32_t 		  m_bytesSentTotal;

};

MeuAlertaApp::MeuAlertaApp ()
  : m_socket (0),
    m_nodeID (0),
    m_nodeWbanID (0),
    m_peer (),
    m_ac (AC_VO),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
    m_timeAlert (0),
    m_bytesSentTotal (0)
{
}

MeuAlertaApp::~MeuAlertaApp()
{
  m_socket = 0;
}

void
MeuAlertaApp::Setup (Ptr<Node> node, uint32_t wbanNodeID, Address address, uint8_t ac, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  Ptr<Socket> socket = Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());
  m_socket = socket;
  m_nodeID = node->GetId();
  m_ac = ac;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_nodeWbanID = wbanNodeID;
  m_timeAlert = Simulator::Now().GetMicroSeconds();
}

void
MeuAlertaApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_bytesSentTotal = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MeuAlertaApp::StopApplication (void)
{

  double timenow = Simulator::Now().GetSeconds();
  std::cout << "Tempo de parada " << timenow << std::endl;

  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MeuAlertaApp::SendPacket (void)
{
  stringstream msgx;
  msgx << "Alerta em wban |" << m_nodeWbanID << "|" << m_timeAlert << "|" << m_packetsSent; //TODO
  Ptr<Packet> packet = Create<Packet>((uint8_t*) msgx.str().c_str(), m_packetSize);
  TagMarker(m_ac, packet);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }

  m_bytesSentTotal += packet->GetSize ();
  m_alertasPacketsTxTotal++;
  if(m_fstAlertasPacketsTxTotal.find(m_nodeWbanID) == m_fstAlertasPacketsTxTotal.end()){ m_fstAlertasPacketsTxTotal[m_nodeWbanID]=1; }

}

void
MeuAlertaApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MeuAlertaApp::SendPacket, this);
    }
}

//------------------------------------------------------------------------
// Fim - meus alertas
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// NodeIDTag class to mark packets with an ID
//------------------------------------------------------------------------

// define this class in a public header
class NodeIDTag : public Tag
{
public:
	static TypeId GetTypeId (void);
	virtual TypeId GetInstanceTypeId (void) const;
	virtual uint32_t GetSerializedSize (void) const;
	virtual void Serialize (TagBuffer i) const;
	virtual void Deserialize (TagBuffer i);
	virtual void Print (std::ostream &os) const;
	void SetSimpleValue (uint32_t value);
	uint32_t GetSimpleValue (void) const;
private:
	uint32_t m_simpleValue;
};

TypeId
NodeIDTag::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::NodeIDTag")
    		.SetParent<Tag> ()
    		.AddConstructor<NodeIDTag> ()
    		.AddAttribute ("NodeID",
    				"A node ID of packet sender",
    				EmptyAttributeValue (),
    				MakeUintegerAccessor (&NodeIDTag::GetSimpleValue),
    				MakeUintegerChecker<uint32_t> ())
    				;
	return tid;
}

TypeId
NodeIDTag::GetInstanceTypeId (void) const
{
	return GetTypeId ();
}

uint32_t
NodeIDTag::GetSerializedSize (void) const
{
	return 1;
}

void
NodeIDTag::Serialize (TagBuffer i) const
{
	i.WriteU8 (m_simpleValue);
}

void
NodeIDTag::Deserialize (TagBuffer i)
{
	m_simpleValue = i.ReadU32 ();
}

void
NodeIDTag::Print (std::ostream &os) const
{
	os << "v=" << (uint32_t)m_simpleValue;
}

void
NodeIDTag::SetSimpleValue (uint32_t value)
{
	m_simpleValue = value;
}

uint32_t
NodeIDTag::GetSimpleValue (void) const
{
	return m_simpleValue;
}

//----------------------------------------------------------------------------------
// Set highest priority for Medical Alerts
//----------------------------------------------------------------------------------

void
SetupHighPriority(NodeContainer nodes) // TODO
{
	// Apply new parameters for each node on NodeContainer
	for(NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++)
	{
		// Get MAC structure into node
		Ptr<NetDevice> nd = (*node)->GetDevice(0);
		Ptr<WifiNetDevice> wifi = nd->GetObject<WifiNetDevice>();
		Ptr<WifiMac> mac = wifi->GetMac();

		PointerValue ptr;
		mac->GetAttribute("DcaTxop", ptr);
		Ptr<DcaTxop> dca = ptr.Get<DcaTxop>();
		dca->SetMinCw(0);
		dca->SetMaxCw(1);
		dca->SetAifsn(2);

		// reduz o intervalo AIFS e CW de VO para tratar os alertas médicos
		for(int ac=0; ac<4; ac++)
		{
			// Set EDCA queue and behavior for AC_VO (reduce AIFS and CW)
			Ptr<EdcaTxopN> edca;
			PointerValue ptrE;
			switch(ac)
			{
			case 0: mac->GetAttribute("VO_EdcaTxopN", ptrE); break;
			case 1: mac->GetAttribute("VI_EdcaTxopN", ptrE); break;
			case 2: mac->GetAttribute("BE_EdcaTxopN", ptrE); break;
			case 3: mac->GetAttribute("BK_EdcaTxopN", ptrE); break;
			}
			edca = ptrE.Get<EdcaTxopN>();
			edca->SetMinCw(0);
			// Initial Contention Window Size
			edca->SetMaxCw(1);
			// maximum Contention Window Size
			edca->SetAifsn(2);  // AIFS is based on Slot Time
		}
	}
}

//----------------------------------------------------------------------------------
// Generate Traffic using OnOffApplication
//----------------------------------------------------------------------------------

void
GenerateTraffic(NodeContainer servers, NodeContainer clients,
		Time start, Time stop, bool UDP, uint32_t cbr_kbps, bool uplink = false,
		uint32_t size = 256, uint8_t ac = AC_BE, uint16_t port = 50000, uint32_t maxBytes = 0)
{
	//<condition> ? <operation 1> : <operation 2>
	std::string protocol = UDP ? "ns3::UdpSocketFactory" : "ns3::TcpSocketFactory";
	std::ostringstream dataRate;
	dataRate<<cbr_kbps<<"kbps";

	// Create UDP flows between clients and servers
	for(NodeContainer::Iterator server = servers.Begin(); server != servers.End(); server++)
	{
		for(NodeContainer::Iterator client = clients.Begin(); client != clients.End(); client++)
		{
			Ptr<Ipv4> destAddress;  // Get node to receive packets
			if(uplink) { destAddress = ((*server)->GetObject<Ipv4>()); }   // dest == server; client --> server
			else	     { destAddress = ((*client)->GetObject<Ipv4>()); } // dest == client; server --> client
			Ipv4Address remoteAddress (destAddress->GetAddress (1,0).GetLocal ());
			InetSocketAddress remote = InetSocketAddress (remoteAddress, port);

			// Get sender ID data
			Ptr<Node> sender; // Get node to sent packets
			sender = uplink ? (*client) : (*server);
			// uint32_t nodeID = sender->GetId();

			// Set application parameters
			Ptr<OnOffApplication> app = CreateObject<OnOffApplication> ();
			app->SetAttribute("Protocol", StringValue (protocol)); //type of protocol: UDP or TCP
			app->SetAttribute("Remote", AddressValue (remote)); //address of the destination
			app->SetAttribute("DataRate", DataRateValue (DataRate(dataRate.str().c_str()))); //taxa de dados
			app->SetAttribute("PacketSize", UintegerValue (size));

			// A distribuição de pareto faz a rede funcionar em forma de rajadas (satura a rede com BE) (TODO)
			if(ac==AC_BE)
			{
				app->SetAttribute("OnTime", StringValue ("ns3::ParetoRandomVariable[Mean=0.05][Shape=1.5]")); //duration on
				app->SetAttribute("OffTime", StringValue ("ns3::ParetoRandomVariable[Mean=0.05][Shape=1.5]")); //duration off
			} else {
				app->SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]")); //duration on
				app->SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); //duration off
			}
			app->SetAttribute ("MaxBytes", UintegerValue(maxBytes)); //number total of bytes to send
			app->TraceConnectWithoutContext("Tx",  MakeBoundCallback (&TagMarker, ac)); //mark AC tag
			app->SetStartTime(start);
			app->SetStopTime(stop);

			// Install application on sender
			sender->AddApplication(app);
		}
	}
}


//----------------------------------------------------------------------------------
// End of helpers functions
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Funções para mapeamento da prioridade
//----------------------------------------------------------------------------------

/// Função para quebrar a linha e passar para tipo float

vector<float> split(const string &s, char delim)
{
	stringstream ss(s);
	string item;
	vector<float> values;

	while(getline(ss,item,delim))
	{
		values.push_back(std::atof(item.c_str()));
	}
	return values;
}

/// Função para ler arquivo e armazenar valores em uma matriz

vector<vector<float> > leArqIndicadoresEWS(const char* pathArq){

	std::string linha;
	vector<vector<float> > ktValues; //matriz
	vector<float> valoresLin;
	ifstream arq;

	arq.open (pathArq);
	if(!arq) cout << "Nao foi possível abrir o arquivo dos indicadores EWS \n";

	//peg todas as linhas do arquivo
	while (std::getline(arq, linha)){

		//quebrar linha do arquivo em colunas e armazena num vetor
		valoresLin = split(linha, ' ');

		//armazena na mariz valores da linha
		ktValues.push_back(valoresLin);
	}
	return ktValues;
}

bool defineAlertaMedico(vector<float> KtValues_anterior, vector<float> KtValues_atual)
{
	/// inicialização das variáveis
	bool alertaMedico;

	float lim_returnrate = -0.6;
	float lim_ArAcfDensr = 0.7; // AR, ACF, DensityRatio
	float lim_sdCoeffvar = 0.6; // SD, CoefficientVar
	float lim_skew_pos = 0.55;
	float lim_skew_neg = -0.55;
	float lim_kurt = 0.2;

	int skewOK = 0;
	int returnrateOK = 0;
	int kurtOK = 0;
	int contAcf = 0;
	int contVar = 0;

	//id KtAR KtACF KtRETURNRATE KtDENSITYRATIO KtSD KtCV KtSK KtKU
	//0   1     2       3              4          5    6    7    8

	for(unsigned int i=0;i<KtValues_anterior.size();i++)
	{
		if(i>0) // pula id
		{
			if(KtValues_atual[6] == 0) // com filtro, nao avalia coeff variation
			{
				if(i==1 || i==2 || i==4){
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_ArAcfDensr)){
						contAcf++; // tem q ser igual a 3
					}else break;
				}
				else if(i==5){
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_sdCoeffvar)){
						contVar++; // tem q ser igual a 1
					}else break;
				}
				else if(i==3){ //return rate
					if((KtValues_atual[i] <= KtValues_anterior[i]) && (KtValues_atual[i] <= lim_returnrate)){
						returnrateOK = 1;
					}else returnrateOK = 0;
				}
				else if(i==7){ //skewness
					if((KtValues_atual[i] >= lim_skew_pos) || (KtValues_atual[i] <= lim_skew_neg)){
						skewOK = 1;
					}else skewOK = 0;
				}
				else{ // kurtosis
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_kurt)){
						kurtOK = 1;
					}else kurtOK = 0;
				}
			}
			else{ // sem filtro, avalia coeff variation
				if(i==1 || i==2 || i==4){
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_ArAcfDensr)){
						contAcf++; // tem q ser igual a 3
					}else break;
				}
				else if(i==5 || i==6){
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_sdCoeffvar)){
						contVar++; // tem q ser igual a 2
					}else break;
				}
				else if(i==3){ //return rate
					if((KtValues_atual[i] <= KtValues_anterior[i]) && (KtValues_atual[i] <= lim_returnrate)){
						returnrateOK = 1;
					}else returnrateOK = 0;
				}
				else if(i==7){ //skewness
					if((KtValues_atual[i] >= lim_skew_pos) || (KtValues_atual[i] <= lim_skew_neg)){
						skewOK = 1;
					}else skewOK = 0;
				}
				else{ // kurtosis
					if((KtValues_atual[i] >= KtValues_anterior[i]) && (KtValues_atual[i] >= lim_kurt)){
						kurtOK = 1;
					}else kurtOK = 0;
				}
			}

		}
	}

	if((contAcf == 3) && (skewOK == 1) && (returnrateOK == 1) && (kurtOK == 1) && (contVar == 1)) alertaMedico = true; //com filtro
	else if((contAcf == 3) && (skewOK == 1) && (returnrateOK == 1) && (kurtOK == 1) && (contVar == 2)) alertaMedico = true; //sem filtro
	else alertaMedico = false;

	return alertaMedico;
}

//----------------------------------------------------------------------------------
// End of priority functions
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Generate medical alerts
//----------------------------------------------------------------------------------

void
AnalisaDadosWBAN(Ptr<Node> wbanNode, NodeContainer medicosNodes, bool prioritizeAlert, uint32_t id)
{
	unsigned char AccessCategory = prioritizeAlert ? AC_VO : AC_BE;

   	NS_LOG_INFO ("Analisando dados dos sensores...");

	//const char* pathArq = "/home/rafael/Desktop/ns-allinone-3.24.1/ns-3.24.1/results-kendall2.csv";
	//const char* pathArq = "/home/rafael/repos/ns-3-allinone/ns-3.25/results-kendall2.csv";
	const char* pathArq = "/home/ubuntu/Documentos/ns-allinone-3.24.1/ns-3.24.1/results-kendall.csv";

	std::vector<float> KtValues_anterior;
	std::vector<float> KtValues_atual;
	bool alertaMedico;

	std::vector<std::vector<float> > m_KtValues = leArqIndicadoresEWS(pathArq);

	// inicializa vetor
	for(unsigned int j=0;j<9;j++){
		KtValues_anterior.push_back(0);
	}

	// le matriz com valores kendall
	for(unsigned int i=0;i<m_KtValues.size();i++)
	{
		for(unsigned int j=0;j<9;j++)
		{
			KtValues_atual.push_back(m_KtValues[i][j]);
		}

		if(i>0){
			alertaMedico = defineAlertaMedico(KtValues_anterior, KtValues_atual);

			if(alertaMedico == true)
			{
				double start = Simulator::Now().GetSeconds();

				Ptr<Ipv4> ipv4 = medicosNodes.Get(0)->GetObject<Ipv4>();
				Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
				Ipv4Address addri = iaddr.GetLocal ();
				Address sinkAddress (InetSocketAddress (addri, 8001));

				Ptr<MeuAlertaApp> app  = CreateObject<MeuAlertaApp> ();
				app->Setup (wbanNode, id, sinkAddress, AccessCategory, 50, 10, DataRate ("50kbps")); //50kbps
				wbanNode->AddApplication (app);
				app->SetStartTime (Seconds (0));
				app->SetStopTime (Seconds (0.5)); // 0.7 s para a aplicação terminar antes de iniciar a próxima, sem sobreposoção

				std::ostringstream msg;
				msg<<"Enviando alerta a partir de WBAN ("<<(wbanNode->GetId())<<") aos "<<start<<" seg...";
				NS_LOG_INFO (msg.str().c_str());
			}
		}

		KtValues_anterior.swap(KtValues_atual); //troca os valores de um vetor para o outro
		KtValues_atual.clear();
	}

}
//----------------------------------------------------------------------------------
// End - Generate medical alerts
//----------------------------------------------------------------------------------

//------------------------------------------------------------------------
// Experiment
//------------------------------------------------------------------------

void MySimulation(uint32_t nWifi, uint32_t nWBAN, bool priority) {

	NS_LOG_INFO ("Configurando Parametros...");

	// Set parameters
	double setupTime = 1.0;
	double simulationTime = setupTime+10.0;
	double endTime = simulationTime+2.0;

	// Create Nodes
	NodeContainer allNodes;
	NodeContainer wifiStaNodes;
	NodeContainer wifiApNodes;
	NodeContainer wbanNodes;
	NodeContainer wlanNodes;
	NodeContainer lanHospitalNodes;
	NodeContainer lanNodes;
	wifiApNodes.Create (1);			// 1 Access Point
	wbanNodes.Create (nWBAN);
	wlanNodes.Create (nWifi);
	lanHospitalNodes.Create (1);	// 1 Central Controller in Hospital
	wifiStaNodes.Add (wlanNodes);
	wifiStaNodes.Add (wbanNodes);
	lanNodes.Add (wifiApNodes);
	lanNodes.Add (lanHospitalNodes);
	allNodes.Add (wifiApNodes);
	allNodes.Add (lanHospitalNodes);
	allNodes.Add (wifiStaNodes);

	NS_LOG_INFO ("Configurando rede Wifi...");

	Config::SetDefault("ns3::WifiMacQueue::MaxDelay", TimeValue(MilliSeconds(500))); // TODO

	//----------------------------------------------------------------------------------
	// Set wifi network
	//----------------------------------------------------------------------------------

	// PHY layer
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default (); //cria meio físico
	phy.SetChannel (channel.Create ()); //no meio físico cria o canal

	WifiHelper wifiHelper;
	wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ); //WIFI_PHY_STANDARD_80211g
	wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
										"DataMode",  StringValue ("HtMcs7"), //ErpOfdmRate54Mbps
										"ControlMode", StringValue ("HtMcs0")); //ErpOfdmRate6Mbps

	// MAC layer with QoS
//	HtWifiMacHelper mac = HtWifiMacHelper::Default ();
	QosWifiMacHelper mac;
	Ssid ssid = Ssid ("ns-3-ssid");

	// Set MAC to STA nodes
	mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid),
			     "ActiveProbing", BooleanValue (false));
				//"QosSupported", BooleanValue (true));


	NetDeviceContainer staDevices;
	staDevices = wifiHelper.Install (phy, mac, wifiStaNodes);

	// Set MAC to AP nodes
	mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid),
			 "BeaconGeneration", BooleanValue (true),
		     "BeaconInterval", TimeValue (Seconds (22.5)));
		     //"QosSupported", BooleanValue (true));

	NetDeviceContainer apDevices;
	apDevices = wifiHelper.Install (phy, mac, wifiApNodes);

	// Set priority for the alert
	if(priority){ SetupHighPriority(wbanNodes); }

	// Mobility
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
	position->Add(Vector(25.0,25.0,1.5)); //ap
	position->Add(Vector(50.0,50.0,1.5)); //lanHospitalNodes

	// Positioning WiFi devices
	double angle = 2 * 3.141592653589 / nWifi;
	for(uint8_t i = 0; i < nWifi; i++)
	{
		double x = 25.0 + 15 * cos(i*angle);
		double y = 25.0 - 15 * sin(i*angle);
		position->Add(Vector(x, y, 1.5));
	}

	// Positioning WBAN devices
	angle = 2 * 3.141592653589 / nWBAN;
	for(uint8_t i = 0; i < nWBAN; i++)
	{
		double x = 25.0 + 15 * cos(i*angle);
		double y = 25.0 - 15 * sin(i*angle);
		position->Add(Vector(x, y, 1.5));
	}
	mobility.SetPositionAllocator(position);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (allNodes); //instala mobilidaed nos nós wifi

	NS_LOG_INFO ("Configurando rede P2P...");

	// PoinToPoint to lanHospitalNodes
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
	NetDeviceContainer lanDevices;
	lanDevices = p2p.Install(lanNodes);

	// Install internet
	InternetStackHelper internet;
	internet.Install (allNodes);

	// Install addresses
	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer staInterfaces;
	Ipv4InterfaceContainer apInterfaces;
	apInterfaces = address.Assign (apDevices); // 10.1.1.1
	staInterfaces = address.Assign (staDevices);
	address.SetBase ("192.168.10.0","255.255.255.0");
	Ipv4InterfaceContainer lanInterfaces;
	lanInterfaces = address.Assign (lanDevices);

	//----------------------------------------------------------------------------------
	// Define default gateway on each STA --> AP
	//----------------------------------------------------------------------------------

	// rota estática dos nós STA para o AP
	Ipv4StaticRoutingHelper ipv4Routing;
    Ptr<Ipv4> ipv4AP = wifiApNodes.Get(0)->GetObject<Ipv4>();
    Ipv4Address addrAP = ipv4AP->GetAddress(1,0).GetLocal(); //primeiro IP do AP
    for(uint8_t j = 0; j < wifiStaNodes.GetN(); j++)
      {
        Ptr<Ipv4> ipv4sta = wifiStaNodes.Get(j)->GetObject<Ipv4>();
        Ptr<Ipv4StaticRouting> staticRoutingSta = ipv4Routing.GetStaticRouting(ipv4sta);
        staticRoutingSta->SetDefaultRoute(addrAP,1);
      }


	//----------------------------------------------------------------------------------
	// Install applications
	//----------------------------------------------------------------------------------

    NS_LOG_INFO ("Instalando aplicações...");

	// flow 1:  wlanNodes -> ApNode ------- BE
	PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),8000));
	ApplicationContainer recvApps;
	recvApps.Add (packetSinkHelper.Install (allNodes));
	recvApps.Start (Seconds (setupTime));
	recvApps.Stop (Seconds (simulationTime));


	// flow 2: wbanNodes -> lanHospitalNodes  ---- alertaMedico
	PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),8001));
	ApplicationContainer recvApps2;
	recvApps2.Add (packetSinkHelper2.Install (allNodes));
	recvApps2.Start (Seconds (setupTime));
	recvApps2.Stop (Seconds (simulationTime));

	// flow 3: wlanNodes -> wifiApNodes  ---- VI
	PacketSinkHelper packetSinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),8003));
	ApplicationContainer recvApps3;
	recvApps3.Add (packetSinkHelper3.Install (allNodes));
	recvApps3.Start (Seconds (setupTime));
	recvApps3.Stop (Seconds (simulationTime));

	// flow 3: wlanNodes -> wifiApNodes  ---- VO
	PacketSinkHelper packetSinkHelper4 ("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),8004));
	ApplicationContainer recvApps4;
	recvApps4.Add (packetSinkHelper4.Install (allNodes));
	recvApps4.Start (Seconds (setupTime));
	recvApps4.Stop (Seconds (simulationTime));

    //----------------------------------------------------------------------------------
    // Gera o trafego da rede Wifi para causar interferencia no canal
    //----------------------------------------------------------------------------------

	// o canal 802.11a suporta 54Mbits, todo trafego maior q 54mbits irá gerar saturacao no canal

	uint32_t cbr = ((100.0 * 1024) / wlanNodes.GetN()) - (384+64) ;	// 100 mbits de trafego no canal

	GenerateTraffic(wifiApNodes.Get(0), wlanNodes, Seconds(setupTime),Seconds(simulationTime),
			true, cbr, true, 1450, AC_BE, 8000, 0);

	GenerateTraffic(wifiApNodes.Get(0), wlanNodes, Seconds(setupTime),Seconds(simulationTime),
			true, 384, true, 1316, AC_VI, 8003, 0);

	GenerateTraffic(wifiApNodes.Get(0), wlanNodes, Seconds(setupTime),Seconds(simulationTime),
			true, 64, true, 160, AC_VO, 8004, 0);

	GenerateTraffic(lanHospitalNodes, wbanNodes, Seconds(setupTime),Seconds(simulationTime),
			true, 1, true, 64, AC_BE, 8005, 0);

    //----------------------------------------------------------------------------------
    // Leitura arquivo com indicadores e mapeamento
    //----------------------------------------------------------------------------------
	for(uint32_t i = 0; i < wbanNodes.GetN(); i++)
	{
		//AnalisaDadosWBAN(Ptr<Node> wbanNode, NodeContainer medicosNodes, bool prioritizeAlert)
		Simulator::Schedule(Seconds(1.5 + i), &AnalisaDadosWBAN, wbanNodes.Get(i), lanHospitalNodes, priority, i);
	}

	//----------------------------------------------------------------------------------
	// Callback
	//----------------------------------------------------------------------------------

	NS_LOG_INFO ("Callbacks...");

	Statistics statistics;

	// Callback to wlanNodes
	for(NodeContainer::Iterator i = wlanNodes.Begin(); i != wlanNodes.End(); i++)
	{
		uint32_t nodeID = (*i)->GetId();

		std::ostringstream paramTx; // Callback Trace to Collect generated packets in OnOffApplication
		paramTx<<"/NodeList/"<<(nodeID)<<"/ApplicationList/*/$ns3::OnOffApplication/Tx";
		Config::Connect (paramTx.str().c_str(), MakeCallback (&Statistics::AppTxCallback, &statistics));
	}

	// Callback to wbanNodes
	for(NodeContainer::Iterator i = wbanNodes.Begin(); i != wbanNodes.End(); i++)
	{
		uint32_t nodeID = (*i)->GetId();
		std::ostringstream paramTx; // Callback Trace to Collect generated packets in OnOffApplication
		paramTx<<"/NodeList/"<<(nodeID)<<"/ApplicationList/*/$ns3::OnOffApplication/Tx";
		Config::Connect (paramTx.str().c_str(), MakeCallback (&Statistics::AppWbanTxCallback, &statistics));
	}

	// Callback to wifiApaNodes
	uint32_t nodeAP = wifiApNodes.Get(0)->GetId();
	std::ostringstream paramRx; // Callback Trace to Collect Received packets in PacketSink
	paramRx<<"/NodeList/"<<(nodeAP)<<"/ApplicationList/*/$ns3::PacketSink/Rx";
	Config::Connect (paramRx.str().c_str(), MakeCallback (&Statistics::AppRxCallback, &statistics));


	//Callback to lanHospitalNodes
	uint32_t nodeLan = lanHospitalNodes.Get(0)->GetId();
	std::ostringstream paramRxP; // Callback Trace to Collect Received packets in PacketSink
	paramRxP<<"/NodeList/"<<(nodeLan)<<"/ApplicationList/*/$ns3::PacketSink/Rx";
	Config::Connect (paramRxP.str().c_str(), MakeCallback (&Statistics::AppWbanRxCallback, &statistics));


	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Stop (Seconds (endTime));

	//----------------------------------------------------------------------------------
	// Animation Interface NetAnim
	//----------------------------------------------------------------------------------

	AnimationInterface anim ("cenariowban.xml"); // Mandatory
	for (uint32_t i = 0; i < wlanNodes.GetN (); ++i)
	{
		std::ostringstream wlanID;
		wlanID<<"DispositivoMovel_AppGeral"<<(i+1);
		anim.UpdateNodeDescription (wlanNodes.Get (i), wlanID.str().c_str());
		anim.UpdateNodeColor (wlanNodes.Get (i), 255, 0, 0);
	}
	for (uint32_t i = 0; i < wbanNodes.GetN (); ++i)
	{
		std::ostringstream wbanID;
		wbanID<<"DispositivoMovel_AppMedica"<<(i+1);
		anim.UpdateNodeDescription (wbanNodes.Get (i), wbanID.str().c_str());
		anim.UpdateNodeColor (wbanNodes.Get (i), 0, 0, 255);
	}
	for (uint32_t i = 0; i < wifiApNodes.GetN (); ++i)
	{
		anim.UpdateNodeDescription (wifiApNodes.Get (i), "AP");
		anim.UpdateNodeColor (wifiApNodes.Get (i), 0, 255, 0);
	}
	for (uint32_t i = 0; i < lanHospitalNodes.GetN (); ++i)
	{
		anim.UpdateNodeDescription (lanHospitalNodes.Get (i), "LAN-Hospital");
		anim.UpdateNodeColor (lanHospitalNodes.Get (i), 0, 255, 255);
	}

	Simulator::Run ();
	Simulator::Destroy ();

	//----------------------------------------------------------------------------------
	// Tracing
	//----------------------------------------------------------------------------------

	std::cout << "----------------- Tx e Rx -----------------" << std::endl;
	std::cout << "Pacotes TxWLAN-geral " << statistics.m_packetsTxTotal << " e RxWLAN-geral no AP "
			<< statistics.m_packetsRxTotal << std::endl;
	std::cout << "Pacotes TxWBAN-AlertaTotal " << statistics.m_packetsWbanTxTotal << " e RxWBAN-total em LAN "
			<< statistics.m_packetsWbanRxTotal << std::endl;
	std::cout << "Pacotes TxWBAN 1º-Alerta/nó " << m_fstAlertasPacketsTxTotal.size() << " e Rx em LAN "
			<< statistics.m_alertasPacketsRxTotal << std::endl;

	//----------------------------------------------------------------------------------
	// Atrasos de todos os pacotes recebidos por tráfego
	//----------------------------------------------------------------------------------

	std::cout << "----------------- Atrasos alerta médico (1ºalerta) -----------------" << std::endl;
	std::cout << "AC_VarAM1";
	for(std::map<uint32_t,int64_t>::iterator it = statistics.m_delayAlertas.begin(); it != statistics.m_delayAlertas.end(); it++)
	{
		std::cout << " " << it->second;
	}
	std::cout << std::endl;

	std::cout << "----------------- Atrasos fluxos gerais -----------------" << std::endl;
	std::cout << "AC_VarBE";
	for(std::vector<int64_t>::iterator it = statistics.m_delayAllPacketsRxAC_BE.begin(); it != statistics.m_delayAllPacketsRxAC_BE.end(); ++it){
		std::cout << " " << *it;
	}
	std::cout << std::endl;

	std::cout << "AC_VarVI";
	for(vector<int64_t>::iterator it=statistics.m_delayAllPacketsRxAC_VI.begin(); it!=statistics.m_delayAllPacketsRxAC_VI.end(); ++it){
		std::cout << " " << *it;
	}
	std::cout << std::endl;

	std::cout << "AC_VarVO";
	for(vector<int64_t>::iterator it=statistics.m_delayAllPacketsRxAC_VO.begin(); it!=statistics.m_delayAllPacketsRxAC_VO.end(); ++it){
		std::cout << " " << *it;
	}
	std::cout << std::endl;


	//-----------------------------------------------------------------------------------------------------------------------
	// metricas de atraso médio, tx de entrega e tx de perda dos Alertas médicos
	//-----------------------------------------------------------------------------------------------------------------------

	double alertsPacketsTx = m_fstAlertasPacketsTxTotal.size();
	double alertsPacketsRx = statistics.m_alertasPacketsRxTotal;

	// atraso médio = atraso total / numTotalPacotesRecebidos - OK
	double meanDelayAlerta = alertsPacketsRx == 0 ? 0 : statistics.m_delayTotalAlertas/alertsPacketsRx;

	// txEntrada (PDR) = (numPacotesRx / numPacotesTx)*100 - OK
	double entrega = alertsPacketsRx / alertsPacketsTx;
	double txEntregaAlerta = entrega*100;

	// txPerda = ((numPacotesTx - numPacotesRx)*100)/numPacotesTx - OK
	double txPerdaAlerta = (((alertsPacketsTx - alertsPacketsRx)*100)/alertsPacketsTx);

	std::cout << "--------- Trafego | AtrasoMedio | AtrasoTotal | Tx | Rx | TxEntrega | TxPerda  -------" << std::endl;

	std::cout << "AC_AM1" << " " << (meanDelayAlerta/1000) << " " << (statistics.m_delayTotalAlertas/1000) << " " <<
			alertsPacketsTx << " " << alertsPacketsRx << " " <<
			txEntregaAlerta << " " << txPerdaAlerta << std::endl;


	//-----------------------------------------------------------------------------------------------------------------------
	// metricas Trafegos Gerais (BE, VI, VO)
	//-----------------------------------------------------------------------------------------------------------------------

	for(std::map<string,uint32_t>::iterator it = statistics.m_acPacketsRxTotal.begin(); it != statistics.m_acPacketsRxTotal.end(); it++)
	{
		double packetRx = it->second;
		double packetTx = statistics.m_acPacketsTxTotal[it->first];
		double atraso = statistics.m_acPacketsDelayTotal[it->first];

		// meanDelay = AtrasoTotalPAcotesRx / numTotalPacotesRx
		double meanDelay =  atraso / packetRx / 1000;

		// txEntrada (PDR) = (numPacotesRx / numPacotesTx)*100
		double txEntrega = (packetRx/packetTx)*100;

		// txPerda = ((numPacotesTx - numPacotesRx)*100)/numPacotesTx
		double perda = (packetTx - packetRx)*100;
		double txPerda = perda/packetTx;

		std::cout << it->first << " " << meanDelay << " " << (statistics.m_acPacketsDelayTotal[it->first]/1000) << " " <<
				statistics.m_acPacketsTxTotal[it->first] << " " << it->second <<" "<< txEntrega <<" "<< txPerda << std::endl;
	}

}

int
main (int argc, char *argv[])
{

	// Set Parameters

	LogComponentEnable ("CenarioWBANv2", LOG_LEVEL_INFO);

	NS_LOG_INFO ("Inicializando...");

	uint32_t nWifi = 10;
	uint32_t nWBAN = 10;
	bool prioritize = true;
	uint32_t nRun = 20;

	CommandLine cmd;
	cmd.AddValue ("nWifi", "Number of wifi STA BE (clients) devices", nWifi);
	cmd.AddValue ("nWBAN", "Number of wifi STA WBAN (clients) devices", nWBAN);
	cmd.AddValue ("prioritize", "Apply prioritization in alerts", prioritize);
	cmd.AddValue ("nRun", "Number of run simluation", nRun);
	cmd.Parse (argc,argv);

	SeedManager::SetRun(nRun); // atualiza semente para n execuções

	MySimulation(nWifi,nWBAN,prioritize);

	NS_LOG_INFO ("Finalizando...");

	return 0;
}
