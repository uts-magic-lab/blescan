import struct
import binascii

class iBeacon:
	def __init__(self, blescan_msg):
		self.msg = blescan_msg

	def is_valid(self):
		if len(self.msg.scan_record) < 30:
			return False
		if self.msg.scan_record[0:2] != '\x02\x01':
			return False
		if self.msg.scan_record[3:7] != '\x1a\xff\x4c\x00':
			return False
		return True

	def uuid(self):
		h = binascii.hexlify(self.msg.scan_record[9:25])
		uid = h[0:8] + '-' + h[8:12] + '-' + h[12:16] + '-' + h[16:20] + '-' + h[20:32]
		major, minor = struct.unpack('>HH', self.msg.scan_record[25:29])
		return (uid, major, minor)

	def tx(self):
		power, = struct.unpack('b', self.msg.scan_record[29])
		return power
