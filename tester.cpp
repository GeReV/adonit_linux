#include <QtCore/QDebug>
#include <gatodescriptor.h>



#include "tester.h"
#include "log.h"

#define ATT_CID 4

Tester::Tester(QObject *parent)
    : QObject(parent)
{
    printf("Hello.\n");

//	manager = new GatoCentralManager(this);
//	connect(mmanager, SIGNAL(discoveredPeripheral(GatoPeripheral*,int)),
//	        SLOT(handleDiscoveredPeripheral(GatoPeripheral*,int)));

	discoverBluetoothDevice();

    fd = open("/dev/input/event14", O_RDONLY | O_NONBLOCK);
    printf("File: %d\n", fd);

    int grab = 1;
    ioctl(fd, EVIOCGRAB, &grab);

	uinput = new UInput();

}

Tester::~Tester()
{
	if (peripheral) {
		disconnect(peripheral, 0, this, 0);
		if (!agg_char.isNull()) {
			peripheral->setNotification(agg_char, false);
		}
		peripheral->disconnectPeripheral();
	}

    ioctl(fd, EVIOCGRAB, NULL);

    close(fd);

	delete uinput;
}

void Tester::closeBluetoothDevice() {
	hci_close_dev(hci);
	hci = -1;
}

void Tester::discoverBluetoothDevice() {

	int rc;

	dev_id = hci_get_route(NULL);
	hci = hci_open_dev(dev_id);

	if (hci == -1) {
		qErrnoWarning("Could not open device");
		return;
	}

	hci_le_set_scan_enable(hci, 0, 0, timeout);

	rc = hci_le_set_scan_parameters(hci, 1,
									htobs(0x0010), htobs(0x0010),
									0 /* Public address */,
									0 /* No filter ? */,
									timeout);
	if (rc < 0) {
		printf("LE Set scan parameters failed");
		closeBluetoothDevice();
		return;
	}

	rc = hci_le_set_scan_enable(hci, 1, 1, timeout);

	if (rc < 0) {
		printf("LE Set scan enable failed");
		closeBluetoothDevice();
		return;
	}

	socklen_t olen = sizeof(hci_of);
	if (getsockopt(hci, SOL_HCI, HCI_FILTER, &hci_of, &olen) < 0) {
		qErrnoWarning("Could not get existing HCI socket options");
		return;
	}

	hci_filter_clear(&hci_nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &hci_nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &hci_nf);

	if (setsockopt(hci, SOL_HCI, HCI_FILTER, &hci_nf, sizeof(hci_nf)) < 0) {
		printf("Could not set HCI socket options");
		return;
	}

	connectBluetoothDevice();
}

void Tester::connectBluetoothDevice() {
	unsigned char buf[HCI_MAX_EVENT_SIZE];

	// Read a full event
	int len;
	if ((len = read(hci, buf, sizeof(buf))) < 0) {
		if (errno != EAGAIN && errno != EINTR) {
			printf("Could not read HCI events");
		}
		return; // Will be notified later, probably.
	}

	int pos = HCI_EVENT_HDR_SIZE + 1;
	assert(pos < len);
	evt_le_meta_event *meta = reinterpret_cast<evt_le_meta_event*>(&buf[pos]);


	if (meta->subevent == EVT_LE_ADVERTISING_REPORT) {
		pos++; // Skip subevent field
		int num_reports = buf[pos];
		pos++; // Skip num_reports field

		assert(pos < len);

		while (num_reports > 0) {
			le_advertising_info *info = reinterpret_cast<le_advertising_info*>(&buf[pos]);
			assert(pos + LE_ADVERTISING_INFO_SIZE < len);
			assert(pos + LE_ADVERTISING_INFO_SIZE + info->length < len);
			int8_t *rssi = reinterpret_cast<int8_t*>(&buf[pos + LE_ADVERTISING_INFO_SIZE + info->length]);

			handleAdvertising(info, *rssi);

			pos += LE_ADVERTISING_INFO_SIZE + info->length + 1;
			num_reports--;
		}
	}
}

void Tester::handleAdvertising(le_advertising_info *info, int rssi) {
	qDebug() << "Advertising event type" << info->evt_type
			 << "address type" << info->bdaddr_type
			 << "data length" << info->length
			 << "rssi" << rssi;

	hci_fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (hci_fd == -1) {
		qErrnoWarning("Could not create L2CAP socket");
		return;
	}

	struct sockaddr_l2 l2addr;
	memset(&l2addr, 0, sizeof(l2addr));

	l2addr.l2_family = AF_BLUETOOTH;
	l2addr.l2_cid = htobs(ATT_CID);
#ifdef BDADDR_LE_PUBLIC
	l2addr.l2_bdaddr_type = BDADDR_LE_PUBLIC; // TODO
#endif
	l2addr.l2_bdaddr = info->bdaddr;

	int err = ::connect(hci_fd, reinterpret_cast<sockaddr*>(&l2addr), sizeof(l2addr));
	if (err == -1 && errno != EINPROGRESS) {
		qErrnoWarning("Could not connect to L2CAP socket");
		hci_fd = -1;
		return;
	}

	if (info->length > 0) {

		printf("We have things to parse!");
		//peripheral->parseEIR(info->data, info->length);
	}

	//uint8_t address[6] = info->bdaddr.b;

	/*GatoPeripheral *peripheral;

	QHash<GatoAddress, GatoPeripheral*>::iterator it = peripherals.find(addr);

	if (it == peripherals.end()) {
		peripheral = new GatoPeripheral(addr, q);
		peripherals.insert(addr, peripheral);
	} else {
		peripheral = *it;
	}

	if (info->length > 0) {
		peripheral->parseEIR(info->data, info->length);
	}

	bool passes_filter;
	if (filter_uuids.isEmpty()) {
		passes_filter = true;
	} else {
		passes_filter = false;
		foreach (const GatoUUID & filter_uuid, filter_uuids) {
			if (peripheral->advertisesService(filter_uuid)) {
				passes_filter = true;
				break;
			}
		}
	}

	if (passes_filter) {
		handleDiscoveredPeripheral(peripheral, rssi);
	}*/
}

//void GatoPeripheral::parseEIR(quint8 data[], int len)
//{
//	Q_D(GatoPeripheral);
//
//	int pos = 0;
//	while (pos < len) {
//		int item_len = data[pos];
//		pos++;
//		if (item_len == 0) break;
//		int type = data[pos];
//		assert(pos + item_len <= len);
//		switch (type) {
//		case EIRFlags:
//			d->parseEIRFlags(&data[pos + 1], item_len - 1);
//			break;
//		case EIRIncompleteUUID16List:
//			d->parseEIRUUIDs(16/8, false, &data[pos + 1], item_len - 1);
//			break;
//		case EIRCompleteUUID16List:
//			d->parseEIRUUIDs(16/8, true, &data[pos + 1], item_len - 1);
//			break;
//		case EIRIncompleteUUID32List:
//			d->parseEIRUUIDs(32/8, false, &data[pos + 1], item_len - 1);
//			break;
//		case EIRCompleteUUID32List:
//			d->parseEIRUUIDs(32/8, true, &data[pos + 1], item_len - 1);
//			break;
//		case EIRIncompleteUUID128List:
//			d->parseEIRUUIDs(128/8, false, &data[pos + 1], item_len - 1);
//			break;
//		case EIRCompleteUUID128List:
//			d->parseEIRUUIDs(128/8, true, &data[pos + 1], item_len - 1);
//			break;
//		case EIRIncompleteLocalName:
//			d->parseName(false, &data[pos + 1], item_len - 1);
//			break;
//		case EIRCompleteLocalName:
//			d->parseName(true, &data[pos + 1], item_len - 1);
//			break;
//		default:
//			qWarning() << "Unknown EIR data type" << type;
//			break;
//		}
//
//		pos += item_len;
//	}
//
//	assert(pos == len);
//}

void Tester::test()
{
	manager->scanForPeripherals();
}

void Tester::handleDiscoveredPeripheral(GatoPeripheral *peripheral, int rssi)
{
	qDebug() << "Found peripheral" << peripheral->address().toString() << peripheral->name();
//	if (peripheral->name() == "JN104FE9") {
		manager->stopScan();
		this->peripheral = peripheral;
		connect(peripheral, SIGNAL(connected()), SLOT(handleConnected()));
		connect(peripheral, SIGNAL(disconnected()), SLOT(handleDisconnected()));
		connect(peripheral, SIGNAL(servicesDiscovered()), SLOT(handleServices()));
		connect(peripheral, SIGNAL(characteristicsDiscovered(GatoService)), SLOT(handleCharacteristics(GatoService)));
		connect(peripheral, SIGNAL(valueUpdated(GatoCharacteristic,QByteArray)), SLOT(handleValueUpdated(GatoCharacteristic,QByteArray)));
		peripheral->connectPeripheral();
//	}
}

void Tester::handleConnected()
{
	qDebug() << "Peripheral connected";
	peripheral->discoverServices();

    info.create_mode = WDAEMON_CREATE;

	if (uinput->uinput_create(&info)) {
		qDebug() << "Device created successfully!";
	}

    }

void Tester::handleDisconnected()
{
	qDebug() << "Peripheral disconnected";
}

void Tester::handleServices()
{
	qDebug() << "Services found";
	foreach (const GatoService &service, peripheral->services()) {
		if (service.uuid() == GatoUUID("dcd68980-aadc-11e1-a22a-0002a5d5c51b")) {
			// Found the service we want
			qDebug() << "Found service!";
			peripheral->discoverCharacteristics(service);
		}
	}
}

void Tester::handleCharacteristics(const GatoService &service)
{
	foreach (const GatoCharacteristic &c, service.characteristics()) {
		if (c.uuid() == GatoUUID("00002a26-0000-1000-8000-00805f9b34fb")) {
			peripheral->readValue(c);
		} else if (c.uuid() == GatoUUID("00002a5a-0000-1000-8000-00805f9b34fb")) {
			peripheral->setNotification(c, true);
			agg_char = c;
		}
	}
}

void Tester::handleValueUpdated(const GatoCharacteristic &characteristic, const QByteArray &value)
{
	if (characteristic.uuid() == agg_char.uuid()) {
		QDataStream s(value);
		s.setByteOrder(QDataStream::BigEndian);
		qint16 p, x, y, z;
		s >> p >> x >> y >> z;
		handleReport(p, x, y, z);
	} else {
		qDebug() << "Value updated" << characteristic.uuid() << QString(value);
	}
}

void Tester::handleReport(int p, int x, int y, int z)
{
	struct input_event ev;

	gettimeofday(&ev.time, 0);

	int btn_0 = (p & 0x1);
	int btn_1 = (p & 0x2) >> 1;

	if (btn_0 != prev_btn_0) {
		ev.type = EV_KEY;
		ev.code = BTN_0;
		ev.value = btn_0;

		uinput->uinput_write_event(&info, &ev);
	}
	prev_btn_0 = btn_0;

	if (btn_1 != prev_btn_1) {
		ev.type = EV_KEY;
		ev.code = BTN_1;
		ev.value = btn_1;

		uinput->uinput_write_event(&info, &ev);
	}
	prev_btn_1 = btn_1;

	ev.type = EV_ABS;
	ev.code = ABS_PRESSURE;
	ev.value = (p >> 5) & 0x7ff;
	uinput->uinput_write_event(&info, &ev);

    //qDebug() << ev.time.tv_usec << ev.value << btn_0 << btn_1;
    //prointf("Reading...\n");
//    while (1) {
        ssize_t size = read(fd, &event, sizeof(struct input_event));
        while(size > 0) {
            struct input_event ev;
            printf("%d %i %i %i\n", event.time, event.type, event.code, event.value);
            if (event.type == 0) {
                printf("PACK %i %i\n", event.code, event.value);
            } else if (event.type == EV_KEY) {
                if (event.code == BTN_TOUCH) {
                    event.code = BTN_TOOL_PEN;
                }
                printf("EV_KEY code=%i value=%i\n", event.code, event.value);

                gettimeofday(&ev.time, 0);
                ev.type = EV_KEY;
                ev.code = event.code;
                ev.value = event.value;

                uinput->uinput_write_event(&info, &ev);
            } else if (event.type == EV_ABS) {
                printf("EV_ABS code=%i value=%i\n", event.code, event.value);

                gettimeofday(&ev.time, 0);

                ev.type = EV_ABS;

                if (event.code == ABS_X || event.code == ABS_Y) {
                    ev.code = event.code;
                    ev.value = event.value;
                }

                uinput->uinput_write_event(&info, &ev);
            }

            size = read(fd, &event, sizeof(struct input_event));

            //qDebug() << ev.time.tv_usec << ev.value << btn_0 << btn_1;
        }

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    uinput->uinput_write_event(&info, &ev);

//    }
    //printf("Done.\n");


}
