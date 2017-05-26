'''
 ----------------------------------------------------------------------------------------

 "Copyright 2014-2015 Intel Corporation.

 The source code, information and material ("Material") contained herein is owned by Intel Corporation
 or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
 or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
 The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
 be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
 in any way without Intel's prior express written permission. No license under any patent, copyright or 
 other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
 by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
 must be express and approved by Intel in writing.

 Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
 embedded in Materials by Intel or Intel's suppliers or licensors in any way."
 -----------------------------------------------------------------------------------------
'''

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from twisted.internet import reactor
from twisted.internet.protocol import Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint, connectProtocol

import threading
import Queue
import time

COMM_HANDLE=''
SENDER_HANDLE = ''
class ClientThread(threading.Thread):
	# globalVar = True
	sendQueue = Queue.Queue()
	def __init__(self, port):
		super(ClientThread, self).__init__(name = "ClientThread")
		self.protocol = ClientProtocol()
		self.port = port

	def run(self):
		point = TCP4ClientEndpoint(reactor, "localhost", self.port)
		d = connectProtocol(point, self.protocol)
		# d.addCallback(self.gotProtocol)
		reactor.run(installSignalHandlers=False)
		pass

	def stop(self):
		# reactor.stop()
		reactor.callFromThread(reactor.stop)
		# self._stop.set()

	def stopped(self):
		# return self._stop.isSet()
		pass

	# def gotProtocol(p):
	#	 p.sendMessage("client_registration")
		# reactor.callLater(1, p.sendMessage, "This is sent in a second")
		# reactor.callLater(2, p.transport.loseConnection)

class ClientProtocol(Protocol):
	def sendMessage(self, msg):
		self.transport.write(msg)

	def dataReceived(self, data):
		print "dataReceived:%s"%data
		# self.transport.write(data)

	def connectionMade(self):
		self.transport.write("client_registration")

	def connectionLost(self, reason):
		pass


class MsgSender(threading.Thread):
	def __init__(self, protocol):
		self.alive = True
		self.protocol = protocol
		super(MsgSender, self).__init__(name="MsgSender")

	def run(self):
		while self.alive:
			try:
				sendMsg = ClientThread.sendQueue.get_nowait()
				if sendMsg == None:
					time.sleep(0.2)
				else:
					print "MsgSender:%s"%sendMsg
					reactor.callFromThread(self.send, "%s"%sendMsg)
			except Queue.Empty:
				time.sleep(0.2)
			# time.sleep(3)
			# print "MsgSender : sending heartbeat"
			# reactor.callFromThread(self.send, "heartbeat")
		pass
	def send(self, msg):
		# self.protocol.transport.write("%s"%word)
		self.protocol.sendMessage(msg)

	def stop(self):
		self.alive = False

	def restart(self):
		self.alive = True

def initial_communication():
	global COMM_HANDLE
	global SENDER_HANDLE
	thread_handle = ClientThread(50006)
	thread_handle.start()
	sender_handler = MsgSender(thread_handle.protocol)
	sender_handler.start()
	COMM_HANDLE = thread_handle
	SENDER_HANDLE = sender_handler

def send_msg(msg):
	global COMM_HANDLE
	COMM_HANDLE.sendQueue.put(msg)

def stop_network():
	global COMM_HANDLE
	global SENDER_HANDLE
	SENDER_HANDLE.stop()
	COMM_HANDLE.stop()
	# SENDER_HANDLE.stop()
# if __name__ == "__main__":
# 	thread = ClientThread()
# 	thread.start()
# 	Sender = MsgSender(thread.protocol)
# 	Sender.start()
# 	info = ['111','222','333']
# 	for i in info:
# 		thread.sendQueue.put(i)
