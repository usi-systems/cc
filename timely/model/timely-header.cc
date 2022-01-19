/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "timely-header.h"

NS_LOG_COMPONENT_DEFINE ("TimelyHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TimelyHeader);


TypeId TimelyHeader::GetTypeId (void) {
	static TypeId tid = TypeId ("ns3::TimelyHeader")
		.SetParent<Header> ()
		.AddConstructor<TimelyHeader> ()
	;
	return tid;
}

TimelyHeader::TimelyHeader () {
	m_type = 0;
	m_seq = 0;
	m_ts = Simulator::Now ().GetTimeStep ();
	m_pg  = 0;
}

TimelyHeader::~TimelyHeader () {
	m_type 			= 0;
	m_seq 			= 0;
	m_ts 			= 0;
	m_pg 			= 0;
}

void TimelyHeader::SetSeq (uint64_t seq) {
	m_seq = seq;
}

uint64_t TimelyHeader::GetSeq (void) const {
	return m_seq;
}

void TimelyHeader::SetPG (uint16_t pg) {
	m_pg = pg;
}

uint16_t TimelyHeader::GetPG (void) const {
	return m_pg;
}

TypeId TimelyHeader::GetInstanceTypeId (void) const {
	return GetTypeId ();
}

void TimelyHeader::Print (std::ostream &os) const {
	//os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
	//os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << m_pg;
	os << "seq: " << m_seq << ", pg:" << m_pg << ", flag: " << (int) m_type ;
}

uint32_t TimelyHeader::GetSerializedSize (void) const {
	return 19;
}

void TimelyHeader::Serialize (Buffer::Iterator start) const {
	Buffer::Iterator i = start;
	i.WriteHtonU64 (m_seq);
	i.WriteHtonU64 (m_ts);
	i.WriteHtonU16 (m_pg);
	i.WriteU8 (m_type);
}

uint32_t TimelyHeader::Deserialize (Buffer::Iterator start) {
	Buffer::Iterator i = start;
	m_seq = 		i.ReadNtohU64 ();
	m_ts =			i.ReadNtohU64 ();
	m_pg =			i.ReadNtohU16 ();
	m_type = 	i.ReadU8();
	return GetSerializedSize ();
}

} // namespace ns3