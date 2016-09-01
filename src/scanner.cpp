// Originally based on Intel Edison Playground code by Damian Kolakowski

#include "ros/ros.h"
#include "blescan/BleScan.h"
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


void send_hci_request(int socket, uint16_t ocf, int len, void* param) {
	void* status;
	struct hci_request request = {0};
	request.ogf = OGF_LE_CTL;
	request.ocf = ocf;
	request.cparam = param;
	request.clen = len;
	request.rparam = status;
	request.rlen = 1;

	if (hci_send_req(socket, &request, 1000) < 0) {
		hci_close_dev(socket);
		perror("Error communicating with Bluetooth device");
		exit(1);
	}
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "blescan_scanner");
	ros::NodeHandle n;
	ros::Publisher scanner_pub = n.advertise<blescan::BleScan>("blescan", 100);

	int device_id = hci_get_route(NULL);
	int socket = hci_open_dev(device_id);
	if (socket < 0) { 
		perror("Error opening bluetooth device");
		exit(1); 
	}

	le_set_scan_parameters_cp scan_params = {0};
	scan_params.interval = htobs(0x10);
	scan_params.window = htobs(0x10);

	send_hci_request(socket,
	                 OCF_LE_SET_SCAN_PARAMETERS,
	                 LE_SET_SCAN_PARAMETERS_CP_SIZE,
                         &scan_params);

	le_set_event_mask_cp event_mask = {0};
	for (int i = 0; i < 8; i++)
		event_mask.mask[i] = 0xFF;

	send_hci_request(socket,
	                 OCF_LE_SET_EVENT_MASK,
	                 LE_SET_EVENT_MASK_CP_SIZE,
			 &event_mask);


	le_set_scan_enable_cp scan_enable = {0};
	scan_enable.enable = 0x01;
	scan_enable.filter_dup = 0x00;

	send_hci_request(socket,
	                 OCF_LE_SET_SCAN_ENABLE,
	                 LE_SET_SCAN_ENABLE_CP_SIZE,
	                 &scan_enable);

	struct hci_filter nf;
	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	if (setsockopt(socket, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		hci_close_dev(socket);
		perror("Error setting Bluetooth socket options\n");
		exit(1);
	}

	printf("Scanning\n");

	uint8_t buffer[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event* meta_event;
	le_advertising_info* info;
	int length;

	while (ros::ok()) {
		length = read(socket, buffer, sizeof(buffer));
		if (length >= HCI_EVENT_HDR_SIZE) {
			meta_event = (evt_le_meta_event*)(buffer+HCI_EVENT_HDR_SIZE+1);
			if (meta_event->subevent == EVT_LE_ADVERTISING_REPORT) {
				uint8_t reports_count = meta_event->data[0];
				void* offset = meta_event->data + 1;
				while (reports_count--) {
					info = (le_advertising_info *)offset;
					blescan::BleScan message;
					message.header.stamp = ros::Time::now();
					for (int i=0; i<6; i++)
						message.address[5-i] = info->bdaddr.b[i];
					message.rssi = (char)info->data[info->length];
					message.scan_record.insert(message.scan_record.end(), &info->data[0], &info->data[info->length]);
					scanner_pub.publish(message);

					offset = info->data + info->length + 2;
				}
			}
		}
	}

	le_set_scan_enable_cp scan_disable = {0};
	scan_disable.enable = 0x00;

	send_hci_request(socket,
	                 OCF_LE_SET_SCAN_ENABLE,
	                 LE_SET_SCAN_ENABLE_CP_SIZE,
	                 &scan_disable);

	hci_close_dev(socket);
	return 0;
}
