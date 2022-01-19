#ifndef TIMELY_RECEIVER_H
#define TIMELY_RECEIVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "timely-header.h"
#include <map>

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup udpecho UdpEcho
 */

/**
 * \ingroup udpecho
 * \brief A Udp Echo server
 *
 * Every packet received is sent back.
 */
class TimelyReceiver : public Application 
{
public:
  static TypeId GetTypeId (void);
  TimelyReceiver ();
  virtual ~TimelyReceiver ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  std::string GetIpv4Address() const;
  

  void HandleRead (Ptr<Socket> socket);
  void CloseConnection (Address from);
  void SendAck (Ptr<Socket> socket, Address remoteAddress, TimelyHeader remoteSeqTs);
  void SendNack (Ptr<Socket> socket, Address remoteAddress, TimelyHeader remoteSeqTs);

  uint16_t m_port;
  Ptr<Socket> m_socket;
  Address m_local;
  uint32_t m_pktSize; 			// packets size. 
  
  std::map<Address, uint32_t> m_received; // latest seq number recieved.
  std::map<Address, uint32_t> m_sent; // number of ACK sent: I'm not sure if it is useful anyway!
  std::map<Address, uint64_t> m_receivedBytes; // amount of bytes recieved
  std::map<Address, Time> m_connectionStartTime; // The start of the connection

};

} // namespace ns3

#endif /* TIMELY_RECEIVER_H */

