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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef TIMELY_HEADER_H
#define TIMELY_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
/**
 * \ingroup udpclientserver
 * \class TimelyHeader
 * \brief Packet header for Udp client/server application
 * The header is made of a 32bits sequence number followed by
 * a 64bits time stamp.
 */
class TimelyHeader : public Header
{
public:
  TimelyHeader ();
  ~TimelyHeader ();

  /**
   * \param seq the sequence number
   */
  void SetSeq (uint64_t seq);
  /**
   * \return the sequence number
   */
  uint64_t GetSeq (void) const;
  
  uint64_t GetTs() { return m_ts; };
  void SetTs(uint64_t ts) { m_ts = ts; };

  void SetPG (uint16_t pg);
  uint16_t GetPG () const;

  static TypeId GetTypeId (void);
  void SetAckNeeded() 	{ m_type = 1; }
  void SetNack() 		    { m_type = 2; }
  void SetAck() 		    { m_type = 4; }
  void SetData() 		    { m_type = 0; }
  void SetDebug() 	    { if(m_type < 100) m_type += 100; else {std::cout << "Debug Twice?!"; exit(88);}}

  bool IsAckNeeded() 	{ return m_type == 1 || m_type == 101;    }
  bool IsNack() 		  { return m_type == 2 || m_type == 102;    }
  bool IsAck() 			  { return m_type == 4 || m_type == 104;    }
  bool IsDebug()      { return m_type >= 100; }

private:
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  uint64_t  m_seq;
  uint64_t  m_ts;
  uint16_t  m_pg;
  uint8_t   m_type;
};

} // namespace ns3

#endif /* TIMELY_HEADER_H */