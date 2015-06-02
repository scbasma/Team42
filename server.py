import SocketServer
import shelve
import math
#Get graph: client sends every graph as a number of blocks of 1024 bytes, have to update object and then shelve it 
#first of all I should save the clients address and then update only that object
#client should send size to begin with to determine if it's the largest yet, and if it is update the object
#if it is not it should rather send the biggest graph available so that the client may update itself
#
class ComputeNode:

	def __init__(self, address):
		self.address = address
		self.graph = ""
		self.receiving = True
		self.size = 0
		self.nmbr_blocks = 0
		self.block_size = 1024.0
		self.taboo_list = ""
	
	def resetGraph(self):
		self.graph = ""	
	
	def setGraph(self, graph):
		self.graph += graph
	
	def setSize(self, size):
		self.size = size
	
	def setNmbrBlocks(self):
		if(self.size == 0):
			print "SIZE HAS NOT BEEN SET"
		self.nmbr_blocks = math.ceil(self.size**2/self.block_size)
	
	def decreaseNmbrBlocks(self):
		self.nmbr_blocks -= 1
	
	def getNmbrBlocks(self):
		return self.nmbr_blocks

	def  getSize(self):
		return self.size
	
	def setReceive(self, rec):
		self.receiving = rec
	
	def getReceive(self):
		return self.receiving
	
	def updateTaboo(self, taboo):
		self.taboo_list += taboo


global d
global bestGraph #represents the best ComputeNode object with key 'best'
class MyTCPHandler(SocketServer.BaseRequestHandler):

	def handle(self):
		
		d = shelve.open("ComputeNodes.db")	
		self.data = self.request.recv(1024).strip()
		
		key = self.client_address[0]

		def GraphRequest(request):
			if d.has_key('best'):
				best = d['best']
				request.sendall(str(best.getSize()))
				request.sendall(best.graph)
			else:
				request.sendall('0')
		
		def ReceiveSize(cNode, size):
			cNode.setSize(size)
			cNode.setNmbrBlocks()
			cNode.resetGraph()
			cNode.setReceive(True)
			d[key] = cNode
			if not d.has_key('best'):
				d['best'] = cNode

		def ReceiveBlock(cNode):
			if cNode.getNmbrBlocks() == 1:
				cNode.setReceive(False)
				cNode.setGraph(self.data.strip('\0'))
				cNode.decreaseNmbrBlocks()
				d[key] = cNode
				
				if cNode.getSize() > d['best'].getSize():
					if hasattr(d['best'], 'taboo_list') and hasattr(cNode, 'taboo_list'):
						taboo_list = d['best'].taboo_list
						cNode.taboo_list = ""
					d['best'] = cNode
					print "New best with size: ", cNode.getSize()
					print cNode.graph
					print
					print
			else:
				cNode.setGraph(self.data.strip('\0'))
				cNode.decreaseNmbrBlocks()
				d[key] = cNode
				
		def TabooRequest(request):
			if d.has_key('best'):
                                #print "Number of taboos: ", (str(len([d['best'].taboo_list[i:i+d['best'].getSize()] for i in range(1, len(d['best'].taboo_list), d['best'].getSize())])))
                       #         request.sendall(str(len([d['best'].taboo_list[i:i+d['best'].getSize()] for i in range(1, len(d['best'].taboo_list), d['best'].getSize())])))
                                request.sendall(str(len(d['best'].taboo_list)))
				request.sendall(d['best'].taboo_list)
			else:
				request.sendall("-1")

	
		def ReceiveTaboo(request):
			
			taboo = self.data.strip('\0')[3:]
			if d.has_key('best'):
                                best = d['best']
				if len(taboo) < best.getSize():
				        print "SENDING -1"	
                                        self.request.sendall("-1")
				else:
					self.request.sendall("0")
                                        columns = [best.taboo_list[i:i+best.getSize()] for i in range(0, len(best.taboo_list), best.getSize())]
                                        if taboo not in columns:
                                            print "Updated best with taboo"
					    best.updateTaboo(taboo)
                                            d['best'] = best
			else:
				if d.has_key(key):
					d[key].updateTaboo(taboo)
    					cNode = d[key]
					d['best'] = cNode
					self.request(sendall("0"))
				else:
					cNode = ComputeNode(key)
					cNode.updateTaboo(taboo)
					d[key] = cNode
					d['best'] = cNode	
					self.request(sendall("0"))
						
			
		print "{} wrote:".format(self.client_address[0])
		print len(self.data)
		if(int(self.data.strip('\0')[:3]) == 299):
				ReceiveTaboo(self.request)
		if(len(self.data) == 4):
			if(int(self.data.strip('\0')) == 309):
				GraphRequest(self.request)
                        
		        elif(int(self.data.strip('\0')[:3]) == 303):
				TabooRequest(self.request)
			else:	
				
				if d.has_key(key):
					cNode = d[key]
					size = int(self.data.strip('\0'))
					ReceiveSize(cNode, size)
				else:
					cNode = ComputeNode(key)
					size = int(self.data.strip('\0'))
					ReceiveSize(cNode, size)
		else:
			cNode = d[key]
			if cNode.getReceive():
				ReceiveBlock(cNode)
		
			
					
		
#		if d.has_key(key):
#			cNode = d[key]
#			if cNode.getReceive():
#				ReceiveBlock(cNode)
#				
#			else:
#				size = int(self.data.strip('\0'))
#				if size < 300:
#					ReceiveSize(cNode, size)
#		else:
#			cNode  = ComputeNode(key)
#			
#			size = int(self.data.strip('\0'))
#			if(size < 300):
#				ReceiveSize(cNode, size)
		d.close() 



if __name__ == "__main__":
	HOST, PORT = "localhost", 9999
	server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
	d = shelve.open("ComputeNodes.db")
	if d.has_key('best'):
		print "Best from db: ", d['best'].getSize()
		print "With graph: ", d['best'].graph
                print "with taboolist", d['best'].taboo_list
	d.close()
	server.serve_forever()

