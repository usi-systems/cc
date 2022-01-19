/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2008 INRIA
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
*/
#include "timely-sender-receiver-helper.h"
#include "ns3/timely-sender.h"
#include "ns3/timely-receiver.h"
#include "ns3/timely-control.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

	TimelyReceiverHelper::	TimelyReceiverHelper() { }
	TimelySenderHelper::	TimelySenderHelper() { }
	TimelyCntrlHelper::		TimelyCntrlHelper () { }

/*RCVER*/
	TimelyReceiverHelper::TimelyReceiverHelper(uint16_t port) {
		m_factory.SetTypeId(TimelyReceiver::GetTypeId());
		SetAttribute("Port", UintegerValue(port));
	}

	void TimelyReceiverHelper::SetAttribute(std::string name, const AttributeValue &value) {
		m_factory.Set(name, value);
	}

	ApplicationContainer TimelyReceiverHelper::Install(NodeContainer c) {
		ApplicationContainer apps;
		for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
			Ptr<Node> node = *i;

			m_receiver = m_factory.Create<TimelyReceiver>();
			node->AddApplication(m_receiver);
			apps.Add(m_receiver);

		}
		return apps;
	}

	Ptr<TimelyReceiver> TimelyReceiverHelper::GetServer(void) {
		return m_receiver;
	}

/*SENDER*/
	TimelySenderHelper::TimelySenderHelper(Address address, uint16_t remotePort) {
		m_factory.SetTypeId(TimelySender::GetTypeId());
		SetAttribute("RemoteAddress", AddressValue(address));
		SetAttribute("RemotePort", UintegerValue(remotePort));
	}

	void TimelySenderHelper::SetAttribute(std::string name, const AttributeValue &value) {
		m_factory.Set(name, value);
	}

	Ptr<Application> TimelySenderHelper::Install(Ptr<Node> c) {
		Ptr<TimelySender> client = m_factory.Create<TimelySender>();
		c->AddApplication(client);
		return client;
	}

/*CNTRL*/
	TimelyCntrlHelper::TimelyCntrlHelper (uint16_t ctrlPort, uint16_t leaf_cnt, uint16_t node_in_rack_cnt) {
		m_factory.SetTypeId(TimelyControl::GetTypeId());
		SetAttribute("ContrlPort", UintegerValue(ctrlPort));
		LEAF_CNT = leaf_cnt;
		NODE_CNT = node_in_rack_cnt;
	}

	uint32_t TimelyCntrlHelper::GetIndex (uint16_t leafId, uint16_t nodeId){
		return (LEAF_CNT + NODE_CNT) * leafId + nodeId;
	}

	void TimelyCntrlHelper::SetAttribute(std::string name, const AttributeValue &value) {
		m_factory.Set(name, value);
	}

	void TimelyCntrlHelper::Install (Ptr<TimelySender> lpApp, Ptr<TimelySender> hpApp, uint16_t leafId, uint16_t nodeId){
		uint32_t index = GetIndex(leafId, nodeId);
		if(m_allTimelyControlApps.find(index) == m_allTimelyControlApps.end()){
			std::cout << "ERROR: no contrl app found >> leafId:" << leafId << ", nodeId:" << nodeId << std::endl; exit(0);
		}
		Ptr<TimelyControl> cntrlApp = m_allTimelyControlApps[index];
		hpApp->TraceConnectWithoutContext ("TotalToSend", 	MakeBoundCallback (&cntrlApp->NewAnnouncmentReq, cntrlApp, hpApp));
		cntrlApp->AddAnnouncementListener(lpApp);
	}

	Ptr<Application> TimelyCntrlHelper::Install (Ptr<Node> c, uint16_t leafId, uint16_t nodeId) {
		uint32_t index = GetIndex(leafId, nodeId);
		SetAttribute("NodeId", UintegerValue(index));
		if(m_allTimelyControlApps.find(index) != m_allTimelyControlApps.end()){
			std::cout << "ERROR: Installing a contorl app twice >> leafId:" << leafId << ", nodeId:" << nodeId << std::endl;
			exit(0);
		}
		Ptr<TimelyControl> client = m_factory.Create<TimelyControl>();
		c->AddApplication(client);
		m_allTimelyControlApps[index] = client;
		return client;
	}


} // namespace ns3
