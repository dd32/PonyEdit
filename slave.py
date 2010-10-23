import os, sys, binascii, struct, stat

thefile = ''
thefilename = ''
logfile = open("log.txt", "a")

def log(t):
	logfile.write(t + '\n')
	logfile.flush()

#
#	Config & Definitions
#

dataTypes = \
{
	0x01: 'h',
	0x81: 'H',
	0x02: 'l',
	0x82: 'L'
}

#
#	DataBlock class
#

class DataBlock:
	def __init__(self):
		self.data = ''

	def setData(self, data):
		self.data = data
		self.cursor = 0

	def read(self, fmt):
		fmt = '<' + fmt
		v = struct.unpack(fmt, self.data[self.cursor:self.cursor + struct.calcsize(fmt)])
		self.cursor += struct.calcsize(fmt)
		return v

	def readString(self):
		length = self.read('L')[0]
		v = self.data[self.cursor:self.cursor + length]
		self.cursor += length
		return v

	def write(self, fmt, *args):
		self.data += struct.pack('<' + fmt, *args)

	def writeString(self, s):
		self.data += struct.pack('<L', len(s)) + s

	def getData(self):
		return self.data

	def atEnd(self):
		return self.cursor >= len(self.data)

	def readMessage(self):
		(messageId, bufferId, length) = self.read('HLL')
		if (length > len(self.data) - self.cursor): raise Exception('Faulty Message Header!')
		params = {}
		target = self.cursor + length
		while (self.cursor < target):
			(f, t) = self.read('BB')
			if (dataTypes.has_key(t)):
				d = self.read(dataTypes[t])[0]
			elif t == 0x03:
				d = self.readString()
			params[chr(f)] = d
		if (self.cursor != target):
			log('Warning: TLD message contents didn\'t match length in header!')
			self.cursor = target
		return (messageId, bufferId, params)

#
#	Message Handlers
#

#	ls
def msg_ls(params, result):
	d = params['d']
	for filename in os.listdir(d):
		s = os.stat(d + '/' + filename)
		result.writeString(filename)
		result.write('BL', stat.S_ISDIR(s.st_mode), s.st_size)

#	open
def msg_open(params, result):
	global thefilename
	global thefile
	name = params['f']
	thefilename = name
	f = open(name, "r")
	thefile = f.read()
	f.close()

#	change
def msg_change(params, result):
	global thefilename
	global thefile
	position = params['p']
	remove = params['r']
	add = params['a']
	thefile = thefile[0:position] + add + thefile[position + remove:]

#	save
def msg_save(params, result):
	global thefilename
	global thefile
	f = open(thefilename, "w")
	f.write(thefile)
	f.close()

#
#	Message Definitions
#

messageDefs = \
{
	1: msg_ls,
	2: msg_open,
	3: msg_change,
	4: msg_save,
}


#
#	Main Guts
#

def mainLoop():
	while (1):
		line = sys.stdin.readline().strip()
		try: line = binascii.a2b_base64(line)
		except:
			log('Received some bogus input: ' + line)
			continue
		block = DataBlock()
		block.setData(line)
		while (not block.atEnd()):
			reply = handleMessage(block)
			print binascii.b2a_base64(reply.getData()).strip()

def handleMessage(message):
	(messageId, bufferId, params) = message.readMessage()

	log('messageId = ' + str(messageId))
	log('Paramaters = ' + str(params))
	try:
		if (not messageDefs.has_key(messageId)): raise Exception("Invalid messageId: " + str(messageId))
		result = DataBlock()
		result.write('B', 1)
		messageDefs[messageId](params, result)
	except Exception, e:
		log('Error occurred: ' + str(e))
		err = DataBlock()
		err.write('B', 0)
		err.writeString(str(e))
		return err
	return result


#
#	Send startup info
#

log('*************************************** Starting up *********************************************')

mainLoop()
