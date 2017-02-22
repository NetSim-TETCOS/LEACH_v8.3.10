/************************************************************************************
 * Copyright (C) 2013                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "main.h"
#include "DSR.h"
#include "List.h"
#include "802_15_4.h"
int fn_NetSim_DSR_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
						 NetSim_EVENTDETAILS *pstruEventDetails_Formal,
						 char *pszAppPath_Formal,
						 char *pszWritePath_Formal,
						 int nVersion_Type,
						 void **fnPointer);
int fn_NetSim_DSR_Configure_F(void** var);
int fn_NetSim_DSR_CopyPacket_F(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket);
int fn_NetSim_DSR_FreePacket_F(NetSim_PACKET* packet);
int fn_NetSim_DSR_Metrics_F(char* filename);



/**
DSR Init function initializes the DSR parameters.
*/
_declspec(dllexport) int fn_NetSim_DSR_Init(struct stru_NetSim_Network *NETWORK_Formal,
											NetSim_EVENTDETAILS *pstruEventDetails_Formal,
											char *pszAppPath_Formal,
											char *pszWritePath_Formal,
											int nVersion_Type,
											void **fnPointer)
{
	 //To enable STATIC ARP, uncomment the line "#define _STATIC_ARP" in DSR.h
	//For more details, see the StaticARP.c file.

#ifdef _STATIC_ARP
	fn_NetSim_GenerateStaticARP(NETWORK_Formal);
#endif	
	return fn_NetSim_DSR_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,pszWritePath_Formal,nVersion_Type,fnPointer);
}
/**
This is the DSR configure function.
*/
_declspec(dllexport) int fn_NetSim_DSR_Configure(void** var)
{
	return fn_NetSim_DSR_Configure_F(var);
}
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This is the DSR Run function which gets called by the IP layer for routing the data
by the DSR Network Routing Protocol. It includes the events NETWORK_OUT_EVENT, 
NETWORK_IN_EVENT and TIMER_EVENT.

NETWORK_OUT_EVENT - 
	It process the Data Packets which arrive at the NetworkOut layer to route the packet.
NETWORK_IN_EVENT - 
	It processes Data Packets, Route Request Packets, Route Reply Packets, Route Error 
	Packets, Ack Packet
TIMER_EVENT -
	It Process the DSR Route Request timeout and the DSR Maintenance Buffer timeout.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*/
_declspec(dllexport) int fn_NetSim_DSR_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case NETWORK_OUT_EVENT:
		switch(pstruEventDetails->nSubEventType)
		{
		case 0: //Packet from upper layer or Network in
		default:
			{
				if(pstruEventDetails->pPacket->nPacketType != PacketType_Control || pstruEventDetails->pPacket->nControlDataType/100 != NW_PROTOCOL_DSR)
				{
					NETSIM_IPAddress n1,n2;
					NETSIM_IPAddress dest = pstruEventDetails->pPacket->pstruNetworkData->szDestIP;
					NETSIM_IPAddress ip = DSR_DEV_IP(pstruEventDetails->nDeviceId);
					if(ip->type != dest->type)
						return 0;
					n1=IP_NETWORK_ADDRESS(ip,
						DEVICE_INTERFACE(pstruEventDetails->nDeviceId,1)->szSubnetMask,
						DEVICE_INTERFACE(pstruEventDetails->nDeviceId,1)->prefix_len);
					n2=IP_NETWORK_ADDRESS(dest,
						DEVICE_INTERFACE(pstruEventDetails->nDeviceId,1)->szSubnetMask,
						DEVICE_INTERFACE(pstruEventDetails->nDeviceId,1)->prefix_len);

					if(!IP_COMPARE(n1,n2))
						DSR_PACKET_PROCESSING();
					else
						return 0; //Other network packet
				}
				else
				{
					//DSR control packet
					//No processing just forward
				}
			}
			break;
		}
		if(pstruEventDetails->pPacket)
			pstruEventDetails->nInterfaceId = 1;
		else
			pstruEventDetails->nInterfaceId = 0;
		break;
	case NETWORK_IN_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case 0:
			default:
				{
					switch(pstruEventDetails->pPacket->nControlDataType)
					{
					default://Data packet or other layer control packet
						DSR_PROCESS_SRC_ROUTE();
						if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
						pstruEventDetails->pPacket=NULL;
						break;
					case ctrlPacket_ROUTE_REQUEST:
						DSR_PROCESS_RREQ();
						if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
						pstruEventDetails->pPacket=NULL;
						break;
					case ctrlPacket_ROUTE_REPLY:
						DSR_PROCESS_RREP();
						if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
						pstruEventDetails->pPacket=NULL;
						break;
					case ctrlPacket_ROUTE_ERROR:
						DSR_PROCESS_RERR();
						if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
						pstruEventDetails->pPacket=NULL;

						break;
					case ctrlPacket_ACK:
						DSR_PROCESS_ACK();
						if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
						pstruEventDetails->pPacket=NULL;

					}
				}
				break;
			case subevent_PROCESS_RERR:
				DSR_PROCESS_RERR();
				if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->pstruNetworkData)
							pstruEventDetails->pPacket->pstruNetworkData->nTTL--;
				pstruEventDetails->pPacket=NULL;
			break;
			}
		}
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
				case subevent_RREQ_TIMEOUT:
					DSR_RREQ_TIMEOUT();
					
				break;
				case subevent_MAINT_TIMEOUT:
					DSR_MAINT_TIMEOUT();
				break;
			}
		}
		break;
	}
	
	return 1;
}
/**
	This function is called by NetworkStack.dll, while writing the evnt trace 
	to get the sub event as a string.
*/
_declspec(dllexport) char* fn_NetSim_DSR_Trace(NETSIM_ID nSubeventid)
{
	switch(nSubeventid)
	{
	case subevent_RREQ_TIMEOUT:
		return "DSR_RREQ_TIMEOUT";
	case subevent_MAINT_TIMEOUT:
		return "DSR_MAINT_TIMEOUT";
	case subevent_PROCESS_RERR:
		return "DSR_PROCESS_RERR";
	default:
		return "DSR_UNKNOWN";
	}
}
/**
	This function is called by NetworkStack.dll, to copy the DSR protocol
	from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_DSR_CopyPacket(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket)
{
	return fn_NetSim_DSR_CopyPacket_F(destPacket,srcPacket);
}
/**
	This function is called by NetworkStack.dll, to free the TCP protocol.
*/
_declspec(dllexport) int fn_NetSim_DSR_FreePacket(NetSim_PACKET* packet)
{
	return fn_NetSim_DSR_FreePacket_F(packet);
}
/**
This function call WLAN Metrics function in lib file to write the Metrics in Metrics.txt.	
*/
_declspec(dllexport) int fn_NetSim_DSR_Metrics(char* filename)
{
	//This #ifdef.....#endif block defines the LEACH Metrics, which shows the packets 
	//sent and received by the Network Layer and the Physicaal Layer
	//To enable LEACH, uncomment the line "#define _LEACH" in DSR.h
    #ifdef _LEACH
    	FILE* fp;
     	int i;
      	fp=fopen(filename,"a+");
       	fprintf(fp,"#LEACH Metrics\n");
        	fprintf(fp,"DeviceID\tData Sent(Network Layer)\tData Sent(Physical Layer)\tData Received(Network Layer)\tData Received(Physical Layer)\tLifetime(sec)\n");
	
        	for(i=0; i<NETWORK->nDeviceCount; i++)
         	{
          		fprintf(fp,"%d\t%d\t%d\t%d\t%d\t%f\n",NETWORK->ppstruDeviceList[i]->nDeviceId,Data_Packet_Sent_NWL[NETWORK->ppstruDeviceList[i]->nDeviceId-1],Data_Packet_Sent_PL[NETWORK->ppstruDeviceList[i]->nDeviceId-1],Data_Packet_Received_NWL[NETWORK->ppstruDeviceList[i]->nDeviceId-1],Data_Packet_Received_PL[NETWORK->ppstruDeviceList[i]->nDeviceId-1],Lifetime[NETWORK->ppstruDeviceList[i]->nDeviceId-1]/1000000);
        	}
         	fclose(fp);
     #endif
	return fn_NetSim_DSR_Metrics_F(filename);
}
/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_DSR_Finish()
{
	return fn_NetSim_DSR_Finish_F();
}
/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_DSR_ConfigPacketTrace()
{
	return "";
}
/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_DSR_WritePacketTrace()
{
	return "";
}
