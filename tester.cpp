#include <QtCore/QDebug>
#include <gatodescriptor.h>

#include "tester.h"
#include "log.h"

Tester::Tester(QObject *parent)
    : QObject(parent)
{
    printf("Hello.\n");

	manager = new GatoCentralManager(this);
	connect(manager, SIGNAL(discoveredPeripheral(GatoPeripheral*,int)),
	        SLOT(handleDiscoveredPeripheral(GatoPeripheral*,int)));

    fd = open("/dev/input/event12", O_RDONLY | O_NONBLOCK);
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

void Tester::test()
{
	manager->scanForPeripherals();
}

void Tester::handleDiscoveredPeripheral(GatoPeripheral *peripheral, int rssi)
{
	qDebug() << "Found peripheral" << peripheral->address().toString() << peripheral->name();
	if (peripheral->name()[0] == 'J') {
		manager->stopScan();
		this->peripheral = peripheral;
		connect(peripheral, SIGNAL(connected()), SLOT(handleConnected()));
		connect(peripheral, SIGNAL(disconnected()), SLOT(handleDisconnected()));
		connect(peripheral, SIGNAL(servicesDiscovered()), SLOT(handleServices()));
		connect(peripheral, SIGNAL(characteristicsDiscovered(GatoService)), SLOT(handleCharacteristics(GatoService)));
		connect(peripheral, SIGNAL(valueUpdated(GatoCharacteristic,QByteArray)), SLOT(handleValueUpdated(GatoCharacteristic,QByteArray)));
		peripheral->connectPeripheral();
	}
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
            if (event.type == EV_SYN) {
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

                    uinput->uinput_write_event(&info, &ev);
                }

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
